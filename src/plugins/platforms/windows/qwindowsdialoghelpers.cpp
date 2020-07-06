/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define QT_NO_URL_CAST_FROM_STRING 1

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include "qwindowscombase.h"
#include "qwindowsdialoghelpers.h"

#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowstheme.h" // Color conversion helpers

#include <QtGui/qguiapplication.h>
#include <QtGui/qcolor.h>

#include <QtCore/qdebug.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdir.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmutex.h>
#include <QtCore/quuid.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/private/qsystemlibrary_p.h>

#include <algorithm>
#include <vector>

#include <QtCore/qt_windows.h>

// #define USE_NATIVE_COLOR_DIALOG /* Testing purposes only */

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM
/* Output UID (IID, CLSID) as C++ constants.
 * The constants are contained in the Windows SDK libs, but not for MinGW. */
static inline QString guidToString(const GUID &g)
{
    QString rc;
    QTextStream str(&rc);
    str.setIntegerBase(16);
    str.setNumberFlags(str.numberFlags() | QTextStream::ShowBase);
    str << '{' << g.Data1 << ", " << g.Data2 << ", " << g.Data3;
    str.setFieldWidth(2);
    str.setFieldAlignment(QTextStream::AlignRight);
    str.setPadChar(u'0');
    str << ",{" << g.Data4[0] << ", " << g.Data4[1]  << ", " << g.Data4[2]  << ", " << g.Data4[3]
        << ", " << g.Data4[4] << ", " << g.Data4[5]  << ", " << g.Data4[6]  << ", " << g.Data4[7]
        << "}};";
    return rc;
}

inline QDebug operator<<(QDebug d, const GUID &g)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << guidToString(g);
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

// Return an allocated wchar_t array from a QString, reserve more memory if desired.
static wchar_t *qStringToWCharArray(const QString &s, size_t reserveSize = 0)
{
    const size_t stringSize = s.size();
    wchar_t *result = new wchar_t[qMax(stringSize + 1, reserveSize)];
    s.toWCharArray(result);
    result[stringSize] = 0;
    return result;
}

namespace QWindowsDialogs
{
/*!
    \fn eatMouseMove()

    After closing a windows dialog with a double click (i.e. open a file)
    the message queue still contains a dubious WM_MOUSEMOVE message where
    the left button is reported to be down (wParam != 0).
    remove all those messages (usually 1) and post the last one with a
    reset button state.

*/

void eatMouseMove()
{
    MSG msg = {nullptr, 0, 0, 0, 0, {0, 0} };
    while (PeekMessage(&msg, nullptr, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
        ;
    if (msg.message == WM_MOUSEMOVE)
        PostMessage(msg.hwnd, msg.message, 0, msg.lParam);
    qCDebug(lcQpaDialogs) << __FUNCTION__ << "triggered=" << (msg.message == WM_MOUSEMOVE);
}

} // namespace QWindowsDialogs

/*!
    \class QWindowsNativeDialogBase
    \brief Base class for Windows native dialogs.

    Base classes for native dialogs (using the CLSID-based
    dialog interfaces "IFileDialog", etc. available from Windows
    Vista on) that mimick the behaviour of their QDialog
    counterparts as close as possible.

    Instances of derived classes are controlled by
    QWindowsDialogHelperBase-derived classes.

    A major difference is that there is only an exec(), which
    is a modal, blocking call; there is no non-blocking show().
    There 2 types of native dialogs:

    \list
    \li Dialogs provided by the Comdlg32 library (ChooseColor,
       ChooseFont). They only provide a modal, blocking
       function call (with idle processing).
    \li File dialogs are classes derived from IFileDialog. They
       inherit IModalWindow and their exec() method (calling
       IModalWindow::Show()) is similarly blocking, but methods
       like close() can be called on them from event handlers.
    \endlist

    \sa QWindowsDialogHelperBase
    \internal
*/

class QWindowsNativeDialogBase : public QObject
{
    Q_OBJECT
public:
    virtual void setWindowTitle(const QString &title) = 0;
    bool executed() const { return m_executed; }
    void exec(HWND owner = nullptr) { doExec(owner); m_executed = true; }

signals:
    void accepted();
    void rejected();

public slots:
    virtual void close() = 0;

protected:
    QWindowsNativeDialogBase() : m_executed(false) {}

private:
    virtual void doExec(HWND owner = nullptr) = 0;

    bool m_executed;
};

/*!
    \class QWindowsDialogHelperBase
    \brief Helper for native Windows dialogs.

    Provides basic functionality and introduces new virtuals.
    The native dialog is created in setVisible_sys() since
    then modality and the state of DontUseNativeDialog is known.

    Modal dialogs are then run by exec(). Non-modal dialogs are shown using a
    separate thread started in show() should they support it.

    \sa QWindowsDialogThread, QWindowsNativeDialogBase
    \internal
*/

template <class BaseClass>
void QWindowsDialogHelperBase<BaseClass>::cleanupThread()
{
    if (m_thread) { // Thread may be running if the dialog failed to close.
        if (m_thread->isRunning())
            m_thread->wait(500);
        if (m_thread->isRunning()) {
            m_thread->terminate();
            m_thread->wait(300);
            if (m_thread->isRunning())
                qCCritical(lcQpaDialogs) <<__FUNCTION__ << "Failed to terminate thread.";
            else
                qCWarning(lcQpaDialogs) << __FUNCTION__ << "Thread terminated.";
        }
        delete m_thread;
        m_thread = nullptr;
    }
}

template <class BaseClass>
QWindowsNativeDialogBase *QWindowsDialogHelperBase<BaseClass>::nativeDialog() const
{
    if (m_nativeDialog.isNull()) {
         qWarning("%s invoked with no native dialog present.", __FUNCTION__);
         return nullptr;
    }
    return m_nativeDialog.data();
}

template <class BaseClass>
void QWindowsDialogHelperBase<BaseClass>::timerEvent(QTimerEvent *)
{
    startDialogThread();
}

template <class BaseClass>
QWindowsNativeDialogBase *QWindowsDialogHelperBase<BaseClass>::ensureNativeDialog()
{
    // Create dialog and apply common settings. Check "executed" flag as well
    // since for example IFileDialog::Show() works only once.
    if (m_nativeDialog.isNull() || m_nativeDialog->executed())
        m_nativeDialog = QWindowsNativeDialogBasePtr(createNativeDialog());
    return m_nativeDialog.data();
}

/*!
    \class QWindowsDialogThread
    \brief Run a non-modal native dialog in a separate thread.

    \sa QWindowsDialogHelperBase
    \internal
*/

class QWindowsDialogThread : public QThread
{
public:
    using QWindowsNativeDialogBasePtr = QSharedPointer<QWindowsNativeDialogBase>;

    explicit QWindowsDialogThread(const QWindowsNativeDialogBasePtr &d, HWND owner)
        : m_dialog(d), m_owner(owner) {}
    void run();

private:
    const QWindowsNativeDialogBasePtr m_dialog;
    const HWND m_owner;
};

void QWindowsDialogThread::run()
{
    qCDebug(lcQpaDialogs) << '>' << __FUNCTION__;
    m_dialog->exec(m_owner);
    qCDebug(lcQpaDialogs) << '<' << __FUNCTION__;
}

template <class BaseClass>
bool QWindowsDialogHelperBase<BaseClass>::show(Qt::WindowFlags,
                                                   Qt::WindowModality windowModality,
                                                   QWindow *parent)
{
    const bool modal = (windowModality != Qt::NonModal);
    if (!parent)
        parent = QGuiApplication::focusWindow(); // Need a parent window, else the application loses activation when closed.
    if (parent) {
        m_ownerWindow = QWindowsWindow::handleOf(parent);
    } else {
        m_ownerWindow = nullptr;
    }
    qCDebug(lcQpaDialogs) << __FUNCTION__ << "modal=" << modal
        << " modal supported? " << supportsNonModalDialog(parent)
        << "native=" << m_nativeDialog.data() << "owner" << m_ownerWindow;
    if (!modal && !supportsNonModalDialog(parent))
        return false; // Was it changed in-between?
    if (!ensureNativeDialog())
        return false;
    // Start a background thread to show the dialog. For modal dialogs,
    // a subsequent call to exec() may follow. So, start an idle timer
    // which will start the dialog thread. If exec() is then called, the
    // timer is stopped and dialog->exec() is called directly.
    cleanupThread();
    if (modal) {
        m_timerId = this->startTimer(0);
    } else {
        startDialogThread();
    }
    return true;
}

template <class BaseClass>
void QWindowsDialogHelperBase<BaseClass>::startDialogThread()
{
    Q_ASSERT(!m_nativeDialog.isNull());
    Q_ASSERT(!m_thread);
    m_thread = new QWindowsDialogThread(m_nativeDialog, m_ownerWindow);
    m_thread->start();
    stopTimer();
}

template <class BaseClass>
void QWindowsDialogHelperBase<BaseClass>::stopTimer()
{
    if (m_timerId) {
        this->killTimer(m_timerId);
        m_timerId = 0;
    }
}

// Find a file dialog window created by IFileDialog by process id, window
// title and class, which starts with a hash '#'.

struct FindDialogContext
{
    explicit FindDialogContext(const QString &titleIn)
        : title(qStringToWCharArray(titleIn)), processId(GetCurrentProcessId()), hwnd(nullptr) {}

    const QScopedArrayPointer<wchar_t> title;
    const DWORD processId;
    HWND hwnd; // contains the HWND of the window found.
};

static BOOL QT_WIN_CALLBACK findDialogEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto *context = reinterpret_cast<FindDialogContext *>(lParam);
    DWORD winPid = 0;
    GetWindowThreadProcessId(hwnd, &winPid);
    if (winPid != context->processId)
        return TRUE;
    wchar_t buf[256];
    if (!RealGetWindowClass(hwnd, buf, sizeof(buf)/sizeof(wchar_t)) || buf[0] != L'#')
        return TRUE;
    if (!GetWindowTextW(hwnd, buf, sizeof(buf)/sizeof(wchar_t)) || wcscmp(buf, context->title.data()) != 0)
        return TRUE;
    context->hwnd = hwnd;
    return FALSE;
}

static inline HWND findDialogWindow(const QString &title)
{
    FindDialogContext context(title);
    EnumWindows(findDialogEnumWindowsProc, reinterpret_cast<LPARAM>(&context));
    return context.hwnd;
}

template <class BaseClass>
void QWindowsDialogHelperBase<BaseClass>::hide()
{
    if (m_nativeDialog)
        m_nativeDialog->close();
    m_ownerWindow = nullptr;
}

template <class BaseClass>
void QWindowsDialogHelperBase<BaseClass>::exec()
{
    qCDebug(lcQpaDialogs) << __FUNCTION__;
    stopTimer();
    if (QWindowsNativeDialogBase *nd = nativeDialog()) {
         nd->exec(m_ownerWindow);
         m_nativeDialog.clear();
    }
}

/*!
    \class QWindowsFileDialogSharedData
    \brief Explicitly shared file dialog parameters that are not in QFileDialogOptions.

    Contain Parameters that need to be cached while the native dialog does not
    exist yet. In addition, the data are updated by the change notifications of the
    IFileDialogEvent, as querying them after the dialog has closed
    does not reliably work. Provides thread-safe setters (for the non-modal case).

    \internal
    \sa QFileDialogOptions
*/

class QWindowsFileDialogSharedData
{
public:
    QWindowsFileDialogSharedData() : m_data(new Data) {}
    void fromOptions(const QSharedPointer<QFileDialogOptions> &o);

