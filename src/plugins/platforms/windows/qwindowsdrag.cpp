/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsdrag.h"
#include "qwindowscontext.h"
#ifndef QT_NO_CLIPBOARD
#  include "qwindowsclipboard.h"
#endif
#include "qwindowsintegration.h"
#include "qwindowsole.h"
#include "qtwindows_additional.h"
#include "qwindowswindow.h"
#include "qwindowsmousehandler.h"
#include "qwindowscursor.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QGuiApplication>
#include <qpa/qwindowsysteminterface_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtCore/QDebug>
#include <QtCore/QBuffer>
#include <QtCore/QPoint>

#include <shlobj.h>

QT_BEGIN_NAMESPACE

// These pixmaps approximate the images in the Windows User Interface Guidelines.
// XPM

static const char * const moveDragCursorXpmC[] = {
"11 20 3 1",
".        c None",
"a        c #FFFFFF",
"X        c #000000", // X11 cursor is traditionally black
"aa.........",
"aXa........",
"aXXa.......",
"aXXXa......",
"aXXXXa.....",
"aXXXXXa....",
"aXXXXXXa...",
"aXXXXXXXa..",
"aXXXXXXXXa.",
"aXXXXXXXXXa",
"aXXXXXXaaaa",
"aXXXaXXa...",
"aXXaaXXa...",
"aXa..aXXa..",
"aa...aXXa..",
"a.....aXXa.",
"......aXXa.",
".......aXXa",
".......aXXa",
"........aa."};


/* XPM */
static const char * const copyDragCursorXpmC[] = {
"24 30 3 1",
".        c None",
"a        c #000000",
"X        c #FFFFFF",
"XX......................",
"XaX.....................",
"XaaX....................",
"XaaaX...................",
"XaaaaX..................",
"XaaaaaX.................",
"XaaaaaaX................",
"XaaaaaaaX...............",
"XaaaaaaaaX..............",
"XaaaaaaaaaX.............",
"XaaaaaaXXXX.............",
"XaaaXaaX................",
"XaaXXaaX................",
"XaX..XaaX...............",
"XX...XaaX...............",
"X.....XaaX..............",
"......XaaX..............",
".......XaaX.............",
".......XaaX.............",
"........XX...aaaaaaaaaaa",
".............aXXXXXXXXXa",
".............aXXXXXXXXXa",
".............aXXXXaXXXXa",
".............aXXXXaXXXXa",
".............aXXaaaaaXXa",
".............aXXXXaXXXXa",
".............aXXXXaXXXXa",
".............aXXXXXXXXXa",
".............aXXXXXXXXXa",
".............aaaaaaaaaaa"};

/* XPM */
static const char * const linkDragCursorXpmC[] = {
"24 30 3 1",
".        c None",
"a        c #000000",
"X        c #FFFFFF",
"XX......................",
"XaX.....................",
"XaaX....................",
"XaaaX...................",
"XaaaaX..................",
"XaaaaaX.................",
"XaaaaaaX................",
"XaaaaaaaX...............",
"XaaaaaaaaX..............",
"XaaaaaaaaaX.............",
"XaaaaaaXXXX.............",
"XaaaXaaX................",
"XaaXXaaX................",
"XaX..XaaX...............",
"XX...XaaX...............",
"X.....XaaX..............",
"......XaaX..............",
".......XaaX.............",
".......XaaX.............",
"........XX...aaaaaaaaaaa",
".............aXXXXXXXXXa",
".............aXXXaaaaXXa",
".............aXXXXaaaXXa",
".............aXXXaaaaXXa",
".............aXXaaaXaXXa",
".............aXXaaXXXXXa",
".............aXXaXXXXXXa",
".............aXXXaXXXXXa",
".............aXXXXXXXXXa",
".............aaaaaaaaaaa"};

static const char * const ignoreDragCursorXpmC[] = {
"24 30 3 1",
".        c None",
"a        c #000000",
"X        c #FFFFFF",
"aa......................",
"aXa.....................",
"aXXa....................",
"aXXXa...................",
"aXXXXa..................",
"aXXXXXa.................",
"aXXXXXXa................",
"aXXXXXXXa...............",
"aXXXXXXXXa..............",
"aXXXXXXXXXa.............",
"aXXXXXXaaaa.............",
"aXXXaXXa................",
"aXXaaXXa................",
"aXa..aXXa...............",
"aa...aXXa...............",
"a.....aXXa..............",
"......aXXa.....XXXX.....",
".......aXXa..XXaaaaXX...",
".......aXXa.XaaaaaaaaX..",
"........aa.XaaaXXXXaaaX.",
"...........XaaaaX..XaaX.",
"..........XaaXaaaX..XaaX",
"..........XaaXXaaaX.XaaX",
"..........XaaX.XaaaXXaaX",
"..........XaaX..XaaaXaaX",
"...........XaaX..XaaaaX.",
"...........XaaaXXXXaaaX.",
"............XaaaaaaaaX..",
".............XXaaaaXX...",
"...............XXXX....."};

