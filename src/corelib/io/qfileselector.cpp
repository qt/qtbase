// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:file-selection

#include "qfileselector.h"
#include "qfileselector_p.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/private/qlocking_p.h>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

//Environment variable to allow tooling full control of file selectors
static const char env_override[] = "QT_NO_BUILTIN_SELECTORS";

Q_GLOBAL_STATIC(QFileSelectorSharedData, sharedData);
Q_CONSTINIT static QBasicMutex sharedDataMutex;

QFileSelectorPrivate::QFileSelectorPrivate()
    : QObjectPrivate()
{
}

/*!
    \class QFileSelector
    \inmodule QtCore
    \brief QFileSelector provides a convenient way of selecting file variants.
    \since 5.2

    QFileSelector is a convenience for selecting file variants based on platform or device
    characteristics. This allows you to develop and deploy one codebase containing all the
    different variants more easily in some circumstances, such as when the correct variant cannot
    be determined during the deploy step.

    \section1 Using QFileSelector

    If you always use the same file you do not need to use QFileSelector.

    Consider the following example usage, where you want to use different settings files on
    different locales. You might select code between locales like this:

    \snippet code/src_corelib_io_qfileselector.cpp 0

    Similarly, if you want to pick a different data file based on target platform,
    your code might look something like this:
    \snippet code/src_corelib_io_qfileselector.cpp 1

    QFileSelector provides a convenient alternative to writing such boilerplate code, and in the
    latter case it allows you to start using an platform-specific configuration without a recompile.
    QFileSelector also allows for chaining of multiple selectors in a convenient way, for example
    selecting a different file only on certain combinations of platform and locale. For example, to
    select based on platform and/or locale, the code is as follows:

    \snippet code/src_corelib_io_qfileselector.cpp 2

    The files to be selected are placed in directories named with a \c'+' and a selector name. In the above
    example you could have the platform configurations selected by placing them in the following locations:
    \snippet code/src_corelib_io_qfileselector.cpp 3

    To find selected files, QFileSelector looks in the same directory as the base file. If there are
    any directories of the form +<selector> with an active selector, QFileSelector will prefer a file
    with the same file name from that directory over the base file. These directories can be nested to
    check against multiple selectors, for example:
    \snippet code/src_corelib_io_qfileselector.cpp 4
    With those files available, you would select a different file on the android platform,
    but only if the locale was en_GB.

    For error handling in the case no valid selectors are present, it is recommended to have a default or
    error-handling file in the base file location even if you expect selectors to be present for all
    deployments.

    In a future version, some may be marked as deploy-time static and be moved during the
    deployment step as an optimization. As selectors come with a performance cost, it is
    recommended to avoid their use in circumstances involving performance-critical code.

    \section1 Adding Selectors

    Selectors normally available are
    \list
    \li platform, any of the following strings which match the platform the application is running
        on (list not exhaustive): android, ios, osx, darwin, mac, macos, linux, qnx, unix, windows.
        On Linux, if it can be determined, the name of the distribution too, like debian,
        fedora or opensuse.
    \li locale, same as QLocale().name().
    \endlist

    Further selectors will be added from the \c QT_FILE_SELECTORS environment variable, which
    when set should be a set of comma separated selectors. Note that this variable will only be
    read once; selectors may not update if the variable changes while the application is running.
    The initial set of selectors are evaluated only once, on first use.

    You can also add extra selectors at runtime for custom behavior. These will be used in any
    future calls to select(). If the extra selectors list has been changed, calls to select() will
    use the new list and may return differently.

    \section1 Conflict Resolution when Multiple Selectors Apply

    When multiple selectors could be applied to the same file, the first matching selector is chosen.
    The order selectors are checked in are:

    \list 1
    \li Selectors set via setExtraSelectors(), in the order they are in the list
    \li Selectors in the \c QT_FILE_SELECTORS environment variable, from left to right
    \li Locale
    \li Platform
    \endlist

    Here is an example involving multiple selectors matching at the same time. It uses platform
    selectors, plus an extra selector named "admin" is set by the application based on user
    credentials. The example is sorted so that the lowest matching file would be chosen if all
    selectors were present:

    \snippet code/src_corelib_io_qfileselector.cpp 5

    Because extra selectors are checked before platform the \c{+admin/background.png} will be chosen
    on Windows when the admin selector is set, and \c{+windows/background.png} will be chosen on
    Windows when the admin selector is not set.  On Linux, the \c{+admin/+linux/background.png} will be
    chosen when admin is set, and the \c{+linux/background.png} when it is not.

*/

/*!
    Create a QFileSelector instance. This instance will have the same static selectors as other
    QFileSelector instances, but its own set of extra selectors.

    If supplied, it will have the given QObject \a parent.
*/
QFileSelector::QFileSelector(QObject *parent)
    : QObject(*(new QFileSelectorPrivate()), parent)
{
}

/*!
  Destroys this selector instance.
*/
QFileSelector::~QFileSelector()
{
}

/*!
   This function returns the selected version of the path, based on the conditions at runtime.
   If no selectable files are present, returns the original \a filePath.

   If the original file does not exist, the original \a filePath is returned. This means that you
   must have a base file to fall back on, you cannot have only files in selectable sub-directories.

   See the class overview for the selection algorithm.
*/
QString QFileSelector::select(const QString &filePath) const
{
    Q_D(const QFileSelector);
    return d->select(filePath);
}

static bool isLocalScheme(const QString &file)
{
    bool local = file == "qrc"_L1;
#ifdef Q_OS_ANDROID
    local |= file == "assets"_L1;
#endif
    return local;
}