    QUrl directory() const;
    void setDirectory(const QUrl &);
    QString selectedNameFilter() const;
    void setSelectedNameFilter(const QString &);
    QList<QUrl> selectedFiles() const;
    void setSelectedFiles(const QList<QUrl> &);
    QString selectedFile() const;

private:
    class Data : public QSharedData {
    public:
        QUrl directory;
        QString selectedNameFilter;
        QList<QUrl> selectedFiles;
        QMutex mutex;
    };
    QExplicitlySharedDataPointer<Data> m_data;
};

inline QUrl QWindowsFileDialogSharedData::directory() const
{
    m_data->mutex.lock();
    const QUrl result = m_data->directory;
    m_data->mutex.unlock();
    return result;
}

inline void QWindowsFileDialogSharedData::setDirectory(const QUrl &d)
{
    QMutexLocker locker(&m_data->mutex);
    m_data->directory = d;
}

inline QString QWindowsFileDialogSharedData::selectedNameFilter() const
{
    m_data->mutex.lock();
    const QString result = m_data->selectedNameFilter;
    m_data->mutex.unlock();
    return result;
}

inline void QWindowsFileDialogSharedData::setSelectedNameFilter(const QString &f)
{
    QMutexLocker locker(&m_data->mutex);
    m_data->selectedNameFilter = f;
}

inline QList<QUrl> QWindowsFileDialogSharedData::selectedFiles() const
{
    m_data->mutex.lock();
    const auto result = m_data->selectedFiles;
    m_data->mutex.unlock();
    return result;
}

inline QString QWindowsFileDialogSharedData::selectedFile() const
{
    const auto files = selectedFiles();
    return files.isEmpty() ? QString() : files.front().toLocalFile();
}

inline void QWindowsFileDialogSharedData::setSelectedFiles(const QList<QUrl> &urls)
{
    QMutexLocker locker(&m_data->mutex);
    m_data->selectedFiles = urls;
}

inline void QWindowsFileDialogSharedData::fromOptions(const QSharedPointer<QFileDialogOptions> &o)
{
    QMutexLocker locker(&m_data->mutex);
    m_data->directory = o->initialDirectory();
    m_data->selectedFiles = o->initiallySelectedFiles();
    m_data->selectedNameFilter = o->initiallySelectedNameFilter();
}

/*!
    \class QWindowsNativeFileDialogEventHandler
    \brief Listens to IFileDialog events and forwards them to QWindowsNativeFileDialogBase

    Events like 'folder change' that have an equivalent signal
    in QFileDialog are forwarded.

    \sa QWindowsNativeFileDialogBase, QWindowsFileDialogHelper
    \internal
*/

class QWindowsNativeFileDialogBase;

class QWindowsNativeFileDialogEventHandler : public QWindowsComBase<IFileDialogEvents>
{
    Q_DISABLE_COPY_MOVE(QWindowsNativeFileDialogEventHandler)
public:
    static IFileDialogEvents *create(QWindowsNativeFileDialogBase *nativeFileDialog);

    // IFileDialogEvents methods
    IFACEMETHODIMP OnFileOk(IFileDialog *);
    IFACEMETHODIMP OnFolderChange(IFileDialog *) { return S_OK; }
    IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *);
    IFACEMETHODIMP OnHelp(IFileDialog *) { return S_OK; }
    IFACEMETHODIMP OnSelectionChange(IFileDialog *);
    IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) { return S_OK; }
    IFACEMETHODIMP OnTypeChange(IFileDialog *);
    IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) { return S_OK; }

    QWindowsNativeFileDialogEventHandler(QWindowsNativeFileDialogBase *nativeFileDialog) :
        m_nativeFileDialog(nativeFileDialog) {}

private:
    QWindowsNativeFileDialogBase *m_nativeFileDialog;
};

IFileDialogEvents *QWindowsNativeFileDialogEventHandler::create(QWindowsNativeFileDialogBase *nativeFileDialog)
{
    IFileDialogEvents *result;
    auto *eventHandler = new QWindowsNativeFileDialogEventHandler(nativeFileDialog);
    if (FAILED(eventHandler->QueryInterface(IID_IFileDialogEvents, reinterpret_cast<void **>(&result)))) {
        qErrnoWarning("Unable to obtain IFileDialogEvents");
        return nullptr;
    }
    eventHandler->Release();
    return result;
}

/*!
    \class QWindowsShellItem
    \brief Wrapper for IShellItem

    \sa QWindowsNativeFileDialogBase
    \internal
*/
class QWindowsShellItem
{
public:
    using IShellItems = std::vector<IShellItem *>;

    explicit QWindowsShellItem(IShellItem *item);

    SFGAOF attributes() const { return m_attributes; }
    QString normalDisplay() const // base name, usually
        { return displayName(m_item, SIGDN_NORMALDISPLAY); }
    QString urlString() const
        { return displayName(m_item, SIGDN_URL); }
    QString fileSysPath() const
        { return displayName(m_item, SIGDN_FILESYSPATH); }
    QString desktopAbsoluteParsing() const
        { return displayName(m_item, SIGDN_DESKTOPABSOLUTEPARSING); }
    QString path() const; // Only set for 'FileSystem' (SFGAO_FILESYSTEM) items
    QUrl url() const;

    bool isFileSystem() const  { return (m_attributes & SFGAO_FILESYSTEM) != 0; }
    bool isDir() const         { return (m_attributes & SFGAO_FOLDER) != 0; }
    // Supports IStream
    bool canStream() const     { return (m_attributes & SFGAO_STREAM) != 0; }

    bool copyData(QIODevice *out, QString *errorMessage);

    static IShellItems itemsFromItemArray(IShellItemArray *items);

#ifndef QT_NO_DEBUG_STREAM
    void format(QDebug &d) const;
#endif

private:
    static QString displayName(IShellItem *item, SIGDN mode);
    static QString libraryItemDefaultSaveFolder(IShellItem *item);
    QUrl urlValue() const;

    IShellItem *m_item;
    SFGAOF m_attributes;
};

QWindowsShellItem::QWindowsShellItem(IShellItem *item)
    : m_item(item)
    , m_attributes(0)
{
    if (FAILED(item->GetAttributes(SFGAO_CAPABILITYMASK | SFGAO_DISPLAYATTRMASK | SFGAO_CONTENTSMASK | SFGAO_STORAGECAPMASK, &m_attributes)))
        m_attributes = 0;
}

QString QWindowsShellItem::path() const
{
    if (isFileSystem())
        return QDir::cleanPath(QWindowsShellItem::displayName(m_item, SIGDN_FILESYSPATH));
    // Check for a "Library" item
    if (isDir())
        return QWindowsShellItem::libraryItemDefaultSaveFolder(m_item);
    return QString();
}

QUrl QWindowsShellItem::urlValue() const // plain URL as returned by SIGDN_URL, not set for all items
{
    QUrl result;
    const QString urlString = displayName(m_item, SIGDN_URL);
    if (!urlString.isEmpty()) {
        const QUrl parsed = QUrl(urlString);
        if (parsed.isValid()) {
            result = parsed;
        } else {
            qWarning("%s: Unable to decode URL \"%s\": %s", __FUNCTION__,
                     qPrintable(urlString), qPrintable(parsed.errorString()));
        }
    }
    return result;
}

QUrl QWindowsShellItem::url() const
{
    // Prefer file if existent to avoid any misunderstandings about UNC shares
    const QString fsPath = path();
    if (!fsPath.isEmpty())
        return QUrl::fromLocalFile(fsPath);
    const QUrl urlV = urlValue();
    if (urlV.isValid())
        return urlV;
    // Last resort: encode the absolute desktop parsing id as data URL
    const QString data = QStringLiteral("data:text/plain;base64,")
        + QLatin1String(desktopAbsoluteParsing().toLatin1().toBase64());
    return QUrl(data);
}

QString QWindowsShellItem::displayName(IShellItem *item, SIGDN mode)
{
    LPWSTR name = nullptr;
    QString result;
    if (SUCCEEDED(item->GetDisplayName(mode, &name))) {
        result = QString::fromWCharArray(name);
        CoTaskMemFree(name);
    }
    return result;
}