/*!
    \class QWindowsDropMimeData
    \brief Special mime data class for data retrieval from Drag operations.

    Implementation of QWindowsInternalMimeDataBase which retrieves the
    current drop data object from QWindowsDrag.

    \sa QWindowsDrag
    \internal
    \ingroup qt-lighthouse-win
*/

IDataObject *QWindowsDropMimeData::retrieveDataObject() const
{
    return QWindowsDrag::instance()->dropDataObject();
}

static inline Qt::DropActions translateToQDragDropActions(DWORD pdwEffects)
{
    Qt::DropActions actions = Qt::IgnoreAction;
    if (pdwEffects & DROPEFFECT_LINK)
        actions |= Qt::LinkAction;
    if (pdwEffects & DROPEFFECT_COPY)
        actions |= Qt::CopyAction;
    if (pdwEffects & DROPEFFECT_MOVE)
        actions |= Qt::MoveAction;
    return actions;
}

static inline Qt::DropAction translateToQDragDropAction(DWORD pdwEffect)
{
    if (pdwEffect & DROPEFFECT_LINK)
        return Qt::LinkAction;
    if (pdwEffect & DROPEFFECT_COPY)
        return Qt::CopyAction;
    if (pdwEffect & DROPEFFECT_MOVE)
        return Qt::MoveAction;
    return Qt::IgnoreAction;
}

static inline DWORD translateToWinDragEffects(Qt::DropActions action)
{
    DWORD effect = DROPEFFECT_NONE;
    if (action & Qt::LinkAction)
        effect |= DROPEFFECT_LINK;
    if (action & Qt::CopyAction)
        effect |= DROPEFFECT_COPY;
    if (action & Qt::MoveAction)
        effect |= DROPEFFECT_MOVE;
    return effect;
}

static inline Qt::KeyboardModifiers toQtKeyboardModifiers(DWORD keyState)
{
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if (keyState & MK_SHIFT)
        modifiers |= Qt::ShiftModifier;
    if (keyState & MK_CONTROL)
        modifiers |= Qt::ControlModifier;
    if (keyState & MK_ALT)
        modifiers |= Qt::AltModifier;

    return modifiers;
}

/*!
    \class QWindowsOleDropSource
    \brief Implementation of IDropSource

    Used for drag operations.

    \sa QWindowsDrag
    \internal
    \ingroup qt-lighthouse-win
*/

class QWindowsOleDropSource : public IDropSource
{
public:
    explicit QWindowsOleDropSource(QWindowsDrag *drag);
    virtual ~QWindowsOleDropSource();

    void createCursors();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IDropSource methods
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

private:
    class DragCursorHandle {
        Q_DISABLE_COPY(DragCursorHandle)
    public:
        DragCursorHandle(HCURSOR c, qint64 k) :  cursor(c), cacheKey(k) {}
        ~DragCursorHandle() { DestroyCursor(cursor); }
        HCURSOR cursor;
        qint64 cacheKey;
    };
    typedef QMap <Qt::DropAction, QSharedPointer<DragCursorHandle> > ActionCursorMap;

    QWindowsDrag *m_drag;
    Qt::MouseButtons m_currentButtons;
    ActionCursorMap m_cursors;

    ULONG m_refs;
};

QWindowsOleDropSource::QWindowsOleDropSource(QWindowsDrag *drag) :
    m_drag(drag), m_currentButtons(Qt::NoButton),
    m_refs(1)
{
    if (QWindowsContext::verboseOLE)
        qDebug("%s", __FUNCTION__);
}

QWindowsOleDropSource::~QWindowsOleDropSource()
{
    m_cursors.clear();
    if (QWindowsContext::verboseOLE)
        qDebug("%s", __FUNCTION__);
}

/*!
    \brief Blend custom pixmap with cursors.
*/