/*!
   This is a convenience version of select operating on QUrl objects. If the scheme is not file or qrc,
   \a filePath is returned immediately. Otherwise selection is applied to the path of \a filePath
   and a QUrl is returned with the selected path and other QUrl parts the same as \a filePath.

   See the class overview for the selection algorithm.
*/
QUrl QFileSelector::select(const QUrl &filePath) const
{
    Q_D(const QFileSelector);
    if (!isLocalScheme(filePath.scheme()) && !filePath.isLocalFile())
        return filePath;
    QUrl ret(filePath);
    if (isLocalScheme(filePath.scheme())) {
        auto scheme = ":"_L1;
#ifdef Q_OS_ANDROID
        // use other scheme because ":" means "qrc" here
        if (filePath.scheme() == "assets"_L1)
            scheme = "assets:"_L1;
#endif

        QString equivalentPath = scheme + filePath.path();
        QString selectedPath = d->select(equivalentPath);
        ret.setPath(selectedPath.remove(0, scheme.size()));
    } else {
        // we need to store the original query and fragment, since toLocalFile() will strip it off
        QString frag;
        if (ret.hasFragment())
            frag = ret.fragment();
        QString query;
        if (ret.hasQuery())
            query= ret.query();
        ret = QUrl::fromLocalFile(d->select(ret.toLocalFile()));
        if (!frag.isNull())
            ret.setFragment(frag);
        if (!query.isNull())
            ret.setQuery(query);
    }
    return ret;
}

QString QFileSelectorPrivate::selectionHelper(const QString &path, const QString &fileName,
                                              const QStringList &selectors, QChar indicator)
{
    /* selectionHelper does a depth-first search of possible selected files. Because there is strict
       selector ordering in the API, we can stop checking as soon as we find the file in a directory
       which does not contain any other valid selector directories.
    */
    Q_ASSERT(path.isEmpty() || path.endsWith(u'/'));

    for (const QString &s : selectors) {
        QString prospectiveBase = path;
        if (!indicator.isNull())
            prospectiveBase += indicator;
        prospectiveBase += s + u'/';
        QStringList remainingSelectors = selectors;
        remainingSelectors.removeAll(s);
        if (!QDir(prospectiveBase).exists())
            continue;
        QString prospectiveFile = selectionHelper(prospectiveBase, fileName, remainingSelectors, indicator);
        if (!prospectiveFile.isEmpty())
            return prospectiveFile;
    }

    // If we reach here there were no successful files found at a lower level in this branch, so we
    // should check this level as a potential result.
    if (!QFile::exists(path + fileName))
        return QString();
    return path + fileName;
}

QString QFileSelectorPrivate::select(const QString &filePath) const
{
    Q_Q(const QFileSelector);
    QFileInfo fi(filePath);

    QString pathString;
    if (auto path = fi.path(); !path.isEmpty())
        pathString = path.endsWith(u'/') ? path : path + u'/';
    QString ret = selectionHelper(pathString,
            fi.fileName(), q->allSelectors());

    if (!ret.isEmpty())
        return ret;
    return filePath;
}

/*!
    Returns the list of extra selectors which have been added programmatically to this instance.
*/
QStringList QFileSelector::extraSelectors() const
{
    Q_D(const QFileSelector);
    return d->extras;
}

/*!
    Sets the \a list of extra selectors which have been added programmatically to this instance.

    These selectors have priority over any which have been automatically picked up.
*/
void QFileSelector::setExtraSelectors(const QStringList &list)
{
    Q_D(QFileSelector);
    d->extras = list;
}

/*!
    Returns the complete, ordered list of selectors used by this instance
*/
QStringList QFileSelector::allSelectors() const
{
    Q_D(const QFileSelector);
    const auto locker = qt_scoped_lock(sharedDataMutex);
    QFileSelectorPrivate::updateSelectors();
    return d->extras + sharedData->staticSelectors;
}

void QFileSelectorPrivate::updateSelectors()
{
    if (!sharedData->staticSelectors.isEmpty())
        return; //Already loaded

    QLatin1Char pathSep(',');
    QStringList envSelectors = QString::fromLatin1(qgetenv("QT_FILE_SELECTORS"))
                                .split(pathSep, Qt::SkipEmptyParts);
    if (envSelectors.size())
        sharedData->staticSelectors << envSelectors;

    if (!qEnvironmentVariableIsEmpty(env_override))
        return;

    sharedData->staticSelectors << sharedData->preloadedStatics; //Potential for static selectors from other modules

    // TODO: Update on locale changed?
    sharedData->staticSelectors << QLocale().name();

    sharedData->staticSelectors << platformSelectors();
}

QStringList QFileSelectorPrivate::platformSelectors()
{
    // similar, but not identical to QSysInfo::osType
    QStringList ret;
#if defined(Q_OS_WIN)
    ret << QStringLiteral("windows");
    ret << QSysInfo::kernelType();  // "winnt"
#elif defined(Q_OS_UNIX)
    ret << QStringLiteral("unix");
#  if !defined(Q_OS_ANDROID) && !defined(Q_OS_QNX) && !defined(Q_OS_VXWORKS)
    // we don't want "linux" for Android or two instances of "qnx" for QNX
    // or two instances of "vxworks" for vxworks
    ret << QSysInfo::kernelType();
#  endif
    QString productName = QSysInfo::productType();
    if (productName != "unknown"_L1)
        ret << productName; // "opensuse", "fedora", "osx", "ios", "android"
#endif
    return ret;
}

void QFileSelectorPrivate::addStatics(const QStringList &statics)
{
    const auto locker = qt_scoped_lock(sharedDataMutex);
    sharedData->preloadedStatics << statics;
    sharedData->staticSelectors.clear();
}

QT_END_NAMESPACE

#include "moc_qfileselector.cpp"