QWindowsShellItem::IShellItems QWindowsShellItem::itemsFromItemArray(IShellItemArray *items)
{
    IShellItems result;
    DWORD itemCount = 0;
    if (FAILED(items->GetCount(&itemCount)) || itemCount == 0)
        return result;
    result.reserve(itemCount);
    for (DWORD i = 0; i < itemCount; ++i) {
        IShellItem *item = nullptr;
        if (SUCCEEDED(items->GetItemAt(i, &item)))
            result.push_back(item);
    }
    return result;
}

bool QWindowsShellItem::copyData(QIODevice *out, QString *errorMessage)
{
    if (!canStream()) {
        *errorMessage = QLatin1String("Item not streamable");
        return false;
    }
    IStream *istream = nullptr;
    HRESULT hr = m_item->BindToHandler(nullptr, BHID_Stream, IID_PPV_ARGS(&istream));
    if (FAILED(hr)) {
        *errorMessage = QLatin1String("BindToHandler() failed: ")
                        + QLatin1String(QWindowsContext::comErrorString(hr));
        return false;
    }
    enum : ULONG { bufSize = 102400 };
    char buffer[bufSize];
    ULONG bytesRead;
    forever {
        bytesRead = 0;
        hr = istream->Read(buffer, bufSize, &bytesRead); // S_FALSE: EOF reached
        if ((hr == S_OK || hr == S_FALSE) && bytesRead)
            out->write(buffer, bytesRead);
        else
            break;
    }
    istream->Release();
    if (hr != S_OK && hr != S_FALSE) {
        *errorMessage = QLatin1String("Read() failed: ")
                        + QLatin1String(QWindowsContext::comErrorString(hr));
        return false;
    }
    return true;
}

// Helper for "Libraries": collections of folders appearing from Windows 7
// on, visible in the file dialogs.

// Load a library from a IShellItem (sanitized copy of the inline function
// SHLoadLibraryFromItem from ShObjIdl.h, which does not exist for MinGW).
static IShellLibrary *sHLoadLibraryFromItem(IShellItem *libraryItem, DWORD mode)
{
    // ID symbols present from Windows 7 on:
    static const CLSID classId_ShellLibrary = {0xd9b3211d, 0xe57f, 0x4426, {0xaa, 0xef, 0x30, 0xa8, 0x6, 0xad, 0xd3, 0x97}};
    static const IID   iId_IShellLibrary    = {0x11a66efa, 0x382e, 0x451a, {0x92, 0x34, 0x1e, 0xe, 0x12, 0xef, 0x30, 0x85}};

    IShellLibrary *helper = nullptr;
    IShellLibrary *result =  nullptr;
    if (SUCCEEDED(CoCreateInstance(classId_ShellLibrary, nullptr, CLSCTX_INPROC_SERVER, iId_IShellLibrary, reinterpret_cast<void **>(&helper))))
        if (SUCCEEDED(helper->LoadLibraryFromItem(libraryItem, mode)))
            helper->QueryInterface(iId_IShellLibrary, reinterpret_cast<void **>(&result));
    if (helper)
        helper->Release();
    return result;
}

// Return default save folders of a library-type item.
QString QWindowsShellItem::libraryItemDefaultSaveFolder(IShellItem *item)
{
    QString result;
    if (IShellLibrary *library = sHLoadLibraryFromItem(item, STGM_READ | STGM_SHARE_DENY_WRITE)) {
        IShellItem *item = nullptr;
        if (SUCCEEDED(library->GetDefaultSaveFolder(DSFT_DETECT, IID_IShellItem, reinterpret_cast<void **>(&item)))) {
            result = QDir::cleanPath(QWindowsShellItem::displayName(item, SIGDN_FILESYSPATH));
            item->Release();
        }
        library->Release();
    }
    return result;
}

#ifndef QT_NO_DEBUG_STREAM
void QWindowsShellItem::format(QDebug &d) const
{
    d << "attributes=0x" << Qt::hex << attributes() << Qt::dec;
    if (isFileSystem())
        d << " [filesys]";
    if (isDir())
        d << " [dir]";
    if (canStream())
        d << " [stream]";
    d << ", normalDisplay=\"" << normalDisplay()
        << "\", desktopAbsoluteParsing=\"" << desktopAbsoluteParsing()
        << "\", urlString=\"" << urlString() << "\", fileSysPath=\"" << fileSysPath() << '"';
    const QString pathS = path();
    if (!pathS.isEmpty())
        d << ", path=\"" << pathS << '"';
    const QUrl urlV = urlValue();
    if (urlV.isValid())
        d << "\", url=" << urlV;
}

QDebug operator<<(QDebug d, const QWindowsShellItem &i)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "QShellItem(";
    i.format(d);
    d << ')';
    return d;
}

QDebug operator<<(QDebug d, IShellItem *i)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "IShellItem(" << static_cast<const void *>(i);
    if (i) {
        d << ", ";
        QWindowsShellItem(i).format(d);
    }
    d << ')';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \class QWindowsNativeFileDialogBase
    \brief Windows native file dialog wrapper around IFileOpenDialog, IFileSaveDialog.

    Provides convenience methods.
    Note that only IFileOpenDialog has multi-file functionality.

    \sa QWindowsNativeFileDialogEventHandler, QWindowsFileDialogHelper
    \internal
*/

class QWindowsNativeFileDialogBase : public QWindowsNativeDialogBase
{
    Q_OBJECT
    Q_PROPERTY(bool hideFiltersDetails READ hideFiltersDetails WRITE setHideFiltersDetails)
public:
    ~QWindowsNativeFileDialogBase() override;

    inline static QWindowsNativeFileDialogBase *create(QFileDialogOptions::AcceptMode am, const QWindowsFileDialogSharedData &data);

    void setWindowTitle(const QString &title) override;
    inline void setMode(QFileDialogOptions::FileMode mode, QFileDialogOptions::AcceptMode acceptMode, QFileDialogOptions::FileDialogOptions options);
    inline void setDirectory(const QUrl &directory);
    inline void updateDirectory() { setDirectory(m_data.directory()); }
    inline QString directory() const;
    void doExec(HWND owner = nullptr) override;
    virtual void setNameFilters(const QStringList &f);
    inline void selectNameFilter(const QString &filter);
    inline void updateSelectedNameFilter() { selectNameFilter(m_data.selectedNameFilter()); }
    inline QString selectedNameFilter() const;
    void selectFile(const QString &fileName) const;
    bool hideFiltersDetails() const    { return m_hideFiltersDetails; }
    void setHideFiltersDetails(bool h) { m_hideFiltersDetails = h; }
    void setDefaultSuffix(const QString &s);
    inline bool hasDefaultSuffix() const  { return m_hasDefaultSuffix; }
    inline void setLabelText(QFileDialogOptions::DialogLabel l, const QString &text);

    // Return the selected files for tracking in OnSelectionChanged().
    virtual QList<QUrl> selectedFiles() const = 0;
    // Return the result for tracking in OnFileOk(). Differs from selection for
    // example by appended default suffixes, etc.
    virtual QList<QUrl> dialogResult() const = 0;

    inline void onFolderChange(IShellItem *);
    inline void onSelectionChange();
    inline void onTypeChange();
    inline bool onFileOk();

signals:
    void directoryEntered(const QUrl &directory);
    void currentChanged(const QUrl &file);
    void filterSelected(const QString & filter);

public slots:
    void close() override;

protected:
    explicit QWindowsNativeFileDialogBase(const QWindowsFileDialogSharedData &data);
    bool init(const CLSID &clsId, const IID &iid);
    void setDefaultSuffixSys(const QString &s);
    inline IFileDialog * fileDialog() const { return m_fileDialog; }
    static IShellItem *shellItem(const QUrl &url);

    const QWindowsFileDialogSharedData &data() const { return m_data; }
    QWindowsFileDialogSharedData &data() { return m_data; }

private:
    IFileDialog *m_fileDialog = nullptr;
    IFileDialogEvents *m_dialogEvents = nullptr;
    DWORD m_cookie = 0;
    QStringList m_nameFilters;
    bool m_hideFiltersDetails = false;
    bool m_hasDefaultSuffix = false;
    QWindowsFileDialogSharedData m_data;
    QString m_title;
};

QWindowsNativeFileDialogBase::QWindowsNativeFileDialogBase(const QWindowsFileDialogSharedData &data) :
    m_data(data)
{
}

QWindowsNativeFileDialogBase::~QWindowsNativeFileDialogBase()
{
    if (m_dialogEvents && m_fileDialog)
        m_fileDialog->Unadvise(m_cookie);
    if (m_dialogEvents)
        m_dialogEvents->Release();
    if (m_fileDialog)
        m_fileDialog->Release();
}

bool QWindowsNativeFileDialogBase::init(const CLSID &clsId, const IID &iid)
{
    HRESULT hr = CoCreateInstance(clsId, nullptr, CLSCTX_INPROC_SERVER,
                                  iid, reinterpret_cast<void **>(&m_fileDialog));
    if (FAILED(hr)) {
        qErrnoWarning("CoCreateInstance failed");
        return false;
    }
    m_dialogEvents = QWindowsNativeFileDialogEventHandler::create(this);
    if (!m_dialogEvents)
        return false;
    // Register event handler
    hr = m_fileDialog->Advise(m_dialogEvents, &m_cookie);
    if (FAILED(hr)) {
        qErrnoWarning("IFileDialog::Advise failed");
        return false;
    }
    qCDebug(lcQpaDialogs) << __FUNCTION__ << m_fileDialog << m_dialogEvents <<  m_cookie;

    return true;
}

void QWindowsNativeFileDialogBase::setWindowTitle(const QString &title)
{
    m_title = title;
    m_fileDialog->SetTitle(reinterpret_cast<const wchar_t *>(title.utf16()));
}