void QWindowsOleDropSource::createCursors()
{
    const QDrag *drag = m_drag->currentDrag();
    const QPixmap pixmap = drag->pixmap();
    const bool hasPixmap = !pixmap.isNull();

    QList<Qt::DropAction> actions;
    actions << Qt::MoveAction << Qt::CopyAction << Qt::LinkAction;
    if (hasPixmap)
        actions << Qt::IgnoreAction;
    const QPoint hotSpot = drag->hotSpot();
    for (int cnum = 0; cnum < actions.size(); ++cnum) {
        const Qt::DropAction action = actions.at(cnum);
        QPixmap cpm = drag->dragCursor(action);
        if (cpm.isNull())
            cpm = m_drag->defaultCursor(action);
        QSharedPointer<DragCursorHandle> cursorHandler = m_cursors.value(action);
        if (!cursorHandler.isNull() && cpm.cacheKey() == cursorHandler->cacheKey)
            continue;
        if (cpm.isNull()) {
            qWarning("%s: Unable to obtain drag cursor for %d.", __FUNCTION__, action);
            continue;
        }

        int w = cpm.width();
        int h = cpm.height();

        if (hasPixmap) {
            const int x1 = qMin(-hotSpot.x(), 0);
            const int x2 = qMax(pixmap.width() - hotSpot.x(), cpm.width());
            const int y1 = qMin(-hotSpot.y(), 0);
            const int y2 = qMax(pixmap.height() - hotSpot.y(), cpm.height());

            w = x2 - x1 + 1;
            h = y2 - y1 + 1;
        }

        const QPoint newHotSpot = hotSpot;
        QPixmap newCursor(w, h);
        if (hasPixmap) {
            newCursor.fill(Qt::transparent);
            QPainter p(&newCursor);
            const QRect srcRect = pixmap.rect();
            const QPoint pmDest = QPoint(qMax(0, -hotSpot.x()), qMax(0, -hotSpot.y()));
            p.drawPixmap(pmDest, pixmap, srcRect);
            p.drawPixmap(qMax(0,newHotSpot.x()),qMax(0,newHotSpot.y()),cpm);
        } else {
            newCursor = cpm;
        }

        const int hotX = hasPixmap ? qMax(0,newHotSpot.x()) : 0;
        const int hotY = hasPixmap ? qMax(0,newHotSpot.y()) : 0;

        if (const HCURSOR sysCursor = QWindowsCursor::createPixmapCursor(newCursor, hotX, hotY)) {
            m_cursors.insert(action, QSharedPointer<DragCursorHandle>(new DragCursorHandle(sysCursor, cpm.cacheKey())));
        }
    }
    if (QWindowsContext::verboseOLE)
        qDebug("%s %d cursors", __FUNCTION__, m_cursors.size());
}

//---------------------------------------------------------------------
//                    IUnknown Methods
//---------------------------------------------------------------------

STDMETHODIMP
QWindowsOleDropSource::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDropSource) {
      *ppv = this;
      ++m_refs;
      return NOERROR;
    }
    *ppv = NULL;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
QWindowsOleDropSource::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
QWindowsOleDropSource::Release(void)
{
    if (--m_refs == 0) {
      delete this;
      return 0;
    }
    return m_refs;
}

/*!
    \brief Check for cancel.
*/

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    HRESULT hr = S_OK;
    do {
        if (fEscapePressed) {
            hr = ResultFromScode(DRAGDROP_S_CANCEL);
            break;
        }

    // grfKeyState is broken on CE & some Windows XP versions,
    // therefore we need to check the state manually
    if ((GetAsyncKeyState(VK_LBUTTON) == 0)
        && (GetAsyncKeyState(VK_MBUTTON) == 0)
        && (GetAsyncKeyState(VK_RBUTTON) == 0)) {
        hr = ResultFromScode(DRAGDROP_S_DROP);
        break;
    }

    const Qt::MouseButtons buttons =  QWindowsMouseHandler::keyStateToMouseButtons(grfKeyState);
    if (m_currentButtons == Qt::NoButton) {
        m_currentButtons = buttons;
    } else {
        // Button changed: Complete Drop operation.
        if (!(m_currentButtons & buttons)) {
            hr = ResultFromScode(DRAGDROP_S_DROP);
            break;
        }
    }

    QGuiApplication::processEvents();

    } while (false);

    if (QWindowsContext::verboseOLE
        && (QWindowsContext::verboseOLE > 1 || hr != S_OK))
        qDebug("%s fEscapePressed=%d, grfKeyState=%lu buttons=%d returns 0x%x",
               __FUNCTION__, fEscapePressed,grfKeyState, int(m_currentButtons),
               int(hr));
    return hr;
}