IShellItem *QWindowsNativeFileDialogBase::shellItem(const QUrl &url)
{
    if (url.isLocalFile()) {
        IShellItem *result = nullptr;
        const QString native = QDir::toNativeSeparators(url.toLocalFile());
        const HRESULT hr =
                SHCreateItemFromParsingName(reinterpret_cast<const wchar_t *>(native.utf16()),
                                            nullptr, IID_IShellItem,
                                            reinterpret_cast<void **>(&result));
        if (FAILED(hr)) {
            qErrnoWarning("%s: SHCreateItemFromParsingName(%s)) failed", __FUNCTION__, qPrintable(url.toString()));
            return nullptr;
        }
        return result;
    } else if (url.scheme() == u"clsid") {
        // Support for virtual folders via GUID
        // (see https://msdn.microsoft.com/en-us/library/windows/desktop/dd378457(v=vs.85).aspx)
        // specified as "clsid:<GUID>" (without '{', '}').
        IShellItem *result = nullptr;
        const auto uuid = QUuid::fromString(url.path());
        if (uuid.isNull()) {
            qWarning() << __FUNCTION__ << ": Invalid CLSID: " << url.path();
            return nullptr;
        }
        PIDLIST_ABSOLUTE idList;
        HRESULT hr = SHGetKnownFolderIDList(uuid, 0, nullptr, &idList);
        if (FAILED(hr)) {
            qErrnoWarning("%s: SHGetKnownFolderIDList(%s)) failed", __FUNCTION__, qPrintable(url.toString()));
            return nullptr;
        }
        hr = SHCreateItemFromIDList(idList, IID_IShellItem, reinterpret_cast<void **>(&result));
        CoTaskMemFree(idList);
        if (FAILED(hr)) {
            qErrnoWarning("%s: SHCreateItemFromIDList(%s)) failed", __FUNCTION__, qPrintable(url.toString()));
            return nullptr;
        }
        return result;
    } else {
        qWarning() << __FUNCTION__ << ": Unhandled scheme: " << url.scheme();
    }
    return nullptr;
}

void QWindowsNativeFileDialogBase::setDirectory(const QUrl &directory)
{
    if (!directory.isEmpty()) {
        if (IShellItem *psi = QWindowsNativeFileDialogBase::shellItem(directory)) {
            m_fileDialog->SetFolder(psi);
            psi->Release();
        }
    }
}

QString QWindowsNativeFileDialogBase::directory() const
{
    QString result;
    IShellItem *item = nullptr;
    if (m_fileDialog && SUCCEEDED(m_fileDialog->GetFolder(&item)) && item) {
        result = QWindowsShellItem(item).path();
        item->Release();
    }
    return result;
}

void QWindowsNativeFileDialogBase::doExec(HWND owner)
{
    qCDebug(lcQpaDialogs) << '>' << __FUNCTION__;
    // Show() blocks until the user closes the dialog, the dialog window
    // gets a WM_CLOSE or the parent window is destroyed.
    const HRESULT hr = m_fileDialog->Show(owner);
    QWindowsDialogs::eatMouseMove();
    qCDebug(lcQpaDialogs) << '<' << __FUNCTION__ << " returns " << Qt::hex << hr;
    // Emit accepted() only if there is a result as otherwise UI hangs occur.
    // For example, typing in invalid URLs results in empty result lists.
    if (hr == S_OK && !m_data.selectedFiles().isEmpty()) {
        emit accepted();
    } else {
        emit rejected();
    }
}

void QWindowsNativeFileDialogBase::setMode(QFileDialogOptions::FileMode mode,
                                           QFileDialogOptions::AcceptMode acceptMode,
                                           QFileDialogOptions::FileDialogOptions options)
{
    DWORD flags = FOS_PATHMUSTEXIST;
    if (QWindowsContext::readAdvancedExplorerSettings(L"Hidden", 1) == 1) // 1:show, 2:hidden
        flags |= FOS_FORCESHOWHIDDEN;
    if (options & QFileDialogOptions::DontResolveSymlinks)
        flags |= FOS_NODEREFERENCELINKS;
    switch (mode) {
    case QFileDialogOptions::AnyFile:
        if (acceptMode == QFileDialogOptions::AcceptSave)
            flags |= FOS_NOREADONLYRETURN;
        if (!(options & QFileDialogOptions::DontConfirmOverwrite))
            flags |= FOS_OVERWRITEPROMPT;
        break;
    case QFileDialogOptions::ExistingFile:
        flags |= FOS_FILEMUSTEXIST;
        break;
    case QFileDialogOptions::Directory:
    case QFileDialogOptions::DirectoryOnly:
        // QTBUG-63645: Restrict to file system items, as Qt cannot deal with
        // places like 'Network', etc.
        flags |= FOS_PICKFOLDERS | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM;
        break;
    case QFileDialogOptions::ExistingFiles:
        flags |= FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT;
        break;
    }
    qCDebug(lcQpaDialogs) << __FUNCTION__ << "mode=" << mode
        << "acceptMode=" << acceptMode << "options=" << options
        << "results in" << Qt::showbase << Qt::hex << flags;

    if (FAILED(m_fileDialog->SetOptions(flags)))
        qErrnoWarning("%s: SetOptions() failed", __FUNCTION__);
}

// Split a list of name filters into description and actual filters
struct FilterSpec
{
    QString description;
    QString filter;
};

static QList<FilterSpec> filterSpecs(const QStringList &filters,
                                     bool hideFilterDetails,
                                     int *totalStringLength)
{
    QList<FilterSpec> result;
    result.reserve(filters.size());
    *totalStringLength = 0;

    const QRegularExpression filterSeparatorRE(QStringLiteral("[;\\s]+"));
    const QString separator = QStringLiteral(";");
    Q_ASSERT(filterSeparatorRE.isValid());
    // Split filter specification as 'Texts (*.txt[;] *.doc)', '*.txt[;] *.doc'
    // into description and filters specification as '*.txt;*.doc'
    for (const QString &filterString : filters) {
        const int openingParenPos = filterString.lastIndexOf(u'(');
        const int closingParenPos = openingParenPos != -1 ?
            filterString.indexOf(u')', openingParenPos + 1) : -1;
        FilterSpec filterSpec;
        filterSpec.filter = closingParenPos == -1 ?
            filterString :
            filterString.mid(openingParenPos + 1, closingParenPos - openingParenPos - 1).trimmed();
        if (filterSpec.filter.isEmpty())
            filterSpec.filter += u'*';
        filterSpec.filter.replace(filterSeparatorRE, separator);
        filterSpec.description = filterString;
        if (hideFilterDetails && openingParenPos != -1) { // Do not show pattern in description
            filterSpec.description.truncate(openingParenPos);
            while (filterSpec.description.endsWith(u' '))
                filterSpec.description.truncate(filterSpec.description.size() - 1);
        }
        *totalStringLength += filterSpec.filter.size() + filterSpec.description.size();
        result.push_back(filterSpec);
    }
    return result;
}

void QWindowsNativeFileDialogBase::setNameFilters(const QStringList &filters)
{
    /* Populates an array of COMDLG_FILTERSPEC from list of filters,
     * store the strings in a flat, contiguous buffer. */
    m_nameFilters = filters;
    int totalStringLength = 0;
    const QList<FilterSpec> specs = filterSpecs(filters, m_hideFiltersDetails, &totalStringLength);
    const int size = specs.size();

    QScopedArrayPointer<WCHAR> buffer(new WCHAR[totalStringLength + 2 * size]);
    QScopedArrayPointer<COMDLG_FILTERSPEC> comFilterSpec(new COMDLG_FILTERSPEC[size]);

    WCHAR *ptr = buffer.data();
    // Split filter specification as 'Texts (*.txt[;] *.doc)'
    // into description and filters specification as '*.txt;*.doc'

    for (int i = 0; i < size; ++i) {
        // Display glitch (CLSID only): Any filter not filtering on suffix (such as
        // '*', 'a.*') will be duplicated in combo: 'All files (*) (*)',
        // 'AAA files (a.*) (a.*)'
        QString description = specs[i].description;
        const QString &filter = specs[i].filter;
        if (!m_hideFiltersDetails && !filter.startsWith(u"*.")) {
            const int pos = description.lastIndexOf(u'(');
            if (pos > 0) {
                description.truncate(pos);
                while (!description.isEmpty() && description.back().isSpace())
                    description.chop(1);
            }
        }
        // Add to buffer.
        comFilterSpec[i].pszName = ptr;
        ptr += description.toWCharArray(ptr);
        *ptr++ = 0;
        comFilterSpec[i].pszSpec = ptr;
        ptr += specs[i].filter.toWCharArray(ptr);
        *ptr++ = 0;
    }

    m_fileDialog->SetFileTypes(size, comFilterSpec.data());
}

void QWindowsNativeFileDialogBase::setDefaultSuffix(const QString &s)
{
    setDefaultSuffixSys(s);
    m_hasDefaultSuffix = !s.isEmpty();
}

void QWindowsNativeFileDialogBase::setDefaultSuffixSys(const QString &s)
{
    // If this parameter is non-empty, it will be appended by the dialog for the 'Any files'
    // filter ('*'). If this parameter is non-empty and the current filter has a suffix,
    // the dialog will append the filter's suffix.
    auto *wSuffix = const_cast<wchar_t *>(reinterpret_cast<const wchar_t *>(s.utf16()));
    m_fileDialog->SetDefaultExtension(wSuffix);
}

static inline IFileDialog2 *getFileDialog2(IFileDialog *fileDialog)
{
    IFileDialog2 *result;
    return SUCCEEDED(fileDialog->QueryInterface(IID_IFileDialog2, reinterpret_cast<void **>(&result)))
        ? result : nullptr;
}

void QWindowsNativeFileDialogBase::setLabelText(QFileDialogOptions::DialogLabel l, const QString &text)
{
    auto *wText = const_cast<wchar_t *>(reinterpret_cast<const wchar_t *>(text.utf16()));
    switch (l) {
    case QFileDialogOptions::FileName:
        m_fileDialog->SetFileNameLabel(wText);
        break;
    case QFileDialogOptions::Accept:
        m_fileDialog->SetOkButtonLabel(wText);
        break;
    case QFileDialogOptions::Reject:
        if (IFileDialog2 *dialog2 = getFileDialog2(m_fileDialog)) {
            dialog2->SetCancelButtonLabel(wText);
            dialog2->Release();
        }
        break;
    case QFileDialogOptions::LookIn:
    case QFileDialogOptions::FileType:
    case QFileDialogOptions::DialogLabelCount:
        break;
    }
}

static bool isHexRange(const QString& s, int start, int end)
{
    for (;start < end; ++start) {
        QChar ch = s.at(start);
        if (!(ch.isDigit()
              || (ch >= u'a' && ch <= u'f')
              || (ch >= u'A' && ch <= u'F')))
            return false;
    }
    return true;
}

static inline bool isClsid(const QString &s)
{
    // detect "374DE290-123F-4565-9164-39C4925E467B".
    const QChar dash(u'-');
    return s.size() == 36
            && isHexRange(s, 0, 8)
            && s.at(8) == dash
            && isHexRange(s, 9, 13)
            && s.at(13) == dash
            && isHexRange(s, 14, 18)
            && s.at(18) == dash
            && isHexRange(s, 19, 23)
            && s.at(23) == dash
            && isHexRange(s, 24, 36);
}

void QWindowsNativeFileDialogBase::selectFile(const QString &fileName) const
{
    // Hack to prevent CLSIDs from being set as file name due to
    // QFileDialogPrivate::initialSelection() being QString-based.
    if (!isClsid(fileName))
        m_fileDialog->SetFileName((wchar_t*)fileName.utf16());
}

// Return the index of the selected filter, accounting for QFileDialog
// sometimes stripping the filter specification depending on the
// hideFilterDetails setting.
static int indexOfNameFilter(const QStringList &filters, const QString &needle)
{
    const int index = filters.indexOf(needle);
    if (index >= 0)
        return index;
    for (int i = 0; i < filters.size(); ++i)
        if (filters.at(i).startsWith(needle))
            return i;
    return -1;
}

void QWindowsNativeFileDialogBase::selectNameFilter(const QString &filter)
{
    if (filter.isEmpty())
        return;
    const int index = indexOfNameFilter(m_nameFilters, filter);
    if (index < 0) {
        qWarning("%s: Invalid parameter '%s' not found in '%s'.",
                 __FUNCTION__, qPrintable(filter),
                 qPrintable(m_nameFilters.join(u", ")));
        return;
    }
    m_fileDialog->SetFileTypeIndex(index + 1); // one-based.
}

QString QWindowsNativeFileDialogBase::selectedNameFilter() const
{
    UINT uIndex = 0;
    if (SUCCEEDED(m_fileDialog->GetFileTypeIndex(&uIndex))) {
        const int index = uIndex - 1; // one-based
        if (index < m_nameFilters.size())
            return m_nameFilters.at(index);
    }
    return QString();
}

void QWindowsNativeFileDialogBase::onFolderChange(IShellItem *item)
{
    if (item) {
        const QUrl directory = QWindowsShellItem(item).url();
        m_data.setDirectory(directory);
        emit directoryEntered(directory);
    }
}

void QWindowsNativeFileDialogBase::onSelectionChange()
{
    const QList<QUrl> current = selectedFiles();
    m_data.setSelectedFiles(current);
    qCDebug(lcQpaDialogs) << __FUNCTION__ << current << current.size();

    if (current.size() == 1)
        emit currentChanged(current.front());
}

void QWindowsNativeFileDialogBase::onTypeChange()
{
    const QString filter = selectedNameFilter();
    m_data.setSelectedNameFilter(filter);
    emit filterSelected(filter);
}

bool QWindowsNativeFileDialogBase::onFileOk()
{
    // Store selected files as GetResults() returns invalid data after the dialog closes.
    m_data.setSelectedFiles(dialogResult());
    return true;
}

void QWindowsNativeFileDialogBase::close()
{
    m_fileDialog->Close(S_OK);
    // IFileDialog::Close() does not work unless invoked from a callback.
    // Try to find the window and send it a WM_CLOSE in addition.
    const HWND hwnd = findDialogWindow(m_title);
    qCDebug(lcQpaDialogs) << __FUNCTION__ << "closing" << hwnd;
    if (hwnd && IsWindowVisible(hwnd))
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
}

HRESULT QWindowsNativeFileDialogEventHandler::OnFolderChanging(IFileDialog *, IShellItem *item)
{
    m_nativeFileDialog->onFolderChange(item);
    return S_OK;
}

HRESULT QWindowsNativeFileDialogEventHandler::OnSelectionChange(IFileDialog *)
{
    m_nativeFileDialog->onSelectionChange();
    return S_OK;
}

HRESULT QWindowsNativeFileDialogEventHandler::OnTypeChange(IFileDialog *)
{
    m_nativeFileDialog->onTypeChange();
    return S_OK;
}

HRESULT QWindowsNativeFileDialogEventHandler::OnFileOk(IFileDialog *)
{
    return m_nativeFileDialog->onFileOk() ? S_OK : S_FALSE;
}

/*!
    \class QWindowsNativeSaveFileDialog
    \brief Windows native file save dialog wrapper around IFileSaveDialog.

    Implements single-selection methods.

    \internal
*/

class QWindowsNativeSaveFileDialog : public QWindowsNativeFileDialogBase
{
    Q_OBJECT
public:
    explicit QWindowsNativeSaveFileDialog(const QWindowsFileDialogSharedData &data)
        : QWindowsNativeFileDialogBase(data) {}
    void setNameFilters(const QStringList &f) override;
    QList<QUrl> selectedFiles() const override;
    QList<QUrl> dialogResult() const override;
};

// Return the first suffix from the name filter "Foo files (*.foo;*.bar)" -> "foo".
// Also handles the simple name filter case "*.txt" -> "txt"
static inline QString suffixFromFilter(const QString &filter)
{
    int suffixPos = filter.indexOf(u"*.");
    if (suffixPos < 0)
        return QString();
    suffixPos += 2;
    int endPos = filter.indexOf(u' ', suffixPos + 1);
    if (endPos < 0)
        endPos = filter.indexOf(u';', suffixPos + 1);
    if (endPos < 0)
        endPos = filter.indexOf(u')', suffixPos + 1);
    if (endPos < 0)
        endPos = filter.size();
    return filter.mid(suffixPos, endPos - suffixPos);
}

void QWindowsNativeSaveFileDialog::setNameFilters(const QStringList &f)
{
    QWindowsNativeFileDialogBase::setNameFilters(f);
    // QTBUG-31381, QTBUG-30748: IFileDialog will update the suffix of the selected name
    // filter only if a default suffix is set (see docs). Set the first available
    // suffix unless we have a defaultSuffix.
    if (!hasDefaultSuffix()) {
        for (const QString &filter : f) {
            const QString suffix = suffixFromFilter(filter);
            if (!suffix.isEmpty()) {
                setDefaultSuffixSys(suffix);
                break;
            }
        }
    } // m_hasDefaultSuffix
}

QList<QUrl> QWindowsNativeSaveFileDialog::dialogResult() const
{
    QList<QUrl> result;
    IShellItem *item = nullptr;
    if (SUCCEEDED(fileDialog()->GetResult(&item)) && item)
        result.append(QWindowsShellItem(item).url());
    return result;
}

QList<QUrl> QWindowsNativeSaveFileDialog::selectedFiles() const
{
    QList<QUrl> result;
    IShellItem *item = nullptr;
    const HRESULT hr = fileDialog()->GetCurrentSelection(&item);
    if (SUCCEEDED(hr) && item) {
        result.append(QWindowsShellItem(item).url());
        item->Release();
    }
    return result;
}

/*!
    \class QWindowsNativeOpenFileDialog
    \brief Windows native file save dialog wrapper around IFileOpenDialog.

    Implements multi-selection methods.

    \internal
*/

class QWindowsNativeOpenFileDialog : public QWindowsNativeFileDialogBase
{
public:
    explicit QWindowsNativeOpenFileDialog(const QWindowsFileDialogSharedData &data) :
        QWindowsNativeFileDialogBase(data) {}
    QList<QUrl> selectedFiles() const override;
    QList<QUrl> dialogResult() const override;

private:
    inline IFileOpenDialog *openFileDialog() const
        { return static_cast<IFileOpenDialog *>(fileDialog()); }
};

// Helpers for managing a list of temporary copies of items with no
// file system representation (SFGAO_FILESYSTEM unset, for example devices
// using MTP) returned by IFileOpenDialog. This emulates the behavior
// of the Win32 API GetOpenFileName() used in Qt 4 (QTBUG-57070).

Q_GLOBAL_STATIC(QStringList, temporaryItemCopies)

static void cleanupTemporaryItemCopies()
{
    for (const QString &file : qAsConst(*temporaryItemCopies()))
        QFile::remove(file);
}

// Determine temporary file pattern from a shell item's display
// name. This can be a URL.

static bool validFileNameCharacter(QChar c)
{
    return c.isLetterOrNumber() || c == u'_' || c == u'-';
}

QString tempFilePattern(QString name)
{
    const int lastSlash = qMax(name.lastIndexOf(u'/'),
                               name.lastIndexOf(u'\\'));
    if (lastSlash != -1)
        name.remove(0, lastSlash + 1);

    int lastDot = name.lastIndexOf(u'.');
    if (lastDot < 0)
        lastDot = name.size();
    name.insert(lastDot, QStringLiteral("_XXXXXX"));

    for (int i = lastDot - 1; i >= 0; --i) {
        if (!validFileNameCharacter(name.at(i)))
            name[i] = u'_';
    }

    name.prepend(QDir::tempPath() + u'/');
    return name;
}