/*!
    \brief Give feedback: Change cursor accoding to action.
*/

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropSource::GiveFeedback(DWORD dwEffect)
{
    const Qt::DropAction action = translateToQDragDropAction(dwEffect);
    m_drag->updateAction(action);

    if (QWindowsContext::verboseOLE > 2)
        qDebug("%s dwEffect=%lu, action=%d", __FUNCTION__, dwEffect, action);

    QSharedPointer<DragCursorHandle> cursorHandler = m_cursors.value(action);
    qint64 currentCacheKey = m_drag->currentDrag()->dragCursor(action).cacheKey();
    if (cursorHandler.isNull() || currentCacheKey != cursorHandler->cacheKey)
        createCursors();

    const ActionCursorMap::const_iterator it = m_cursors.constFind(action);
    if (it != m_cursors.constEnd()) {
        SetCursor(it.value()->cursor);
        return ResultFromScode(S_OK);
    }

    return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}

/*!
    \class QWindowsOleDropTarget
    \brief Implementation of IDropTarget

    To be registered for each window. Currently, drop sites
    are enabled for top levels. The child window handling
    (sending DragEnter/Leave, etc) is handled in here.

    \sa QWindowsDrag
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsOleDropTarget::QWindowsOleDropTarget(QWindow *w) :
    m_refs(1), m_window(w), m_chosenEffect(0), m_lastKeyState(0)
{
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ <<  this << w;
}

QWindowsOleDropTarget::~QWindowsOleDropTarget()
{
    if (QWindowsContext::verboseOLE)
        qDebug("%s %p", __FUNCTION__, this);
}

STDMETHODIMP
QWindowsOleDropTarget::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDropTarget) {
      *ppv = this;
      AddRef();
      return NOERROR;
    }
    *ppv = NULL;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
QWindowsOleDropTarget::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
QWindowsOleDropTarget::Release(void)
{
    if (--m_refs == 0) {
      delete this;
      return 0;
    }
    return m_refs;
}

QWindow *QWindowsOleDropTarget::findDragOverWindow(const POINTL &pt) const
{
    if (QWindowsWindow *child =
            QWindowsWindow::baseWindowOf(m_window)->childAtScreenPoint(QPoint(pt.x, pt.y)))
            return child->window();
    return m_window;
}

void QWindowsOleDropTarget::handleDrag(QWindow *window, DWORD grfKeyState,
                                       const QPoint &point, LPDWORD pdwEffect)
{
    Q_ASSERT(window);
    m_lastPoint = point;
    m_lastKeyState = grfKeyState;

    QWindowsDrag *windowsDrag = QWindowsDrag::instance();
    const Qt::DropActions actions = translateToQDragDropActions(*pdwEffect);
    QGuiApplicationPrivate::modifier_buttons = toQtKeyboardModifiers(grfKeyState);
    QGuiApplicationPrivate::mouse_buttons = QWindowsMouseHandler::keyStateToMouseButtons(grfKeyState);

    const QPlatformDragQtResponse response =
          QWindowSystemInterface::handleDrag(window, windowsDrag->dropData(), m_lastPoint, actions);

    m_answerRect = response.answerRect();
    const Qt::DropAction action = response.acceptedAction();
    if (response.isAccepted()) {
        m_chosenEffect = translateToWinDragEffects(action);
    } else {
        m_chosenEffect = DROPEFFECT_NONE;
    }
    *pdwEffect = m_chosenEffect;
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ << m_window
                 << windowsDrag->dropData() << " supported actions=" << actions
                 << " mods=" << QGuiApplicationPrivate::modifier_buttons
                 << " mouse=" << QGuiApplicationPrivate::mouse_buttons
                 << " accepted: " << response.isAccepted() << action
                 << m_answerRect << " effect" << *pdwEffect;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragEnter(LPDATAOBJECT pDataObj, DWORD grfKeyState,
                                 POINTL pt, LPDWORD pdwEffect)
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->DragEnter(reinterpret_cast<HWND>(m_window->winId()), pDataObj, reinterpret_cast<POINT*>(&pt), *pdwEffect);

    if (QWindowsContext::verboseOLE)
        qDebug("%s widget=%p key=%lu, pt=%ld,%ld", __FUNCTION__, m_window, grfKeyState, pt.x, pt.y);

    QWindowsDrag::instance()->setDropDataObject(pDataObj);
    pDataObj->AddRef();
    const QPoint point = QWindowsGeometryHint::mapFromGlobal(m_window, QPoint(pt.x,pt.y));
    handleDrag(m_window, grfKeyState, point, pdwEffect);
    return NOERROR;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->DragOver(reinterpret_cast<POINT*>(&pt), *pdwEffect);

    QWindow *dragOverWindow = findDragOverWindow(pt);
    if (QWindowsContext::verboseOLE)
        qDebug("%s widget=%p key=%lu, pt=%ld,%ld", __FUNCTION__, dragOverWindow, grfKeyState, pt.x, pt.y);
    const QPoint tmpPoint = QWindowsGeometryHint::mapFromGlobal(dragOverWindow, QPoint(pt.x,pt.y));
    // see if we should compress this event
    if ((tmpPoint == m_lastPoint || m_answerRect.contains(tmpPoint))
        && m_lastKeyState == grfKeyState) {
        *pdwEffect = m_chosenEffect;
        if (QWindowsContext::verboseOLE)
            qDebug("%s: compressed event", __FUNCTION__);
        return NOERROR;
    }

    handleDrag(dragOverWindow, grfKeyState, tmpPoint, pdwEffect);
    return NOERROR;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragLeave()
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->DragLeave();

    if (QWindowsContext::verboseOLE)
        qDebug().nospace() <<__FUNCTION__ << ' ' << m_window;

    QWindowSystemInterface::handleDrag(m_window, 0, QPoint(), Qt::IgnoreAction);
    QWindowsDrag::instance()->releaseDropDataObject();

    return NOERROR;
}

#define KEY_STATE_BUTTON_MASK (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::Drop(LPDATAOBJECT pDataObj, DWORD grfKeyState,
                            POINTL pt, LPDWORD pdwEffect)
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->Drop(pDataObj, reinterpret_cast<POINT*>(&pt), *pdwEffect);

    QWindow *dropWindow = findDragOverWindow(pt);

    if (QWindowsContext::verboseOLE)
        qDebug().nospace() << __FUNCTION__ << ' ' << m_window
                           << " on " << dropWindow
                           << " keys=" << grfKeyState << " pt="
                           << pt.x << ',' << pt.y;

    m_lastPoint = QWindowsGeometryHint::mapFromGlobal(dropWindow, QPoint(pt.x,pt.y));
    // grfKeyState does not all ways contain button state in the drop so if
    // it doesn't then use the last known button state;
    if ((grfKeyState & KEY_STATE_BUTTON_MASK) == 0)
        grfKeyState |= m_lastKeyState & KEY_STATE_BUTTON_MASK;
    m_lastKeyState = grfKeyState;

    QWindowsDrag *windowsDrag = QWindowsDrag::instance();

    const QPlatformDropQtResponse response =
        QWindowSystemInterface::handleDrop(dropWindow, windowsDrag->dropData(), m_lastPoint,
                                           translateToQDragDropActions(*pdwEffect));

    if (response.isAccepted()) {
        const Qt::DropAction action = response.acceptedAction();
        if (action == Qt::MoveAction || action == Qt::TargetMoveAction) {
            if (action == Qt::MoveAction)
                m_chosenEffect = DROPEFFECT_MOVE;
            else
                m_chosenEffect = DROPEFFECT_COPY;
            HGLOBAL hData = GlobalAlloc(0, sizeof(DWORD));
            if (hData) {
                DWORD *moveEffect = (DWORD *)GlobalLock(hData);;
                *moveEffect = DROPEFFECT_MOVE;
                GlobalUnlock(hData);
                STGMEDIUM medium;
                memset(&medium, 0, sizeof(STGMEDIUM));
                medium.tymed = TYMED_HGLOBAL;
                medium.hGlobal = hData;
                FORMATETC format;
                format.cfFormat = RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
                format.tymed = TYMED_HGLOBAL;
                format.ptd = 0;
                format.dwAspect = 1;
                format.lindex = -1;
                windowsDrag->dropDataObject()->SetData(&format, &medium, true);
            }
        } else {
            m_chosenEffect = translateToWinDragEffects(action);
        }
    } else {
        m_chosenEffect = DROPEFFECT_NONE;
    }
    *pdwEffect = m_chosenEffect;

    windowsDrag->releaseDropDataObject();
    return NOERROR;
}


/*!
    \class QWindowsDrag
    \brief Windows drag implementation.
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsDrag::QWindowsDrag() :
    m_dropDataObject(0), m_cachedDropTargetHelper(0)
{
}

QWindowsDrag::~QWindowsDrag()
{
    if (m_cachedDropTargetHelper)
        m_cachedDropTargetHelper->Release();
}

/*!
    \brief Return data for a drop in process. If it stems from a current drag, use a shortcut.
*/