static QString createTemporaryItemCopy(QWindowsShellItem &qItem, QString *errorMessage)
{
    if (!qItem.canStream()) {
        *errorMessage = QLatin1String("Item not streamable");
        return QString();
    }

    QTemporaryFile targetFile(tempFilePattern(qItem.normalDisplay()));
    targetFile.setAutoRemove(false);
    if (!targetFile.open())  {
        *errorMessage = QLatin1String("Cannot create temporary file: ")
                        + targetFile.errorString();
        return QString();
    }
    if (!qItem.copyData(&targetFile, errorMessage))
        return QString();
    const QString result = targetFile.fileName();
    if (temporaryItemCopies()->isEmpty())
        qAddPostRoutine(cleanupTemporaryItemCopies);
    temporaryItemCopies()->append(result);
    return result;
}

static QUrl itemToDialogUrl(QWindowsShellItem &qItem, QString *errorMessage)
{
    QUrl url = qItem.url();
    if (url.isLocalFile() || url.scheme().startsWith(u"http"))
        return url;
    const QString path = qItem.path();
    if (path.isEmpty() && !qItem.isDir() && qItem.canStream()) {
        const QString temporaryCopy = createTemporaryItemCopy(qItem, errorMessage);
        if (temporaryCopy.isEmpty()) {
            QDebug(errorMessage).noquote() << "Unable to create a local copy of"
                << qItem << ": " << errorMessage;
            return QUrl();
        }
        return QUrl::fromLocalFile(temporaryCopy);
    }
    if (!url.isValid())
        QDebug(errorMessage).noquote() << "Invalid URL obtained from" << qItem;
    return url;
}

QList<QUrl> QWindowsNativeOpenFileDialog::dialogResult() const
{
    QList<QUrl> result;
    IShellItemArray *items = nullptr;
    if (SUCCEEDED(openFileDialog()->GetResults(&items)) && items) {
        QString errorMessage;
        for (IShellItem *item : QWindowsShellItem::itemsFromItemArray(items)) {
            QWindowsShellItem qItem(item);
            const QUrl url = itemToDialogUrl(qItem, &errorMessage);
            if (!url.isValid()) {
                qWarning("%s", qPrintable(errorMessage));
                result.clear();
                break;
            }
            result.append(url);
        }
    }
    return result;
}

QList<QUrl> QWindowsNativeOpenFileDialog::selectedFiles() const
{
    QList<QUrl> result;
    IShellItemArray *items = nullptr;
    const HRESULT hr = openFileDialog()->GetSelectedItems(&items);
    if (SUCCEEDED(hr) && items) {
        for (IShellItem *item : QWindowsShellItem::itemsFromItemArray(items)) {
            const QWindowsShellItem qItem(item);
            const QUrl url = qItem.url();
            if (url.isValid())
                result.append(url);
            else
                qWarning().nospace() << __FUNCTION__<< ": Unable to obtain URL of " << qItem;
        }
    }
    return result;
}

/*!
    \brief Factory method for QWindowsNativeFileDialogBase returning
    QWindowsNativeOpenFileDialog or QWindowsNativeSaveFileDialog depending on
    QFileDialog::AcceptMode.
*/

QWindowsNativeFileDialogBase *QWindowsNativeFileDialogBase::create(QFileDialogOptions::AcceptMode am,
                                                                   const QWindowsFileDialogSharedData &data)
{
    QWindowsNativeFileDialogBase *result = nullptr;
    if (am == QFileDialogOptions::AcceptOpen) {
        result = new QWindowsNativeOpenFileDialog(data);
        if (!result->init(CLSID_FileOpenDialog, IID_IFileOpenDialog)) {
            delete result;
            return nullptr;
        }
    } else {
        result = new QWindowsNativeSaveFileDialog(data);
        if (!result->init(CLSID_FileSaveDialog, IID_IFileSaveDialog)) {
            delete result;
            return nullptr;
        }
    }
    return result;
}

/*!
    \class QWindowsFileDialogHelper
    \brief Helper for native Windows file dialogs

    For Qt 4 compatibility, do not create native non-modal dialogs on widgets,
    but only on QQuickWindows, which do not have a fallback.

    \internal
*/

class QWindowsFileDialogHelper : public QWindowsDialogHelperBase<QPlatformFileDialogHelper>
{
public:
    QWindowsFileDialogHelper() {}
    bool supportsNonModalDialog(const QWindow * /* parent */ = nullptr) const override { return false; }
    bool defaultNameFilterDisables() const override
        { return false; }
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &filename) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;

private:
    QWindowsNativeDialogBase *createNativeDialog() override;
    inline QWindowsNativeFileDialogBase *nativeFileDialog() const
        { return static_cast<QWindowsNativeFileDialogBase *>(nativeDialog()); }

    // Cache for the case no native dialog is created.
    QWindowsFileDialogSharedData m_data;
};

QWindowsNativeDialogBase *QWindowsFileDialogHelper::createNativeDialog()
{
    QWindowsNativeFileDialogBase *result = QWindowsNativeFileDialogBase::create(options()->acceptMode(), m_data);
    if (!result)
        return nullptr;
    QObject::connect(result, &QWindowsNativeDialogBase::accepted, this, &QPlatformDialogHelper::accept);
    QObject::connect(result, &QWindowsNativeDialogBase::rejected, this, &QPlatformDialogHelper::reject);
    QObject::connect(result, &QWindowsNativeFileDialogBase::directoryEntered,
                     this, &QPlatformFileDialogHelper::directoryEntered);
    QObject::connect(result, &QWindowsNativeFileDialogBase::currentChanged,
                     this, &QPlatformFileDialogHelper::currentChanged);
    QObject::connect(result, &QWindowsNativeFileDialogBase::filterSelected,
                     this, &QPlatformFileDialogHelper::filterSelected);

    // Apply settings.
    const QSharedPointer<QFileDialogOptions> &opts = options();
    m_data.fromOptions(opts);
    const QFileDialogOptions::FileMode mode = opts->fileMode();
    result->setWindowTitle(opts->windowTitle());
    result->setMode(mode, opts->acceptMode(), opts->options());
    result->setHideFiltersDetails(opts->testOption(QFileDialogOptions::HideNameFilterDetails));
    const QStringList nameFilters = opts->nameFilters();
    if (!nameFilters.isEmpty())
        result->setNameFilters(nameFilters);
    if (opts->isLabelExplicitlySet(QFileDialogOptions::FileName))
        result->setLabelText(QFileDialogOptions::FileName, opts->labelText(QFileDialogOptions::FileName));
    if (opts->isLabelExplicitlySet(QFileDialogOptions::Accept))
        result->setLabelText(QFileDialogOptions::Accept, opts->labelText(QFileDialogOptions::Accept));
    if (opts->isLabelExplicitlySet(QFileDialogOptions::Reject))
        result->setLabelText(QFileDialogOptions::Reject, opts->labelText(QFileDialogOptions::Reject));
    result->updateDirectory();
    result->updateSelectedNameFilter();
    const QList<QUrl> initialSelection = opts->initiallySelectedFiles();
    if (!initialSelection.empty()) {
        const QUrl &url = initialSelection.constFirst();
        if (url.isLocalFile()) {
            QFileInfo info(url.toLocalFile());
            if (!info.isDir())
                result->selectFile(info.fileName());
        } else {
            result->selectFile(url.path()); // TODO url.fileName() once it exists
        }
    }
    // No need to select initialNameFilter if mode is Dir
    if (mode != QFileDialogOptions::Directory && mode != QFileDialogOptions::DirectoryOnly) {
        const QString initialNameFilter = opts->initiallySelectedNameFilter();
        if (!initialNameFilter.isEmpty())
            result->selectNameFilter(initialNameFilter);
    }
    const QString defaultSuffix = opts->defaultSuffix();
    if (!defaultSuffix.isEmpty())
        result->setDefaultSuffix(defaultSuffix);
    return result;
}

void QWindowsFileDialogHelper::setDirectory(const QUrl &directory)
{
    qCDebug(lcQpaDialogs) << __FUNCTION__ << directory.toString();

    m_data.setDirectory(directory);
    if (hasNativeDialog())
        nativeFileDialog()->updateDirectory();
}

QUrl QWindowsFileDialogHelper::directory() const
{
    return m_data.directory();
}

void QWindowsFileDialogHelper::selectFile(const QUrl &fileName)
{
    qCDebug(lcQpaDialogs) << __FUNCTION__ << fileName.toString();

    if (hasNativeDialog()) // Might be invoked from the QFileDialog constructor.
        nativeFileDialog()->selectFile(fileName.toLocalFile()); // ## should use QUrl::fileName() once it exists
}

QList<QUrl> QWindowsFileDialogHelper::selectedFiles() const
{
    return m_data.selectedFiles();
}

void QWindowsFileDialogHelper::setFilter()
{
    qCDebug(lcQpaDialogs) << __FUNCTION__;
}

void QWindowsFileDialogHelper::selectNameFilter(const QString &filter)
{
    m_data.setSelectedNameFilter(filter);
    if (hasNativeDialog())
        nativeFileDialog()->updateSelectedNameFilter();
}

QString QWindowsFileDialogHelper::selectedNameFilter() const
{
    return m_data.selectedNameFilter();
}

/*!
    \class QWindowsXpNativeFileDialog
    \brief Native Windows directory dialog for Windows XP using SHlib-functions.

    Uses the synchronous GetOpenFileNameW(), GetSaveFileNameW() from ComDlg32
    or SHBrowseForFolder() for directories.

    \internal
    \sa QWindowsXpFileDialogHelper

*/

class QWindowsXpNativeFileDialog : public QWindowsNativeDialogBase
{
    Q_OBJECT
public:
    using OptionsPtr = QSharedPointer<QFileDialogOptions>;

    static QWindowsXpNativeFileDialog *create(const OptionsPtr &options, const QWindowsFileDialogSharedData &data);

    void setWindowTitle(const QString &t) override { m_title =  t; }
    void doExec(HWND owner = nullptr) override;

    int existingDirCallback(HWND hwnd, UINT uMsg, LPARAM lParam);

public slots:
    void close() override {}

private:
    typedef BOOL (APIENTRY *PtrGetOpenFileNameW)(LPOPENFILENAMEW);
    typedef BOOL (APIENTRY *PtrGetSaveFileNameW)(LPOPENFILENAMEW);

    explicit QWindowsXpNativeFileDialog(const OptionsPtr &options, const QWindowsFileDialogSharedData &data);
    void populateOpenFileName(OPENFILENAME *ofn, HWND owner) const;
    QList<QUrl> execExistingDir(HWND owner);
    QList<QUrl> execFileNames(HWND owner, int *selectedFilterIndex) const;

    const OptionsPtr m_options;
    QString m_title;
    QPlatformDialogHelper::DialogCode m_result;
    QWindowsFileDialogSharedData m_data;

    static PtrGetOpenFileNameW m_getOpenFileNameW;
    static PtrGetSaveFileNameW m_getSaveFileNameW;
};

QWindowsXpNativeFileDialog::PtrGetOpenFileNameW QWindowsXpNativeFileDialog::m_getOpenFileNameW = nullptr;
QWindowsXpNativeFileDialog::PtrGetSaveFileNameW QWindowsXpNativeFileDialog::m_getSaveFileNameW = nullptr;

QWindowsXpNativeFileDialog *QWindowsXpNativeFileDialog::create(const OptionsPtr &options, const QWindowsFileDialogSharedData &data)
{
    // GetOpenFileNameW() GetSaveFileName() are resolved
    // dynamically as not to create a dependency on Comdlg32, which
    // is used on XP only.
    if (!m_getOpenFileNameW) {
        QSystemLibrary library(QStringLiteral("Comdlg32"));
        m_getOpenFileNameW = (PtrGetOpenFileNameW)(library.resolve("GetOpenFileNameW"));
        m_getSaveFileNameW = (PtrGetSaveFileNameW)(library.resolve("GetSaveFileNameW"));
    }
    if (m_getOpenFileNameW && m_getSaveFileNameW)
        return new QWindowsXpNativeFileDialog(options, data);
    return nullptr;
}

QWindowsXpNativeFileDialog::QWindowsXpNativeFileDialog(const OptionsPtr &options,
                                                       const QWindowsFileDialogSharedData &data) :
    m_options(options), m_result(QPlatformDialogHelper::Rejected), m_data(data)
{
    setWindowTitle(m_options->windowTitle());
}

void QWindowsXpNativeFileDialog::doExec(HWND owner)
{
    int selectedFilterIndex = -1;
    const QList<QUrl> selectedFiles =
        m_options->fileMode() == QFileDialogOptions::DirectoryOnly ?
        execExistingDir(owner) : execFileNames(owner, &selectedFilterIndex);
    m_data.setSelectedFiles(selectedFiles);
    QWindowsDialogs::eatMouseMove();
    if (selectedFiles.isEmpty()) {
        m_result = QPlatformDialogHelper::Rejected;
        emit rejected();
    } else {
        const QStringList nameFilters = m_options->nameFilters();
        if (selectedFilterIndex >= 0 && selectedFilterIndex < nameFilters.size())
            m_data.setSelectedNameFilter(nameFilters.at(selectedFilterIndex));
        const QUrl &firstFile = selectedFiles.constFirst();
        m_data.setDirectory(firstFile.adjusted(QUrl::RemoveFilename));
        m_result = QPlatformDialogHelper::Accepted;
        emit accepted();
    }
}

// Callback for QWindowsNativeXpFileDialog directory dialog.
// MFC Directory Dialog. Contrib: Steve Williams (minor parts from Scott Powers)

static int QT_WIN_CALLBACK xpFileDialogGetExistingDirCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    auto *dialog = reinterpret_cast<QWindowsXpNativeFileDialog *>(lpData);
    return dialog->existingDirCallback(hwnd, uMsg, lParam);
}

/* The correct declaration of the SHGetPathFromIDList symbol is
 * being used in mingw-w64 as of r6215, which is a v3 snapshot.  */
#if defined(Q_CC_MINGW) && (!defined(__MINGW64_VERSION_MAJOR) || __MINGW64_VERSION_MAJOR < 3)
typedef ITEMIDLIST *qt_LpItemIdList;
#else
using qt_LpItemIdList = PIDLIST_ABSOLUTE;
#endif

int QWindowsXpNativeFileDialog::existingDirCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    switch (uMsg) {
    case BFFM_INITIALIZED: {
        if (!m_title.isEmpty())
            SetWindowText(hwnd, reinterpret_cast<const wchar_t *>(m_title.utf16()));
        const QString initialFile = QDir::toNativeSeparators(m_data.directory().toLocalFile());
        if (!initialFile.isEmpty())
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, LPARAM(initialFile.utf16()));
    }
        break;
    case BFFM_SELCHANGED: {
        wchar_t path[MAX_PATH];
        const bool ok = SHGetPathFromIDList(reinterpret_cast<qt_LpItemIdList>(lParam), path)
                        && path[0];
        SendMessage(hwnd, BFFM_ENABLEOK, ok ? 1 : 0, 1);
    }
        break;
    }
    return 0;
}

QList<QUrl> QWindowsXpNativeFileDialog::execExistingDir(HWND owner)
{
    BROWSEINFO bi;
    wchar_t initPath[MAX_PATH];
    initPath[0] = 0;
    bi.hwndOwner = owner;
    bi.pidlRoot = nullptr;
    bi.lpszTitle = nullptr;
    bi.pszDisplayName = initPath;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_NEWDIALOGSTYLE;
    bi.lpfn = xpFileDialogGetExistingDirCallbackProc;
    bi.lParam = LPARAM(this);
    QList<QUrl> selectedFiles;
    if (qt_LpItemIdList pItemIDList = SHBrowseForFolder(&bi)) {
        wchar_t path[MAX_PATH];
        path[0] = 0;
        if (SHGetPathFromIDList(pItemIDList, path) && path[0])
            selectedFiles.push_back(QUrl::fromLocalFile(QDir::cleanPath(QString::fromWCharArray(path))));
        IMalloc *pMalloc;
        if (SHGetMalloc(&pMalloc) == NOERROR) {
            pMalloc->Free(pItemIDList);
            pMalloc->Release();
        }
    }
    return selectedFiles;
}

// Open/Save files
void QWindowsXpNativeFileDialog::populateOpenFileName(OPENFILENAME *ofn, HWND owner) const
{
    ZeroMemory(ofn, sizeof(OPENFILENAME));
    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->hwndOwner = owner;

    // Create a buffer with the filter strings.
    int totalStringLength = 0;
    const QList<FilterSpec> specs =
        filterSpecs(m_options->nameFilters(), m_options->options() & QFileDialogOptions::HideNameFilterDetails, &totalStringLength);
    const int size = specs.size();
    auto *ptr = new wchar_t[totalStringLength + 2 * size + 1];
    ofn->lpstrFilter = ptr;
    for (const FilterSpec &spec : specs) {
        ptr += spec.description.toWCharArray(ptr);
        *ptr++ = 0;
        ptr += spec.filter.toWCharArray(ptr);
        *ptr++ = 0;
    }
    *ptr = 0;
    const int nameFilterIndex = indexOfNameFilter(m_options->nameFilters(), m_data.selectedNameFilter());
    if (nameFilterIndex >= 0)
        ofn->nFilterIndex = nameFilterIndex + 1; // 1..n based.
    // lpstrFile receives the initial selection and is the buffer
    // for the target. If it contains any invalid character, the dialog
    // will not show.
    ofn->nMaxFile = 65535;
    QString initiallySelectedFile = m_data.selectedFile();
    initiallySelectedFile.remove(u'<');
    initiallySelectedFile.remove(u'>');
    initiallySelectedFile.remove(u'"');
    initiallySelectedFile.remove(u'|');
    ofn->lpstrFile = qStringToWCharArray(QDir::toNativeSeparators(initiallySelectedFile), ofn->nMaxFile);
    ofn->lpstrInitialDir = qStringToWCharArray(QDir::toNativeSeparators(m_data.directory().toLocalFile()));
    ofn->lpstrTitle = (wchar_t*)m_title.utf16();
    // Determine lpstrDefExt. Note that the current MSDN docs document this
    // member wrong. It should rather be documented as "the default extension
    // if no extension was given and if the current filter does not have an
    // extension (e.g (*)). If the current filter has an extension, use
    // the extension of the current filter".
    if (m_options->acceptMode() == QFileDialogOptions::AcceptSave) {
        QString defaultSuffix = m_options->defaultSuffix();
        if (defaultSuffix.startsWith(u'.'))
            defaultSuffix.remove(0, 1);
        // QTBUG-33156, also create empty strings to trigger the appending mechanism.
        ofn->lpstrDefExt = qStringToWCharArray(defaultSuffix);
    }
    // Flags.
    ofn->Flags = (OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST);
    if (m_options->fileMode() == QFileDialogOptions::ExistingFile
        || m_options->fileMode() == QFileDialogOptions::ExistingFiles)
        ofn->Flags |= (OFN_FILEMUSTEXIST);
    if (m_options->fileMode() == QFileDialogOptions::ExistingFiles)
        ofn->Flags |= (OFN_ALLOWMULTISELECT);
    if (!(m_options->options() & QFileDialogOptions::DontConfirmOverwrite))
        ofn->Flags |= OFN_OVERWRITEPROMPT;
}