QMimeData *QWindowsDrag::dropData()
{
    if (const QDrag *drag = currentDrag())
        return drag->mimeData();
    return &m_dropData;
}

/*!
    \brief May be used to handle extended cursors functionality for drags from outside the app.
*/
IDropTargetHelper* QWindowsDrag::dropHelper() {
    if (!m_cachedDropTargetHelper) {
        CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER,
                         IID_IDropTargetHelper,
                         reinterpret_cast<void**>(&m_cachedDropTargetHelper));
    }
    return m_cachedDropTargetHelper;
}

QPixmap QWindowsDrag::defaultCursor(Qt::DropAction action) const
{
    switch (action) {
    case Qt::CopyAction:
        if (m_copyDragCursor.isNull())
            m_copyDragCursor = QPixmap(copyDragCursorXpmC);
        return m_copyDragCursor;
    case Qt::TargetMoveAction:
    case Qt::MoveAction:
        if (m_moveDragCursor.isNull())
            m_moveDragCursor = QPixmap(moveDragCursorXpmC);
        return m_moveDragCursor;
    case Qt::LinkAction:
        if (m_linkDragCursor.isNull())
            m_linkDragCursor = QPixmap(linkDragCursorXpmC);
        return m_linkDragCursor;
    default:
        break;
    }
    if (m_ignoreDragCursor.isNull())
        m_ignoreDragCursor = QPixmap(ignoreDragCursorXpmC);
    return m_ignoreDragCursor;
}