QList<QUrl> QWindowsXpNativeFileDialog::execFileNames(HWND owner, int *selectedFilterIndex) const
{
    *selectedFilterIndex = -1;
    OPENFILENAME ofn;
    populateOpenFileName(&ofn, owner);
    QList<QUrl> result;
    const bool isSave = m_options->acceptMode() == QFileDialogOptions::AcceptSave;
    if (isSave ? m_getSaveFileNameW(&ofn) : m_getOpenFileNameW(&ofn)) {
        *selectedFilterIndex = ofn.nFilterIndex - 1;
        const QString dir = QDir::cleanPath(QString::fromWCharArray(ofn.lpstrFile));
        result.push_back(QUrl::fromLocalFile(dir));
        // For multiselection, the first item is the path followed
        // by "\0<file1>\0<file2>\0\0".
        if (ofn.Flags & (OFN_ALLOWMULTISELECT)) {
            wchar_t *ptr = ofn.lpstrFile + dir.size() + 1;
            if (*ptr) {
                result.pop_front();
                const QString path = dir + u'/';
                while (*ptr) {
                    const QString fileName = QString::fromWCharArray(ptr);
                    result.push_back(QUrl::fromLocalFile(path + fileName));
                    ptr += fileName.size() + 1;
                } // extract multiple files
            } // has multiple files
        } // multiple flag set
    }
    delete [] ofn.lpstrFile;
    delete [] ofn.lpstrInitialDir;
    delete [] ofn.lpstrFilter;
    delete [] ofn.lpstrDefExt;
    return result;
}

/*!
    \class QWindowsXpFileDialogHelper
    \brief Dialog helper using QWindowsXpNativeFileDialog

    \sa QWindowsXpNativeFileDialog
    \internal
*/

class QWindowsXpFileDialogHelper : public QWindowsDialogHelperBase<QPlatformFileDialogHelper>
{
public:
    QWindowsXpFileDialogHelper() = default;
    bool supportsNonModalDialog(const QWindow * /* parent */ = nullptr) const override { return false; }
    bool defaultNameFilterDisables() const override
        { return true; }
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &url) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override {}
    void selectNameFilter(const QString &) override;
    QString selectedNameFilter() const override;

private:
    QWindowsNativeDialogBase *createNativeDialog() override;
    inline QWindowsXpNativeFileDialog *nativeFileDialog() const
        { return static_cast<QWindowsXpNativeFileDialog *>(nativeDialog()); }

    QWindowsFileDialogSharedData m_data;
};

QWindowsNativeDialogBase *QWindowsXpFileDialogHelper::createNativeDialog()
{
    m_data.fromOptions(options());
    if (QWindowsXpNativeFileDialog *result = QWindowsXpNativeFileDialog::create(options(), m_data)) {
        QObject::connect(result, &QWindowsNativeDialogBase::accepted, this, &QPlatformDialogHelper::accept);
        QObject::connect(result, &QWindowsNativeDialogBase::rejected, this, &QPlatformDialogHelper::reject);
        return result;
    }
    return nullptr;
}

void QWindowsXpFileDialogHelper::setDirectory(const QUrl &directory)
{
    m_data.setDirectory(directory); // Dialog cannot be updated at run-time.
}

QUrl QWindowsXpFileDialogHelper::directory() const
{
    return m_data.directory();
}

void QWindowsXpFileDialogHelper::selectFile(const QUrl &url)
{
    m_data.setSelectedFiles(QList<QUrl>() << url); // Dialog cannot be updated at run-time.
}

QList<QUrl> QWindowsXpFileDialogHelper::selectedFiles() const
{
    return m_data.selectedFiles();
}

void QWindowsXpFileDialogHelper::selectNameFilter(const QString &f)
{
    m_data.setSelectedNameFilter(f); // Dialog cannot be updated at run-time.
}

QString QWindowsXpFileDialogHelper::selectedNameFilter() const
{
    return m_data.selectedNameFilter();
}

/*!
    \class QWindowsNativeColorDialog
    \brief Native Windows color dialog.

    Wrapper around Comdlg32's ChooseColor() function.
    Not currently in use as QColorDialog is equivalent.

    \sa QWindowsColorDialogHelper
    \sa #define USE_NATIVE_COLOR_DIALOG
    \internal
*/

using SharedPointerColor = QSharedPointer<QColor>;

#ifdef USE_NATIVE_COLOR_DIALOG
class QWindowsNativeColorDialog : public QWindowsNativeDialogBase
{
    Q_OBJECT
public:
    enum { CustomColorCount = 16 };

    explicit QWindowsNativeColorDialog(const SharedPointerColor &color);

    void setWindowTitle(const QString &) override {}

public slots:
    void close() override {}

private:
    void doExec(HWND owner = 0) override;

    COLORREF m_customColors[CustomColorCount];
    QPlatformDialogHelper::DialogCode m_code;
    SharedPointerColor m_color;
};

QWindowsNativeColorDialog::QWindowsNativeColorDialog(const SharedPointerColor &color) :
    m_code(QPlatformDialogHelper::Rejected), m_color(color)
{
    std::fill(m_customColors, m_customColors + 16, COLORREF(0));
}

void QWindowsNativeColorDialog::doExec(HWND owner)
{
    typedef BOOL (WINAPI *ChooseColorWType)(LPCHOOSECOLORW);

    CHOOSECOLOR chooseColor;
    ZeroMemory(&chooseColor, sizeof(chooseColor));
    chooseColor.lStructSize = sizeof(chooseColor);
    chooseColor.hwndOwner = owner;
    chooseColor.lpCustColors = m_customColors;
    QRgb *qCustomColors = QColorDialogOptions::customColors();
    const int customColorCount = qMin(QColorDialogOptions::customColorCount(),
                                      int(CustomColorCount));
    for (int c= 0; c < customColorCount; ++c)
        m_customColors[c] = qColorToCOLORREF(QColor(qCustomColors[c]));
    chooseColor.rgbResult = qColorToCOLORREF(*m_color);
    chooseColor.Flags = CC_FULLOPEN | CC_RGBINIT;
    static ChooseColorWType chooseColorW = 0;
    if (!chooseColorW) {
        QSystemLibrary library(QStringLiteral("Comdlg32"));
        chooseColorW = (ChooseColorWType)library.resolve("ChooseColorW");
    }
    if (chooseColorW) {
        m_code = chooseColorW(&chooseColor) ?
            QPlatformDialogHelper::Accepted : QPlatformDialogHelper::Rejected;
        QWindowsDialogs::eatMouseMove();
    } else {
        m_code = QPlatformDialogHelper::Rejected;
    }
    if (m_code == QPlatformDialogHelper::Accepted) {
        *m_color = COLORREFToQColor(chooseColor.rgbResult);
        for (int c= 0; c < customColorCount; ++c)
            qCustomColors[c] = COLORREFToQColor(m_customColors[c]).rgb();
        emit accepted();
    } else {
        emit rejected();
    }
}

/*!
    \class QWindowsColorDialogHelper
    \brief Helper for native Windows color dialogs

    Not currently in use as QColorDialog is equivalent.

    \sa #define USE_NATIVE_COLOR_DIALOG
    \sa QWindowsNativeColorDialog
    \internal
*/

class QWindowsColorDialogHelper : public QWindowsDialogHelperBase<QPlatformColorDialogHelper>
{
public:
    QWindowsColorDialogHelper() : m_currentColor(new QColor) {}

    virtual bool supportsNonModalDialog()
        { return false; }

    virtual QColor currentColor() const { return *m_currentColor; }
    virtual void setCurrentColor(const QColor &c) { *m_currentColor = c; }

private:
    inline QWindowsNativeColorDialog *nativeFileDialog() const
        { return static_cast<QWindowsNativeColorDialog *>(nativeDialog()); }
    virtual QWindowsNativeDialogBase *createNativeDialog();

    SharedPointerColor m_currentColor;
};

QWindowsNativeDialogBase *QWindowsColorDialogHelper::createNativeDialog()
{
    QWindowsNativeColorDialog *nativeDialog = new QWindowsNativeColorDialog(m_currentColor);
    nativeDialog->setWindowTitle(options()->windowTitle());
    connect(nativeDialog, &QWindowsNativeDialogBase::accepted, this, &QPlatformDialogHelper::accept);
    connect(nativeDialog, &QWindowsNativeDialogBase::rejected, this, &QPlatformDialogHelper::reject);
    return nativeDialog;
}
#endif // USE_NATIVE_COLOR_DIALOG

namespace QWindowsDialogs {

// QWindowsDialogHelperBase creation functions
bool useHelper(QPlatformTheme::DialogType type)
{
    if (QWindowsIntegration::instance()->options() & QWindowsIntegration::NoNativeDialogs)
        return false;
    switch (type) {
    case QPlatformTheme::FileDialog:
        return true;
    case QPlatformTheme::ColorDialog:
#ifdef USE_NATIVE_COLOR_DIALOG
        return true;
#else
        break;
#endif
    case QPlatformTheme::FontDialog:
    case QPlatformTheme::MessageDialog:
        break;
    default:
        break;
    }
    return false;
}

QPlatformDialogHelper *createHelper(QPlatformTheme::DialogType type)
{
    if (QWindowsIntegration::instance()->options() & QWindowsIntegration::NoNativeDialogs)
        return nullptr;
    switch (type) {
    case QPlatformTheme::FileDialog:
        if (QWindowsIntegration::instance()->options() & QWindowsIntegration::XpNativeDialogs)
            return new QWindowsXpFileDialogHelper();
        return new QWindowsFileDialogHelper;
    case QPlatformTheme::ColorDialog:
#ifdef USE_NATIVE_COLOR_DIALOG
        return new QWindowsColorDialogHelper();
#else
        break;
#endif
    case QPlatformTheme::FontDialog:
    case QPlatformTheme::MessageDialog:
        break;
    default:
        break;
    }
    return nullptr;
}

} // namespace QWindowsDialogs
QT_END_NAMESPACE

#include "qwindowsdialoghelpers.moc"