Qt::DropAction QWindowsDrag::drag(QDrag *drag)
{
    // TODO: Accessibility handling?
    QMimeData *dropData = drag->mimeData();
    Qt::DropAction dragResult = Qt::IgnoreAction;

    DWORD resultEffect;
    QWindowsOleDropSource *windowDropSource = new QWindowsOleDropSource(this);
    windowDropSource->createCursors();
    QWindowsOleDataObject *dropDataObject = new QWindowsOleDataObject(dropData);
    const Qt::DropActions possibleActions = drag->supportedActions();
    const DWORD allowedEffects = translateToWinDragEffects(possibleActions);
    if (QWindowsContext::verboseOLE)
          qDebug(">%s possible Actions=%x, effects=0x%lx", __FUNCTION__,
                 int(possibleActions), allowedEffects);
    const HRESULT r = DoDragDrop(dropDataObject, windowDropSource, allowedEffects, &resultEffect);
    const DWORD  reportedPerformedEffect = dropDataObject->reportedPerformedEffect();
    if (r == DRAGDROP_S_DROP) {
        if (reportedPerformedEffect == DROPEFFECT_MOVE && resultEffect != DROPEFFECT_MOVE) {
            dragResult = Qt::TargetMoveAction;
            resultEffect = DROPEFFECT_MOVE;
        } else {
            dragResult = translateToQDragDropAction(resultEffect);
        }
        // Force it to be a copy if an unsupported operation occurred.
        // This indicates a bug in the drop target.
        if (resultEffect != DROPEFFECT_NONE && !(resultEffect & allowedEffects)) {
            qWarning("%s: Forcing Qt::CopyAction", __FUNCTION__);
            dragResult = Qt::CopyAction;
        }
    }
    // clean up
    dropDataObject->releaseQt();
    dropDataObject->Release();        // Will delete obj if refcount becomes 0
    windowDropSource->Release();        // Will delete src if refcount becomes 0
    if (QWindowsContext::verboseOLE)
        qDebug("<%s allowedEffects=0x%lx, reportedPerformedEffect=0x%lx, resultEffect=0x%lx, hr=0x%x, dropAction=%d",
               __FUNCTION__, allowedEffects, reportedPerformedEffect,
               resultEffect, int(r), dragResult);
    return dragResult;
}

QWindowsDrag *QWindowsDrag::instance()
{
    return static_cast<QWindowsDrag *>(QWindowsIntegration::instance()->drag());
}

void QWindowsDrag::releaseDropDataObject()
{
    if (QWindowsContext::verboseOLE)
        qDebug("%s %p", __FUNCTION__, m_dropDataObject);
    if (m_dropDataObject) {
        m_dropDataObject->Release();
        m_dropDataObject = 0;
    }
}

QT_END_NAMESPACE
