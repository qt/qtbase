// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfilesystemmodel_p.h"
#include "qfilesystemmodel.h"
#include <qabstractfileiconprovider.h>
#include <qlocale.h>
#include <qmimedata.h>
#include <qurl.h>
#include <qdebug.h>
#include <QtCore/qcollator.h>
#if QT_CONFIG(regularexpression)
#  include <QtCore/qregularexpression.h>
#endif

#include <algorithm>

#ifdef Q_OS_WIN
#  include <QtCore/QVarLengthArray>
#  include <qt_windows.h>
#  include <shlobj.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \enum QFileSystemModel::Roles
    \value FileIconRole
    \value FilePathRole
    \value FileNameRole
    \value FilePermissions
*/

/*!
    \class QFileSystemModel
    \since 4.4

    \brief The QFileSystemModel class provides a data model for the local filesystem.

    \ingroup model-view
    \inmodule QtGui

    This class provides access to the local filesystem, providing functions
    for renaming and removing files and directories, and for creating new
    directories. In the simplest case, it can be used with a suitable display
    widget as part of a browser or filter.

    QFileSystemModel can be accessed using the standard interface provided by
    QAbstractItemModel, but it also provides some convenience functions that are
    specific to a directory model.
    The fileInfo(), isDir(), fileName() and filePath() functions provide information
    about the underlying files and directories related to items in the model.
    Directories can be created and removed using mkdir(), rmdir().

    \note QFileSystemModel requires an instance of \l QApplication.

    \section1 Example Usage

    A directory model that displays the contents of a default directory
    is usually constructed with a parent object:

    \snippet shareddirmodel/main.cpp 2

    A tree view can be used to display the contents of the model

    \snippet shareddirmodel/main.cpp 4

    and the contents of a particular directory can be displayed by
    setting the tree view's root index:

    \snippet shareddirmodel/main.cpp 7

    The view's root index can be used to control how much of a
    hierarchical model is displayed. QFileSystemModel provides a convenience
    function that returns a suitable model index for a path to a
    directory within the model.

    \section1 Caching and Performance

    QFileSystemModel will not fetch any files or directories until setRootPath()
    is called.  This will prevent any unnecessary querying on the file system
    until that point such as listing the drives on Windows.

    QFileSystemModel uses a separate thread to populate itself so it will not
    cause the main thread to hang as the file system is being queried.
    Calls to rowCount() will return 0 until the model populates a directory.

    QFileSystemModel keeps a cache with file information. The cache is
    automatically kept up to date using the QFileSystemWatcher.

    \sa {Model Classes}
*/

/*!
    \fn bool QFileSystemModel::rmdir(const QModelIndex &index)

    Removes the directory corresponding to the model item \a index in the
    file system model and \b{deletes the corresponding directory from the
    file system}, returning true if successful. If the directory cannot be
    removed, false is returned.

    \warning This function deletes directories from the file system; it does
    \b{not} move them to a location where they can be recovered.

    \sa remove()
*/

/*!
    \fn QString QFileSystemModel::fileName(const QModelIndex &index) const

    Returns the file name for the item stored in the model under the given
    \a index.
*/

/*!
    \fn QIcon QFileSystemModel::fileIcon(const QModelIndex &index) const

    Returns the icon for the item stored in the model under the given
    \a index.
*/

/*!
    \fn QFileInfo QFileSystemModel::fileInfo(const QModelIndex &index) const

    Returns the QFileInfo for the item stored in the model under the given
    \a index.
*/
QFileInfo QFileSystemModel::fileInfo(const QModelIndex &index) const
{
    Q_D(const QFileSystemModel);
    return d->node(index)->fileInfo();
}

/*!
    \fn void QFileSystemModel::rootPathChanged(const QString &newPath);

    This signal is emitted whenever the root path has been changed to a \a newPath.
*/

/*!
    \fn void QFileSystemModel::fileRenamed(const QString &path, const QString &oldName, const QString &newName)

    This signal is emitted whenever a file with the \a oldName is successfully
    renamed to \a newName.  The file is located in the directory \a path.
*/

/*!
    \since 4.7
    \fn void QFileSystemModel::directoryLoaded(const QString &path)

    This signal is emitted when the gatherer thread has finished to load the \a path.

*/

/*!
    \fn bool QFileSystemModel::remove(const QModelIndex &index)

    Removes the model item \a index from the file system model and \b{deletes the
    corresponding file from the file system}, returning true if successful. If the
    item cannot be removed, false is returned.

    \warning This function deletes files from the file system; it does \b{not}
    move them to a location where they can be recovered.

    \sa rmdir()
*/

bool QFileSystemModel::remove(const QModelIndex &aindex)
{
    Q_D(QFileSystemModel);

    const QString path = d->filePath(aindex);
    const QFileInfo fileInfo(path);
#if QT_CONFIG(filesystemwatcher) && defined(Q_OS_WIN)
    // QTBUG-65683: Remove file system watchers prior to deletion to prevent
    // failure due to locked files on Windows.
    const QStringList watchedPaths = d->unwatchPathsAt(aindex);
#endif // filesystemwatcher && Q_OS_WIN
    const bool success = (fileInfo.isFile() || fileInfo.isSymLink())
            ? QFile::remove(path) : QDir(path).removeRecursively();
#if QT_CONFIG(filesystemwatcher) && defined(Q_OS_WIN)
    if (!success)
        d->watchPaths(watchedPaths);
#endif // filesystemwatcher && Q_OS_WIN
    return success;
}

/*!
  Constructs a file system model with the given \a parent.
*/
QFileSystemModel::QFileSystemModel(QObject *parent) :
    QFileSystemModel(*new QFileSystemModelPrivate, parent)
{
}

/*!
    \internal
*/
QFileSystemModel::QFileSystemModel(QFileSystemModelPrivate &dd, QObject *parent)
    : QAbstractItemModel(dd, parent)
{
    Q_D(QFileSystemModel);
    d->init();
}

/*!
  Destroys this file system model.
*/
QFileSystemModel::~QFileSystemModel() = default;

/*!
    \reimp
*/
QModelIndex QFileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QFileSystemModel);
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();

    // get the parent node
    QFileSystemModelPrivate::QFileSystemNode *parentNode = (d->indexValid(parent) ? d->node(parent) :
                                                   const_cast<QFileSystemModelPrivate::QFileSystemNode*>(&d->root));
    Q_ASSERT(parentNode);

    // now get the internal pointer for the index
    const int i = d->translateVisibleLocation(parentNode, row);
    if (i >= parentNode->visibleChildren.size())
        return QModelIndex();
    const QString &childName = parentNode->visibleChildren.at(i);
    const QFileSystemModelPrivate::QFileSystemNode *indexNode = parentNode->children.value(childName);
    Q_ASSERT(indexNode);

    return createIndex(row, column, const_cast<QFileSystemModelPrivate::QFileSystemNode*>(indexNode));
}

/*!
    \reimp
*/
QModelIndex QFileSystemModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if (row == idx.row() && column < columnCount(idx.parent())) {
        // cheap sibling operation: just adjust the column:
        return createIndex(row, column, idx.internalPointer());
    } else {
        // for anything else: call the default implementation
        // (this could probably be optimized, too):
        return QAbstractItemModel::sibling(row, column, idx);
    }
}

/*!
    \overload

    Returns the model item index for the given \a path and \a column.
*/
QModelIndex QFileSystemModel::index(const QString &path, int column) const
{
    Q_D(const QFileSystemModel);
    QFileSystemModelPrivate::QFileSystemNode *node = d->node(path, false);
    return d->index(node, column);
}

/*!
    \internal

    Return the QFileSystemNode that goes to index.
  */
QFileSystemModelPrivate::QFileSystemNode *QFileSystemModelPrivate::node(const QModelIndex &index) const
{
    if (!index.isValid())
        return const_cast<QFileSystemNode*>(&root);
    QFileSystemModelPrivate::QFileSystemNode *indexNode = static_cast<QFileSystemModelPrivate::QFileSystemNode*>(index.internalPointer());
    Q_ASSERT(indexNode);
    return indexNode;
}

#ifdef Q_OS_WIN32
static QString qt_GetLongPathName(const QString &strShortPath)
{
    if (strShortPath.isEmpty()
        || strShortPath == "."_L1 || strShortPath == ".."_L1)
        return strShortPath;
    if (strShortPath.length() == 2 && strShortPath.endsWith(u':'))
        return strShortPath.toUpper();
    const QString absPath = QDir(strShortPath).absolutePath();
    if (absPath.startsWith("//"_L1)
        || absPath.startsWith("\\\\"_L1)) // unc
        return QDir::fromNativeSeparators(absPath);
    if (absPath.startsWith(u'/'))
        return QString();
    const QString inputString = "\\\\?\\"_L1 + QDir::toNativeSeparators(absPath);
    QVarLengthArray<TCHAR, MAX_PATH> buffer(MAX_PATH);
    DWORD result = ::GetLongPathName((wchar_t*)inputString.utf16(),
                                     buffer.data(),
                                     buffer.size());
    if (result > DWORD(buffer.size())) {
        buffer.resize(result);
        result = ::GetLongPathName((wchar_t*)inputString.utf16(),
                                   buffer.data(),
                                   buffer.size());
    }
    if (result > 4) {
        QString longPath = QString::fromWCharArray(buffer.data() + 4); // ignoring prefix
        longPath[0] = longPath.at(0).toUpper(); // capital drive letters
        return QDir::fromNativeSeparators(longPath);
    } else {
        return QDir::fromNativeSeparators(strShortPath);
    }
}
#endif

/*!
    \internal

    Given a path return the matching QFileSystemNode or &root if invalid
*/
QFileSystemModelPrivate::QFileSystemNode *QFileSystemModelPrivate::node(const QString &path, bool fetch) const
{
    Q_Q(const QFileSystemModel);
    Q_UNUSED(q);
    if (path.isEmpty() || path == myComputer() || path.startsWith(u':'))
        return const_cast<QFileSystemModelPrivate::QFileSystemNode*>(&root);

    // Construct the nodes up to the new root path if they need to be built
    QString absolutePath;
#ifdef Q_OS_WIN32
    QString longPath = qt_GetLongPathName(path);
#else
    QString longPath = path;
#endif
    if (longPath == rootDir.path())
        absolutePath = rootDir.absolutePath();
    else
        absolutePath = QDir(longPath).absolutePath();

    // ### TODO can we use bool QAbstractFileEngine::caseSensitive() const?
    QStringList pathElements = absolutePath.split(u'/', Qt::SkipEmptyParts);
    if ((pathElements.isEmpty())
#if !defined(Q_OS_WIN)
        && QDir::fromNativeSeparators(longPath) != "/"_L1
#endif
        )
        return const_cast<QFileSystemModelPrivate::QFileSystemNode*>(&root);
    QModelIndex index = QModelIndex(); // start with "My Computer"
    QString elementPath;
    QChar separator = u'/';
    QString trailingSeparator;
#if defined(Q_OS_WIN)
    if (absolutePath.startsWith("//"_L1)) { // UNC path
        QString host = "\\\\"_L1 + pathElements.constFirst();
        if (absolutePath == QDir::fromNativeSeparators(host))
            absolutePath.append(u'/');
        if (longPath.endsWith(u'/') && !absolutePath.endsWith(u'/'))
            absolutePath.append(u'/');
        if (absolutePath.endsWith(u'/'))
            trailingSeparator = "\\"_L1;
        int r = 0;
        auto rootNode = const_cast<QFileSystemModelPrivate::QFileSystemNode*>(&root);
        auto it = root.children.constFind(host);
        if (it != root.children.cend()) {
            host = it.key(); // Normalize case for lookup in visibleLocation()
        } else {
            if (pathElements.count() == 1 && !absolutePath.endsWith(u'/'))
                return rootNode;
            QFileInfo info(host);
            if (!info.exists())
                return rootNode;
            QFileSystemModelPrivate *p = const_cast<QFileSystemModelPrivate*>(this);
            p->addNode(rootNode, host,info);
            p->addVisibleFiles(rootNode, QStringList(host));
        }
        r = rootNode->visibleLocation(host);
        r = translateVisibleLocation(rootNode, r);
        index = q->index(r, 0, QModelIndex());
        pathElements.pop_front();
        separator = u'\\';
        elementPath = host;
        elementPath.append(separator);
    } else {
        if (!pathElements.at(0).contains(u':')) {
            QString rootPath = QDir(longPath).rootPath();
            pathElements.prepend(rootPath);
        }
        if (pathElements.at(0).endsWith(u'/'))
            pathElements[0].chop(1);
    }
#else
    // add the "/" item, since it is a valid path element on Unix
    if (absolutePath[0] == u'/')
        pathElements.prepend("/"_L1);
#endif

    QFileSystemModelPrivate::QFileSystemNode *parent = node(index);

    for (int i = 0; i < pathElements.size(); ++i) {
        QString element = pathElements.at(i);
        if (i != 0)
            elementPath.append(separator);
        elementPath.append(element);
        if (i == pathElements.size() - 1)
            elementPath.append(trailingSeparator);
#ifdef Q_OS_WIN
        // On Windows, "filename    " and "filename" are equivalent and
        // "filename  .  " and "filename" are equivalent
        // "filename......." and "filename" are equivalent Task #133928
        // whereas "filename  .txt" is still "filename  .txt"
        // If after stripping the characters there is nothing left then we
        // just return the parent directory as it is assumed that the path
        // is referring to the parent
        while (element.endsWith(u'.') || element.endsWith(u' '))
            element.chop(1);
        // Only filenames that can't possibly exist will be end up being empty
        if (element.isEmpty())
            return parent;
#endif
        bool alreadyExisted = parent->children.contains(element);

        // we couldn't find the path element, we create a new node since we
        // _know_ that the path is valid
        if (alreadyExisted) {
            if ((parent->children.size() == 0)
                || (parent->caseSensitive()
                    && parent->children.value(element)->fileName != element)
                || (!parent->caseSensitive()
                    && parent->children.value(element)->fileName.toLower() != element.toLower()))
                alreadyExisted = false;
        }

        QFileSystemModelPrivate::QFileSystemNode *node;
        if (!alreadyExisted) {
            // Someone might call ::index("file://cookie/monster/doesn't/like/veggies"),
            // a path that doesn't exists, I.E. don't blindly create directories.
            QFileInfo info(elementPath);
            if (!info.exists())
                return const_cast<QFileSystemModelPrivate::QFileSystemNode*>(&root);
            QFileSystemModelPrivate *p = const_cast<QFileSystemModelPrivate*>(this);
            node = p->addNode(parent, element,info);
#if QT_CONFIG(filesystemwatcher)
            node->populate(fileInfoGatherer.getInfo(info));
#endif
        } else {
            node = parent->children.value(element);
        }

        Q_ASSERT(node);
        if (!node->isVisible) {
            // It has been filtered out
            if (alreadyExisted && node->hasInformation() && !fetch)
                return const_cast<QFileSystemModelPrivate::QFileSystemNode*>(&root);

            QFileSystemModelPrivate *p = const_cast<QFileSystemModelPrivate*>(this);
            p->addVisibleFiles(parent, QStringList(element));
            if (!p->bypassFilters.contains(node))
                p->bypassFilters[node] = 1;
            QString dir = q->filePath(this->index(parent));
            if (!node->hasInformation() && fetch) {
                Fetching f = { std::move(dir), std::move(element), node };
                p->toFetch.append(std::move(f));
                p->fetchingTimer.start(0, const_cast<QFileSystemModel*>(q));
            }
        }
        parent = node;
    }

    return parent;
}

/*!
    \reimp
*/
void QFileSystemModel::timerEvent(QTimerEvent *event)
{
    Q_D(QFileSystemModel);
    if (event->timerId() == d->fetchingTimer.timerId()) {
        d->fetchingTimer.stop();
#if QT_CONFIG(filesystemwatcher)
        for (int i = 0; i < d->toFetch.size(); ++i) {
            const QFileSystemModelPrivate::QFileSystemNode *node = d->toFetch.at(i).node;
            if (!node->hasInformation()) {
                d->fileInfoGatherer.fetchExtendedInformation(d->toFetch.at(i).dir,
                                                 QStringList(d->toFetch.at(i).file));
            } else {
                // qDebug("yah!, you saved a little gerbil soul");
            }
        }
#endif
        d->toFetch.clear();
    }
}

/*!
    Returns \c true if the model item \a index represents a directory;
    otherwise returns \c false.
*/
bool QFileSystemModel::isDir(const QModelIndex &index) const
{
    // This function is for public usage only because it could create a file info
    Q_D(const QFileSystemModel);
    if (!index.isValid())
        return true;
    QFileSystemModelPrivate::QFileSystemNode *n = d->node(index);
    if (n->hasInformation())
        return n->isDir();
    return fileInfo(index).isDir();
}

/*!
    Returns the size in bytes of \a index. If the file does not exist, 0 is returned.
  */
qint64 QFileSystemModel::size(const QModelIndex &index) const
{
    Q_D(const QFileSystemModel);
    if (!index.isValid())
        return 0;
    return d->node(index)->size();
}

/*!
    Returns the type of file \a index such as "Directory" or "JPEG file".
  */
QString QFileSystemModel::type(const QModelIndex &index) const
{
    Q_D(const QFileSystemModel);
    if (!index.isValid())
        return QString();
    return d->node(index)->type();
}

/*!
    Returns the date and time (in local time) when \a index was last modified.

    This is an overloaded function, equivalent to calling:
    \code
    lastModified(index, QTimeZone::LocalTime);
    \endcode

    If \a index is invalid, a default constructed QDateTime is returned.
 */
QDateTime QFileSystemModel::lastModified(const QModelIndex &index) const
{
    return lastModified(index, QTimeZone::LocalTime);
}

/*!
    \since 6.6
    Returns the date and time, in the time zone \a tz, when
    \a index was last modified.

    Typical arguments for \a tz are \c QTimeZone::UTC or \c QTimeZone::LocalTime.
    UTC does not require any conversion from the time returned by the native file
    system API, therefore getting the time in UTC is potentially faster. LocalTime
    is typically chosen if the time is shown to the user.

    If \a index is invalid, a default constructed QDateTime is returned.
 */
QDateTime QFileSystemModel::lastModified(const QModelIndex &index, const QTimeZone &tz) const
{
    Q_D(const QFileSystemModel);
    if (!index.isValid())
        return QDateTime();
    return d->node(index)->lastModified(tz);
}

/*!
    \reimp
*/
QModelIndex QFileSystemModel::parent(const QModelIndex &index) const
{
    Q_D(const QFileSystemModel);
    if (!d->indexValid(index))
        return QModelIndex();

    QFileSystemModelPrivate::QFileSystemNode *indexNode = d->node(index);
    Q_ASSERT(indexNode != nullptr);
    QFileSystemModelPrivate::QFileSystemNode *parentNode = indexNode->parent;
    if (parentNode == nullptr || parentNode == &d->root)
        return QModelIndex();

    // get the parent's row
    QFileSystemModelPrivate::QFileSystemNode *grandParentNode = parentNode->parent;
    Q_ASSERT(grandParentNode->children.contains(parentNode->fileName));
    int visualRow = d->translateVisibleLocation(grandParentNode, grandParentNode->visibleLocation(grandParentNode->children.value(parentNode->fileName)->fileName));
    if (visualRow == -1)
        return QModelIndex();
    return createIndex(visualRow, 0, parentNode);
}

/*
    \internal

    return the index for node
*/
QModelIndex QFileSystemModelPrivate::index(const QFileSystemModelPrivate::QFileSystemNode *node, int column) const
{
    Q_Q(const QFileSystemModel);
    QFileSystemModelPrivate::QFileSystemNode *parentNode = (node ? node->parent : nullptr);
    if (node == &root || !parentNode)
        return QModelIndex();

    // get the parent's row
    Q_ASSERT(node);
    if (!node->isVisible)
        return QModelIndex();

    int visualRow = translateVisibleLocation(parentNode, parentNode->visibleLocation(node->fileName));
    return q->createIndex(visualRow, column, const_cast<QFileSystemNode*>(node));
}

/*!
    \reimp
*/
bool QFileSystemModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const QFileSystemModel);
    if (parent.column() > 0)
        return false;

    if (!parent.isValid()) // drives
        return true;

    const QFileSystemModelPrivate::QFileSystemNode *indexNode = d->node(parent);
    Q_ASSERT(indexNode);
    return (indexNode->isDir());
}

/*!
    \reimp
 */
bool QFileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    Q_D(const QFileSystemModel);
    if (!d->setRootPath)
        return false;
    const QFileSystemModelPrivate::QFileSystemNode *indexNode = d->node(parent);
    return (!indexNode->populatedChildren);
}

/*!
    \reimp
 */
void QFileSystemModel::fetchMore(const QModelIndex &parent)
{
    Q_D(QFileSystemModel);
    if (!d->setRootPath)
        return;
    QFileSystemModelPrivate::QFileSystemNode *indexNode = d->node(parent);
    if (indexNode->populatedChildren)
        return;
    indexNode->populatedChildren = true;
#if QT_CONFIG(filesystemwatcher)
    d->fileInfoGatherer.list(filePath(parent));
#endif
}

/*!
    \reimp
*/
int QFileSystemModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QFileSystemModel);
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return d->root.visibleChildren.size();

    const QFileSystemModelPrivate::QFileSystemNode *parentNode = d->node(parent);
    return parentNode->visibleChildren.size();
}

/*!
    \reimp
*/
int QFileSystemModel::columnCount(const QModelIndex &parent) const
{
    return (parent.column() > 0) ? 0 : QFileSystemModelPrivate::NumColumns;
}

/*!
    Returns the data stored under the given \a role for the item "My Computer".

    \sa Qt::ItemDataRole
 */
QVariant QFileSystemModel::myComputer(int role) const
{
#if QT_CONFIG(filesystemwatcher)
    Q_D(const QFileSystemModel);
#endif
    switch (role) {
    case Qt::DisplayRole:
        return QFileSystemModelPrivate::myComputer();
#if QT_CONFIG(filesystemwatcher)
    case Qt::DecorationRole:
        return d->fileInfoGatherer.iconProvider()->icon(QAbstractFileIconProvider::Computer);
#endif
    }
    return QVariant();
}

/*!
    \reimp
*/
QVariant QFileSystemModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QFileSystemModel);
    if (!index.isValid() || index.model() != this)
        return QVariant();

    switch (role) {
    case Qt::EditRole:
        if (index.column() == QFileSystemModelPrivate::NameColumn)
            return d->name(index);
        Q_FALLTHROUGH();
    case Qt::DisplayRole:
        switch (index.column()) {
        case QFileSystemModelPrivate::NameColumn: return d->displayName(index);
        case QFileSystemModelPrivate::SizeColumn: return d->size(index);
        case QFileSystemModelPrivate::TypeColumn: return d->type(index);
        case QFileSystemModelPrivate::TimeColumn: return d->time(index);
        default:
            qWarning("data: invalid display value column %d", index.column());
            break;
        }
        break;
    case FilePathRole:
        return filePath(index);
    case FileNameRole:
        return d->name(index);
    case Qt::DecorationRole:
        if (index.column() == QFileSystemModelPrivate::NameColumn) {
            QIcon icon = d->icon(index);
#if QT_CONFIG(filesystemwatcher)
            if (icon.isNull()) {
                if (d->node(index)->isDir())
                    icon = d->fileInfoGatherer.iconProvider()->icon(QAbstractFileIconProvider::Folder);
                else
                    icon = d->fileInfoGatherer.iconProvider()->icon(QAbstractFileIconProvider::File);
            }
#endif // filesystemwatcher
            return icon;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == QFileSystemModelPrivate::SizeColumn)
            return QVariant(Qt::AlignTrailing | Qt::AlignVCenter);
        break;
    case FilePermissions:
        int p = permissions(index);
        return p;
    }

    return QVariant();
}

/*!
    \internal
*/
QString QFileSystemModelPrivate::size(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    const QFileSystemNode *n = node(index);
    if (n->isDir()) {
#ifdef Q_OS_MAC
        return "--"_L1;
#else
        return ""_L1;
#endif
    // Windows   - ""
    // OS X      - "--"
    // Konqueror - "4 KB"
    // Nautilus  - "9 items" (the number of children)
    }
    return size(n->size());
}

QString QFileSystemModelPrivate::size(qint64 bytes)
{
    return QLocale::system().formattedDataSize(bytes);
}

/*!
    \internal
*/
QString QFileSystemModelPrivate::time(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
#if QT_CONFIG(datestring)
    return QLocale::system().toString(node(index)->lastModified(QTimeZone::LocalTime), QLocale::ShortFormat);
#else
    Q_UNUSED(index);
    return QString();
#endif
}

/*
    \internal
*/
QString QFileSystemModelPrivate::type(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    return node(index)->type();
}

/*!
    \internal
*/
QString QFileSystemModelPrivate::name(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    QFileSystemNode *dirNode = node(index);
    if (
#if QT_CONFIG(filesystemwatcher)
        fileInfoGatherer.resolveSymlinks() &&
#endif
        !resolvedSymLinks.isEmpty() && dirNode->isSymLink(/* ignoreNtfsSymLinks = */ true)) {
        QString fullPath = QDir::fromNativeSeparators(filePath(index));
        return resolvedSymLinks.value(fullPath, dirNode->fileName);
    }
    return dirNode->fileName;
}

/*!
    \internal
*/
QString QFileSystemModelPrivate::displayName(const QModelIndex &index) const
{
#if defined(Q_OS_WIN)
    QFileSystemNode *dirNode = node(index);
    if (!dirNode->volumeName.isEmpty())
        return dirNode->volumeName;
#endif
    return name(index);
}

/*!
    \internal
*/
QIcon QFileSystemModelPrivate::icon(const QModelIndex &index) const
{
    if (!index.isValid())
        return QIcon();
    return node(index)->icon();
}

/*!
    \reimp
*/
bool QFileSystemModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    Q_D(QFileSystemModel);
    if (!idx.isValid()
        || idx.column() != 0
        || role != Qt::EditRole
        || (flags(idx) & Qt::ItemIsEditable) == 0) {
        return false;
    }

    QString newName = value.toString();
    QString oldName = idx.data().toString();
    if (newName == oldName)
        return true;

    const QString parentPath = filePath(parent(idx));

    if (newName.isEmpty() || QDir::toNativeSeparators(newName).contains(QDir::separator()))
        return false;

#if QT_CONFIG(filesystemwatcher) && defined(Q_OS_WIN)
    // QTBUG-65683: Remove file system watchers prior to renaming to prevent
    // failure due to locked files on Windows.
    const QStringList watchedPaths = d->unwatchPathsAt(idx);
#endif // filesystemwatcher && Q_OS_WIN
    if (!QDir(parentPath).rename(oldName, newName)) {
#if QT_CONFIG(filesystemwatcher) && defined(Q_OS_WIN)
        d->watchPaths(watchedPaths);
#endif
        return false;
    } else {
        /*
            *After re-naming something we don't want the selection to change*
            - can't remove rows and later insert
            - can't quickly remove and insert
            - index pointer can't change because treeview doesn't use persistent index's

            - if this get any more complicated think of changing it to just
              use layoutChanged
         */

        QFileSystemModelPrivate::QFileSystemNode *indexNode = d->node(idx);
        QFileSystemModelPrivate::QFileSystemNode *parentNode = indexNode->parent;
        int visibleLocation = parentNode->visibleLocation(parentNode->children.value(indexNode->fileName)->fileName);

        parentNode->visibleChildren.removeAt(visibleLocation);
        std::unique_ptr<QFileSystemModelPrivate::QFileSystemNode> nodeToRename(parentNode->children.take(oldName));
        nodeToRename->fileName = newName;
        nodeToRename->parent = parentNode;
#if QT_CONFIG(filesystemwatcher)
        nodeToRename->populate(d->fileInfoGatherer.getInfo(QFileInfo(parentPath, newName)));
#endif
        nodeToRename->isVisible = true;
        parentNode->children[newName] = nodeToRename.release();
        parentNode->visibleChildren.insert(visibleLocation, newName);

        d->delayedSort();
        emit fileRenamed(parentPath, oldName, newName);
    }
    return true;
}

/*!
    \reimp
*/
QVariant QFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        if (section == 0) {
            // ### TODO oh man this is ugly and doesn't even work all the way!
            // it is still 2 pixels off
            QImage pixmap(16, 1, QImage::Format_ARGB32_Premultiplied);
            pixmap.fill(Qt::transparent);
            return pixmap;
        }
        break;
    case Qt::TextAlignmentRole:
        return Qt::AlignLeft;
    }

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    QString returnValue;
    switch (section) {
    case QFileSystemModelPrivate::NameColumn:
        returnValue = tr("Name");
        break;
    case QFileSystemModelPrivate::SizeColumn:
        returnValue = tr("Size");
        break;
    case QFileSystemModelPrivate::TypeColumn:
        returnValue =
#ifdef Q_OS_MAC
                    tr("Kind", "Match OS X Finder");
#else
                    tr("Type", "All other platforms");
#endif
        break;
    // Windows   - Type
    // OS X      - Kind
    // Konqueror - File Type
    // Nautilus  - Type
    case QFileSystemModelPrivate::TimeColumn:
        returnValue = tr("Date Modified");
        break;
    default: return QVariant();
    }
    return returnValue;
}

/*!
    \reimp
*/
Qt::ItemFlags QFileSystemModel::flags(const QModelIndex &index) const
{
    Q_D(const QFileSystemModel);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.isValid())
        return flags;

    QFileSystemModelPrivate::QFileSystemNode *indexNode = d->node(index);
    if (d->nameFilterDisables && !d->passNameFilters(indexNode)) {
        flags &= ~Qt::ItemIsEnabled;
        // ### TODO you shouldn't be able to set this as the current item, task 119433
        return flags;
    }

    flags |= Qt::ItemIsDragEnabled;

    if (!indexNode->isDir())
        flags |= Qt::ItemNeverHasChildren;
    if (d->readOnly)
        return flags;
    if ((index.column() == 0) && indexNode->permissions() & QFile::WriteUser) {
        flags |= Qt::ItemIsEditable;
        if (indexNode->isDir())
            flags |= Qt::ItemIsDropEnabled;
    }
    return flags;
}

/*!
    \internal
*/
void QFileSystemModelPrivate::_q_performDelayedSort()
{
    Q_Q(QFileSystemModel);
    q->sort(sortColumn, sortOrder);
}


/*
    \internal
    Helper functor used by sort()
*/
class QFileSystemModelSorter
{
public:
    inline QFileSystemModelSorter(int column) : sortColumn(column)
    {
        naturalCompare.setNumericMode(true);
        naturalCompare.setCaseSensitivity(Qt::CaseInsensitive);
    }

    bool compareNodes(const QFileSystemModelPrivate::QFileSystemNode *l,
                    const QFileSystemModelPrivate::QFileSystemNode *r) const
    {
        switch (sortColumn) {
        case QFileSystemModelPrivate::NameColumn: {
#ifndef Q_OS_MAC
            // place directories before files
            bool left = l->isDir();
            bool right = r->isDir();
            if (left ^ right)
                return left;
#endif
            return naturalCompare.compare(l->fileName, r->fileName) < 0;
                }
        case QFileSystemModelPrivate::SizeColumn:
        {
            // Directories go first
            bool left = l->isDir();
            bool right = r->isDir();
            if (left ^ right)
                return left;

            qint64 sizeDifference = l->size() - r->size();
            if (sizeDifference == 0)
                return naturalCompare.compare(l->fileName, r->fileName) < 0;

            return sizeDifference < 0;
        }
        case QFileSystemModelPrivate::TypeColumn:
        {
            int compare = naturalCompare.compare(l->type(), r->type());
            if (compare == 0)
                return naturalCompare.compare(l->fileName, r->fileName) < 0;

            return compare < 0;
        }
        case QFileSystemModelPrivate::TimeColumn:
        {
            const QDateTime left = l->lastModified(QTimeZone::UTC);
            const QDateTime right = r->lastModified(QTimeZone::UTC);
            if (left == right)
                return naturalCompare.compare(l->fileName, r->fileName) < 0;

            return left < right;
        }
        }
        Q_ASSERT(false);
        return false;
    }

    bool operator()(const QFileSystemModelPrivate::QFileSystemNode *l,
                    const QFileSystemModelPrivate::QFileSystemNode *r) const
    {
        return compareNodes(l, r);
    }


private:
    QCollator naturalCompare;
    int sortColumn;
};

/*
    \internal

    Sort all of the children of parent
*/
void QFileSystemModelPrivate::sortChildren(int column, const QModelIndex &parent)
{
    Q_Q(QFileSystemModel);
    QFileSystemModelPrivate::QFileSystemNode *indexNode = node(parent);
    if (indexNode->children.size() == 0)
        return;

    QList<QFileSystemModelPrivate::QFileSystemNode *> values;

    for (auto iterator = indexNode->children.constBegin(), cend = indexNode->children.constEnd(); iterator != cend; ++iterator) {
        if (filtersAcceptsNode(iterator.value())) {
            values.append(iterator.value());
        } else {
            iterator.value()->isVisible = false;
        }
    }
    QFileSystemModelSorter ms(column);
    std::sort(values.begin(), values.end(), ms);
    // First update the new visible list
    indexNode->visibleChildren.clear();
    //No more dirty item we reset our internal dirty index
    indexNode->dirtyChildrenIndex = -1;
    indexNode->visibleChildren.reserve(values.size());
    for (QFileSystemNode *node : std::as_const(values)) {
        indexNode->visibleChildren.append(node->fileName);
        node->isVisible = true;
    }

    if (!disableRecursiveSort) {
        for (int i = 0; i < q->rowCount(parent); ++i) {
            const QModelIndex childIndex = q->index(i, 0, parent);
            QFileSystemModelPrivate::QFileSystemNode *indexNode = node(childIndex);
            //Only do a recursive sort on visible nodes
            if (indexNode->isVisible)
                sortChildren(column, childIndex);
        }
    }
}

/*!
    \reimp
*/
void QFileSystemModel::sort(int column, Qt::SortOrder order)
{
    Q_D(QFileSystemModel);
    if (d->sortOrder == order && d->sortColumn == column && !d->forceSort)
        return;

    emit layoutAboutToBeChanged();
    QModelIndexList oldList = persistentIndexList();
    QList<QPair<QFileSystemModelPrivate::QFileSystemNode *, int>> oldNodes;
    oldNodes.reserve(oldList.size());
    for (const QModelIndex &oldNode : oldList) {
        QPair<QFileSystemModelPrivate::QFileSystemNode*, int> pair(d->node(oldNode), oldNode.column());
        oldNodes.append(pair);
    }

    if (!(d->sortColumn == column && d->sortOrder != order && !d->forceSort)) {
        //we sort only from where we are, don't need to sort all the model
        d->sortChildren(column, index(rootPath()));
        d->sortColumn = column;
        d->forceSort = false;
    }
    d->sortOrder = order;

    QModelIndexList newList;
    newList.reserve(oldNodes.size());
    for (const auto &[node, col]: std::as_const(oldNodes))
        newList.append(d->index(node, col));

    changePersistentIndexList(oldList, newList);
    emit layoutChanged();
}

/*!
    Returns a list of MIME types that can be used to describe a list of items
    in the model.
*/
QStringList QFileSystemModel::mimeTypes() const
{
    return QStringList("text/uri-list"_L1);
}

/*!
    Returns an object that contains a serialized description of the specified
    \a indexes. The format used to describe the items corresponding to the
    indexes is obtained from the mimeTypes() function.

    If the list of indexes is empty, \nullptr is returned rather than a
    serialized empty list.
*/
QMimeData *QFileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
        if ((*it).column() == QFileSystemModelPrivate::NameColumn)
            urls << QUrl::fromLocalFile(filePath(*it));
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

/*!
    Handles the \a data supplied by a drag and drop operation that ended with
    the given \a action over the row in the model specified by the \a row and
    \a column and by the \a parent index. Returns true if the operation was
    successful.

    \sa supportedDropActions()
*/
bool QFileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                             int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    if (!parent.isValid() || isReadOnly())
        return false;

    bool success = true;
    QString to = filePath(parent) + QDir::separator();

    QList<QUrl> urls = data->urls();
    QList<QUrl>::const_iterator it = urls.constBegin();

    switch (action) {
    case Qt::CopyAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::copy(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::LinkAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::link(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::MoveAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::rename(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    default:
        return false;
    }

    return success;
}

/*!
    \reimp
*/
Qt::DropActions QFileSystemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

/*!
    \reimp
*/
QHash<int, QByteArray> QFileSystemModel::roleNames() const
{
    auto ret = QAbstractItemModel::roleNames();
    ret.insert(QFileSystemModel::FileIconRole,
               QByteArrayLiteral("fileIcon")); // == Qt::decoration
    ret.insert(QFileSystemModel::FilePathRole, QByteArrayLiteral("filePath"));
    ret.insert(QFileSystemModel::FileNameRole, QByteArrayLiteral("fileName"));
    ret.insert(QFileSystemModel::FilePermissions, QByteArrayLiteral("filePermissions"));
    return ret;
}

/*!
    \enum QFileSystemModel::Option
    \since 5.14

    \value DontWatchForChanges Do not add file watchers to the paths.
    This reduces overhead when using the model for simple tasks
    like line edit completion.

    \value DontResolveSymlinks Don't resolve symlinks in the file
    system model. By default, symlinks are resolved.

    \value DontUseCustomDirectoryIcons Always use the default directory icon.
    Some platforms allow the user to set a different icon. Custom icon lookup
    causes a big performance impact over network or removable drives.
    This sets the QFileIconProvider::DontUseCustomDirectoryIcons
    option in the icon provider accordingly.

    \sa resolveSymlinks
*/

/*!
    \since 5.14
    Sets the given \a option to be enabled if \a on is true; otherwise,
    clears the given \a option.

    Options should be set before changing properties.

    \sa options, testOption()
*/
void QFileSystemModel::setOption(Option option, bool on)
{
    QFileSystemModel::Options previousOptions = options();
    setOptions(previousOptions.setFlag(option, on));
}

/*!
    \since 5.14

    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool QFileSystemModel::testOption(Option option) const
{
    return options().testFlag(option);
}

/*!
    \property QFileSystemModel::options
    \brief the various options that affect the model
    \since 5.14

    By default, all options are disabled.

    Options should be set before changing properties.

    \sa setOption(), testOption()
*/
void QFileSystemModel::setOptions(Options options)
{
    const Options changed = (options ^ QFileSystemModel::options());

    if (changed.testFlag(DontResolveSymlinks))
        setResolveSymlinks(!options.testFlag(DontResolveSymlinks));

#if QT_CONFIG(filesystemwatcher)
    Q_D(QFileSystemModel);
    if (changed.testFlag(DontWatchForChanges))
        d->fileInfoGatherer.setWatching(!options.testFlag(DontWatchForChanges));
#endif

    if (changed.testFlag(DontUseCustomDirectoryIcons)) {
        if (auto provider = iconProvider()) {
            QAbstractFileIconProvider::Options providerOptions = provider->options();
            providerOptions.setFlag(QAbstractFileIconProvider::DontUseCustomDirectoryIcons,
                                    options.testFlag(QFileSystemModel::DontUseCustomDirectoryIcons));
            provider->setOptions(providerOptions);
        } else {
            qWarning("Setting QFileSystemModel::DontUseCustomDirectoryIcons has no effect when no provider is used");
        }
    }
}

QFileSystemModel::Options QFileSystemModel::options() const
{
    QFileSystemModel::Options result;
    result.setFlag(DontResolveSymlinks, !resolveSymlinks());
#if QT_CONFIG(filesystemwatcher)
    Q_D(const QFileSystemModel);
    result.setFlag(DontWatchForChanges, !d->fileInfoGatherer.isWatching());
#else
    result.setFlag(DontWatchForChanges);
#endif
    if (auto provider = iconProvider()) {
        result.setFlag(DontUseCustomDirectoryIcons,
                       provider->options().testFlag(QAbstractFileIconProvider::DontUseCustomDirectoryIcons));
    }
    return result;
}

/*!
    Returns the path of the item stored in the model under the
    \a index given.
*/
QString QFileSystemModel::filePath(const QModelIndex &index) const
{
    Q_D(const QFileSystemModel);
    QString fullPath = d->filePath(index);
    QFileSystemModelPrivate::QFileSystemNode *dirNode = d->node(index);
    if (dirNode->isSymLink()
#if QT_CONFIG(filesystemwatcher)
        && d->fileInfoGatherer.resolveSymlinks()
#endif
        && d->resolvedSymLinks.contains(fullPath)
        && dirNode->isDir()) {
        QFileInfo fullPathInfo(dirNode->fileInfo());
        if (!dirNode->hasInformation())
            fullPathInfo = QFileInfo(fullPath);
        QString canonicalPath = fullPathInfo.canonicalFilePath();
        auto *canonicalNode = d->node(fullPathInfo.canonicalFilePath(), false);
        QFileInfo resolvedInfo = canonicalNode->fileInfo();
        if (!canonicalNode->hasInformation())
            resolvedInfo = QFileInfo(canonicalPath);
        if (resolvedInfo.exists())
            return resolvedInfo.filePath();
    }
    return fullPath;
}

QString QFileSystemModelPrivate::filePath(const QModelIndex &index) const
{
    Q_Q(const QFileSystemModel);
    Q_UNUSED(q);
    if (!index.isValid())
        return QString();
    Q_ASSERT(index.model() == q);

    QStringList path;
    QModelIndex idx = index;
    while (idx.isValid()) {
        QFileSystemModelPrivate::QFileSystemNode *dirNode = node(idx);
        if (dirNode)
            path.prepend(dirNode->fileName);
        idx = idx.parent();
    }
    QString fullPath = QDir::fromNativeSeparators(path.join(QDir::separator()));
#if !defined(Q_OS_WIN)
    if ((fullPath.size() > 2) && fullPath[0] == u'/' && fullPath[1] == u'/')
        fullPath = fullPath.mid(1);
#else
    if (fullPath.length() == 2 && fullPath.endsWith(u':'))
        fullPath.append(u'/');
#endif
    return fullPath;
}

/*!
    Create a directory with the \a name in the \a parent model index.
*/
QModelIndex QFileSystemModel::mkdir(const QModelIndex &parent, const QString &name)
{
    Q_D(QFileSystemModel);
    if (!parent.isValid())
        return parent;

    QDir dir(filePath(parent));
    if (!dir.mkdir(name))
        return QModelIndex();
    QFileSystemModelPrivate::QFileSystemNode *parentNode = d->node(parent);
    d->addNode(parentNode, name, QFileInfo());
    Q_ASSERT(parentNode->children.contains(name));
    QFileSystemModelPrivate::QFileSystemNode *node = parentNode->children[name];
#if QT_CONFIG(filesystemwatcher)
    node->populate(d->fileInfoGatherer.getInfo(QFileInfo(dir.absolutePath() + QDir::separator() + name)));
#endif
    d->addVisibleFiles(parentNode, QStringList(name));
    return d->index(node);
}

/*!
    Returns the complete OR-ed together combination of QFile::Permission for the \a index.
 */
QFile::Permissions QFileSystemModel::permissions(const QModelIndex &index) const
{
    Q_D(const QFileSystemModel);
    return d->node(index)->permissions();
}

/*!
    Sets the directory that is being watched by the model to \a newPath by
    installing a \l{QFileSystemWatcher}{file system watcher} on it. Any
    changes to files and directories within this directory will be
    reflected in the model.

    If the path is changed, the rootPathChanged() signal will be emitted.

    \note This function does not change the structure of the model or
    modify the data available to views. In other words, the "root" of
    the model is \e not changed to include only files and directories
    within the directory specified by \a newPath in the file system.
  */
QModelIndex QFileSystemModel::setRootPath(const QString &newPath)
{
    Q_D(QFileSystemModel);
#ifdef Q_OS_WIN
#ifdef Q_OS_WIN32
    QString longNewPath = qt_GetLongPathName(newPath);
#else
    QString longNewPath = QDir::fromNativeSeparators(newPath);
#endif
#else
    QString longNewPath = newPath;
#endif
    //we remove .. and . from the given path if exist
    if (!newPath.isEmpty())
        longNewPath = QDir::cleanPath(longNewPath);

    d->setRootPath = true;

    //user don't ask for the root path ("") but the conversion failed
    if (!newPath.isEmpty() && longNewPath.isEmpty())
        return d->index(rootPath());

    if (d->rootDir.path() == longNewPath)
        return d->index(rootPath());

    auto node = d->node(longNewPath);
    QFileInfo newPathInfo;
    if (node && node->hasInformation())
        newPathInfo = node->fileInfo();
    else
        newPathInfo = QFileInfo(longNewPath);

    bool showDrives = (longNewPath.isEmpty() || longNewPath == QFileSystemModelPrivate::myComputer());
    if (!showDrives && !newPathInfo.exists())
        return d->index(rootPath());

    //We remove the watcher on the previous path
    if (!rootPath().isEmpty() && rootPath() != "."_L1) {
        //This remove the watcher for the old rootPath
#if QT_CONFIG(filesystemwatcher)
        d->fileInfoGatherer.removePath(rootPath());
#endif
        //This line "marks" the node as dirty, so the next fetchMore
        //call on the path will ask the gatherer to install a watcher again
        //But it doesn't re-fetch everything
        d->node(rootPath())->populatedChildren = false;
    }

    // We have a new valid root path
    d->rootDir = QDir(longNewPath);
    QModelIndex newRootIndex;
    if (showDrives) {
        // otherwise dir will become '.'
        d->rootDir.setPath(""_L1);
    } else {
        newRootIndex = d->index(d->rootDir.path());
    }
    fetchMore(newRootIndex);
    emit rootPathChanged(longNewPath);
    d->forceSort = true;
    d->delayedSort();
    return newRootIndex;
}

/*!
    The currently set root path

    \sa rootDirectory()
*/
QString QFileSystemModel::rootPath() const
{
    Q_D(const QFileSystemModel);
    return d->rootDir.path();
}

/*!
    The currently set directory

    \sa rootPath()
*/
QDir QFileSystemModel::rootDirectory() const
{
    Q_D(const QFileSystemModel);
    QDir dir(d->rootDir);
    dir.setNameFilters(nameFilters());
    dir.setFilter(filter());
    return dir;
}

/*!
    Sets the \a provider of file icons for the directory model.
*/
void QFileSystemModel::setIconProvider(QAbstractFileIconProvider *provider)
{
    Q_D(QFileSystemModel);
#if QT_CONFIG(filesystemwatcher)
    d->fileInfoGatherer.setIconProvider(provider);
#endif
    d->root.updateIcon(provider, QString());
}

/*!
    Returns the file icon provider for this directory model.
*/
QAbstractFileIconProvider *QFileSystemModel::iconProvider() const
{
#if QT_CONFIG(filesystemwatcher)
    Q_D(const QFileSystemModel);
    return d->fileInfoGatherer.iconProvider();
#else
    return 0;
#endif
}

/*!
    Sets the directory model's filter to that specified by \a filters.

    Note that the filter you set should always include the QDir::AllDirs enum value,
    otherwise QFileSystemModel won't be able to read the directory structure.

    \sa QDir::Filters
*/
void QFileSystemModel::setFilter(QDir::Filters filters)
{
    Q_D(QFileSystemModel);
    if (d->filters == filters)
        return;
    const bool changingCaseSensitivity =
        filters.testFlag(QDir::CaseSensitive) != d->filters.testFlag(QDir::CaseSensitive);
    d->filters = filters;
    if (changingCaseSensitivity)
        d->rebuildNameFilterRegexps();
    d->forceSort = true;
    d->delayedSort();
}

/*!
    Returns the filter specified for the directory model.

    If a filter has not been set, the default filter is QDir::AllEntries |
    QDir::NoDotAndDotDot | QDir::AllDirs.

    \sa QDir::Filters
*/
QDir::Filters QFileSystemModel::filter() const
{
    Q_D(const QFileSystemModel);
    return d->filters;
}

/*!
    \property QFileSystemModel::resolveSymlinks
    \brief Whether the directory model should resolve symbolic links

    This is only relevant on Windows.

    By default, this property is \c true.

    \sa QFileSystemModel::Options
*/
void QFileSystemModel::setResolveSymlinks(bool enable)
{
#if QT_CONFIG(filesystemwatcher)
    Q_D(QFileSystemModel);
    d->fileInfoGatherer.setResolveSymlinks(enable);
#else
    Q_UNUSED(enable);
#endif
}

bool QFileSystemModel::resolveSymlinks() const
{
#if QT_CONFIG(filesystemwatcher)
    Q_D(const QFileSystemModel);
    return d->fileInfoGatherer.resolveSymlinks();
#else
    return false;
#endif
}

/*!
    \property QFileSystemModel::readOnly
    \brief Whether the directory model allows writing to the file system

    If this property is set to false, the directory model will allow renaming, copying
    and deleting of files and directories.

    This property is \c true by default
*/
void QFileSystemModel::setReadOnly(bool enable)
{
    Q_D(QFileSystemModel);
    d->readOnly = enable;
}

bool QFileSystemModel::isReadOnly() const
{
    Q_D(const QFileSystemModel);
    return d->readOnly;
}

/*!
    \property QFileSystemModel::nameFilterDisables
    \brief Whether files that don't pass the name filter are hidden or disabled

    This property is \c true by default
*/
void QFileSystemModel::setNameFilterDisables(bool enable)
{
    Q_D(QFileSystemModel);
    if (d->nameFilterDisables == enable)
        return;
    d->nameFilterDisables = enable;
    d->forceSort = true;
    d->delayedSort();
}

bool QFileSystemModel::nameFilterDisables() const
{
    Q_D(const QFileSystemModel);
    return d->nameFilterDisables;
}

/*!
    Sets the name \a filters to apply against the existing files.
*/
void QFileSystemModel::setNameFilters(const QStringList &filters)
{
#if QT_CONFIG(regularexpression)
    Q_D(QFileSystemModel);

    if (!d->bypassFilters.isEmpty()) {
        // update the bypass filter to only bypass the stuff that must be kept around
        d->bypassFilters.clear();
        // We guarantee that rootPath will stick around
        QPersistentModelIndex root(index(rootPath()));
        const QModelIndexList persistentList = persistentIndexList();
        for (const auto &persistentIndex : persistentList) {
            QFileSystemModelPrivate::QFileSystemNode *node = d->node(persistentIndex);
            while (node) {
                if (d->bypassFilters.contains(node))
                    break;
                if (node->isDir())
                    d->bypassFilters[node] = true;
                node = node->parent;
            }
        }
    }

    d->nameFilters = filters;
    d->rebuildNameFilterRegexps();
    d->forceSort = true;
    d->delayedSort();
#else
    Q_UNUSED(filters);
#endif
}

/*!
    Returns a list of filters applied to the names in the model.
*/
QStringList QFileSystemModel::nameFilters() const
{
#if QT_CONFIG(regularexpression)
    Q_D(const QFileSystemModel);
    return d->nameFilters;
#else
    return QStringList();
#endif
}

/*!
    \reimp
*/
bool QFileSystemModel::event(QEvent *event)
{
#if QT_CONFIG(filesystemwatcher)
    Q_D(QFileSystemModel);
    if (event->type() == QEvent::LanguageChange) {
        d->root.retranslateStrings(d->fileInfoGatherer.iconProvider(), QString());
        return true;
    }
#endif
    return QAbstractItemModel::event(event);
}

bool QFileSystemModel::rmdir(const QModelIndex &aindex)
{
    QString path = filePath(aindex);
    const bool success = QDir().rmdir(path);
#if QT_CONFIG(filesystemwatcher)
    if (success) {
        QFileSystemModelPrivate * d = const_cast<QFileSystemModelPrivate*>(d_func());
        d->fileInfoGatherer.removePath(path);
    }
#endif
    return success;
}

/*!
     \internal

    Performed quick listing and see if any files have been added or removed,
    then fetch more information on visible files.
 */
void QFileSystemModelPrivate::_q_directoryChanged(const QString &directory, const QStringList &files)
{
    QFileSystemModelPrivate::QFileSystemNode *parentNode = node(directory, false);
    if (parentNode->children.size() == 0)
        return;
    QStringList toRemove;
    QStringList newFiles = files;
    std::sort(newFiles.begin(), newFiles.end());
    for (auto i = parentNode->children.constBegin(), cend = parentNode->children.constEnd(); i != cend; ++i) {
        QStringList::iterator iterator = std::lower_bound(newFiles.begin(), newFiles.end(), i.value()->fileName);
        if ((iterator == newFiles.end()) || (i.value()->fileName < *iterator))
            toRemove.append(i.value()->fileName);
    }
    for (int i = 0 ; i < toRemove.size() ; ++i )
        removeNode(parentNode, toRemove[i]);
}

#if defined(Q_OS_WIN)
static QString volumeName(const QString &path)
{
    IShellItem *item = nullptr;
    const QString native = QDir::toNativeSeparators(path);
    HRESULT hr = SHCreateItemFromParsingName(reinterpret_cast<const wchar_t *>(native.utf16()),
                                             nullptr, IID_IShellItem,
                                             reinterpret_cast<void **>(&item));
    if (FAILED(hr))
        return QString();
    LPWSTR name = nullptr;
    hr = item->GetDisplayName(SIGDN_NORMALDISPLAY, &name);
    if (FAILED(hr))
        return QString();
    QString result = QString::fromWCharArray(name);
    CoTaskMemFree(name);
    item->Release();
    return result;
}
#endif // Q_OS_WIN

/*!
    \internal

    Adds a new file to the children of parentNode

    *WARNING* this will change the count of children
*/
QFileSystemModelPrivate::QFileSystemNode* QFileSystemModelPrivate::addNode(QFileSystemNode *parentNode, const QString &fileName, const QFileInfo& info)
{
    // In the common case, itemLocation == count() so check there first
    QFileSystemModelPrivate::QFileSystemNode *node = new QFileSystemModelPrivate::QFileSystemNode(fileName, parentNode);
#if QT_CONFIG(filesystemwatcher)
    node->populate(info);
#else
    Q_UNUSED(info);
#endif
#if defined(Q_OS_WIN)
    //The parentNode is "" so we are listing the drives
    if (parentNode->fileName.isEmpty())
        node->volumeName = volumeName(fileName);
#endif
    Q_ASSERT(!parentNode->children.contains(fileName));
    parentNode->children.insert(fileName, node);
    return node;
}

/*!
    \internal

    File at parentNode->children(itemLocation) has been removed, remove from the lists
    and emit signals if necessary

    *WARNING* this will change the count of children and could change visibleChildren
 */
void QFileSystemModelPrivate::removeNode(QFileSystemModelPrivate::QFileSystemNode *parentNode, const QString& name)
{
    Q_Q(QFileSystemModel);
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);

    int vLocation = parentNode->visibleLocation(name);
    if (vLocation >= 0 && !indexHidden)
        q->beginRemoveRows(parent, translateVisibleLocation(parentNode, vLocation),
                                       translateVisibleLocation(parentNode, vLocation));
    QFileSystemNode * node = parentNode->children.take(name);
    delete node;
    // cleanup sort files after removing rather then re-sorting which is O(n)
    if (vLocation >= 0)
        parentNode->visibleChildren.removeAt(vLocation);
    if (vLocation >= 0 && !indexHidden)
        q->endRemoveRows();
}

/*!
    \internal

    File at parentNode->children(itemLocation) was not visible before, but now should be
    and emit signals if necessary.

    *WARNING* this will change the visible count
 */
void QFileSystemModelPrivate::addVisibleFiles(QFileSystemNode *parentNode, const QStringList &newFiles)
{
    Q_Q(QFileSystemModel);
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);
    if (!indexHidden) {
        q->beginInsertRows(parent, parentNode->visibleChildren.size() , parentNode->visibleChildren.size() + newFiles.size() - 1);
    }

    if (parentNode->dirtyChildrenIndex == -1)
        parentNode->dirtyChildrenIndex = parentNode->visibleChildren.size();

    for (const auto &newFile : newFiles) {
        parentNode->visibleChildren.append(newFile);
        parentNode->children.value(newFile)->isVisible = true;
    }
    if (!indexHidden)
      q->endInsertRows();
}

/*!
    \internal

    File was visible before, but now should NOT be

    *WARNING* this will change the visible count
 */
void QFileSystemModelPrivate::removeVisibleFile(QFileSystemNode *parentNode, int vLocation)
{
    Q_Q(QFileSystemModel);
    if (vLocation == -1)
        return;
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);
    if (!indexHidden)
        q->beginRemoveRows(parent, translateVisibleLocation(parentNode, vLocation),
                                       translateVisibleLocation(parentNode, vLocation));
    parentNode->children.value(parentNode->visibleChildren.at(vLocation))->isVisible = false;
    parentNode->visibleChildren.removeAt(vLocation);
    if (!indexHidden)
        q->endRemoveRows();
}

/*!
    \internal

    The thread has received new information about files,
    update and emit dataChanged if it has actually changed.
 */
void QFileSystemModelPrivate::_q_fileSystemChanged(const QString &path,
                                                   const QList<QPair<QString, QFileInfo>> &updates)
{
#if QT_CONFIG(filesystemwatcher)
    Q_Q(QFileSystemModel);
    QList<QString> rowsToUpdate;
    QStringList newFiles;
    QFileSystemModelPrivate::QFileSystemNode *parentNode = node(path, false);
    QModelIndex parentIndex = index(parentNode);
    for (const auto &update : updates) {
        QString fileName = update.first;
        Q_ASSERT(!fileName.isEmpty());
        QExtendedInformation info = fileInfoGatherer.getInfo(update.second);
        bool previouslyHere = parentNode->children.contains(fileName);
        if (!previouslyHere) {
            addNode(parentNode, fileName, info.fileInfo());
        }
        QFileSystemModelPrivate::QFileSystemNode * node = parentNode->children.value(fileName);
        bool isCaseSensitive = parentNode->caseSensitive();
        if (isCaseSensitive) {
            if (node->fileName != fileName)
                continue;
        } else {
            if (QString::compare(node->fileName,fileName,Qt::CaseInsensitive) != 0)
                continue;
        }
        if (isCaseSensitive) {
            Q_ASSERT(node->fileName == fileName);
        } else {
            node->fileName = fileName;
        }

        if (*node != info ) {
            node->populate(info);
            bypassFilters.remove(node);
            // brand new information.
            if (filtersAcceptsNode(node)) {
                if (!node->isVisible) {
                    newFiles.append(fileName);
                } else {
                    rowsToUpdate.append(fileName);
                }
            } else {
                if (node->isVisible) {
                    int visibleLocation = parentNode->visibleLocation(fileName);
                    removeVisibleFile(parentNode, visibleLocation);
                } else {
                    // The file is not visible, don't do anything
                }
            }
        }
    }

    // bundle up all of the changed signals into as few as possible.
    std::sort(rowsToUpdate.begin(), rowsToUpdate.end());
    QString min;
    QString max;
    for (const QString &value : std::as_const(rowsToUpdate)) {
        //##TODO is there a way to bundle signals with QString as the content of the list?
        /*if (min.isEmpty()) {
            min = value;
            if (i != rowsToUpdate.count() - 1)
                continue;
        }
        if (i != rowsToUpdate.count() - 1) {
            if ((value == min + 1 && max.isEmpty()) || value == max + 1) {
                max = value;
                continue;
            }
        }*/
        max = value;
        min = value;
        int visibleMin = parentNode->visibleLocation(min);
        int visibleMax = parentNode->visibleLocation(max);
        if (visibleMin >= 0
            && visibleMin < parentNode->visibleChildren.size()
            && parentNode->visibleChildren.at(visibleMin) == min
            && visibleMax >= 0) {
            // don't use NumColumns here, a subclass might override columnCount
            const int lastColumn = q->columnCount(parentIndex) - 1;
            const QModelIndex top = q->index(translateVisibleLocation(parentNode, visibleMin),
                                             QFileSystemModelPrivate::NameColumn, parentIndex);
            const QModelIndex bottom = q->index(translateVisibleLocation(parentNode, visibleMax),
                                                lastColumn, parentIndex);
            // We document that emitting dataChanged with indexes that don't have the
            // same parent is undefined behavior.
            Q_ASSERT(bottom.parent() == top.parent());
            emit q->dataChanged(top, bottom);
        }

        /*min = QString();
        max = QString();*/
    }

    if (newFiles.size() > 0) {
        addVisibleFiles(parentNode, newFiles);
    }

    if (newFiles.size() > 0 || (sortColumn != 0 && rowsToUpdate.size() > 0)) {
        forceSort = true;
        delayedSort();
    }
#else
    Q_UNUSED(path);
    Q_UNUSED(updates);
#endif // filesystemwatcher
}

/*!
    \internal
*/
void QFileSystemModelPrivate::_q_resolvedName(const QString &fileName, const QString &resolvedName)
{
    resolvedSymLinks[fileName] = resolvedName;
}

#if QT_CONFIG(filesystemwatcher) && defined(Q_OS_WIN)
// Remove file system watchers at/below the index and return a list of previously
// watched files. This should be called prior to operations like rename/remove
// which might fail due to watchers on platforms like Windows. The watchers
// should be restored on failure.
QStringList QFileSystemModelPrivate::unwatchPathsAt(const QModelIndex &index)
{
    const QFileSystemModelPrivate::QFileSystemNode *indexNode = node(index);
    if (indexNode == nullptr)
        return QStringList();
    const Qt::CaseSensitivity caseSensitivity = indexNode->caseSensitive()
        ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const QString path = indexNode->fileInfo().absoluteFilePath();

    QStringList result;
    const auto filter = [path, caseSensitivity] (const QString &watchedPath)
    {
        const int pathSize = path.size();
        if (pathSize == watchedPath.size()) {
            return path.compare(watchedPath, caseSensitivity) == 0;
        } else if (watchedPath.size() > pathSize) {
            return watchedPath.at(pathSize) == u'/'
                && watchedPath.startsWith(path, caseSensitivity);
        }
        return false;
    };

    const QStringList &watchedFiles = fileInfoGatherer.watchedFiles();
    std::copy_if(watchedFiles.cbegin(), watchedFiles.cend(),
                 std::back_inserter(result), filter);

    const QStringList &watchedDirectories = fileInfoGatherer.watchedDirectories();
    std::copy_if(watchedDirectories.cbegin(), watchedDirectories.cend(),
                 std::back_inserter(result), filter);

    fileInfoGatherer.unwatchPaths(result);
    return result;
}
#endif // filesystemwatcher && Q_OS_WIN

QFileSystemModelPrivate::QFileSystemModelPrivate() = default;

QFileSystemModelPrivate::~QFileSystemModelPrivate() = default;

/*!
    \internal
*/
void QFileSystemModelPrivate::init()
{
    Q_Q(QFileSystemModel);

    delayedSortTimer.setSingleShot(true);

    qRegisterMetaType<QList<QPair<QString, QFileInfo>>>();
#if QT_CONFIG(filesystemwatcher)
    q->connect(&fileInfoGatherer, SIGNAL(newListOfFiles(QString,QStringList)),
               q, SLOT(_q_directoryChanged(QString,QStringList)));
    q->connect(&fileInfoGatherer, SIGNAL(updates(QString,QList<std::pair<QString,QFileInfo>>)), q,
               SLOT(_q_fileSystemChanged(QString,QList<std::pair<QString,QFileInfo>>)));
    q->connect(&fileInfoGatherer, SIGNAL(nameResolved(QString,QString)),
            q, SLOT(_q_resolvedName(QString,QString)));
    q->connect(&fileInfoGatherer, SIGNAL(directoryLoaded(QString)),
               q, SIGNAL(directoryLoaded(QString)));
#endif // filesystemwatcher
    q->connect(&delayedSortTimer, SIGNAL(timeout()), q, SLOT(_q_performDelayedSort()), Qt::QueuedConnection);
}

/*!
    \internal

    Returns \c false if node doesn't pass the filters otherwise true

    QDir::Modified is not supported
    QDir::Drives is not supported
*/
bool QFileSystemModelPrivate::filtersAcceptsNode(const QFileSystemNode *node) const
{
    // always accept drives
    if (node->parent == &root || bypassFilters.contains(node))
        return true;

    // If we don't know anything yet don't accept it
    if (!node->hasInformation())
        return false;

    const bool filterPermissions = ((filters & QDir::PermissionMask)
                                   && (filters & QDir::PermissionMask) != QDir::PermissionMask);
    const bool hideDirs          = !(filters & (QDir::Dirs | QDir::AllDirs));
    const bool hideFiles         = !(filters & QDir::Files);
    const bool hideReadable      = !(!filterPermissions || (filters & QDir::Readable));
    const bool hideWritable      = !(!filterPermissions || (filters & QDir::Writable));
    const bool hideExecutable    = !(!filterPermissions || (filters & QDir::Executable));
    const bool hideHidden        = !(filters & QDir::Hidden);
    const bool hideSystem        = !(filters & QDir::System);
    const bool hideSymlinks      = (filters & QDir::NoSymLinks);
    const bool hideDot           = (filters & QDir::NoDot);
    const bool hideDotDot        = (filters & QDir::NoDotDot);

    // Note that we match the behavior of entryList and not QFileInfo on this.
    bool isDot    = (node->fileName == "."_L1);
    bool isDotDot = (node->fileName == ".."_L1);
    if (   (hideHidden && !(isDot || isDotDot) && node->isHidden())
        || (hideSystem && node->isSystem())
        || (hideDirs && node->isDir())
        || (hideFiles && node->isFile())
        || (hideSymlinks && node->isSymLink())
        || (hideReadable && node->isReadable())
        || (hideWritable && node->isWritable())
        || (hideExecutable && node->isExecutable())
        || (hideDot && isDot)
        || (hideDotDot && isDotDot))
        return false;

    return nameFilterDisables || passNameFilters(node);
}

/*
    \internal

    Returns \c true if node passes the name filters and should be visible.
 */
bool QFileSystemModelPrivate::passNameFilters(const QFileSystemNode *node) const
{
#if QT_CONFIG(regularexpression)
    if (nameFilters.isEmpty())
        return true;

    // Check the name regularexpression filters
    if (!(node->isDir() && (filters & QDir::AllDirs))) {
        const auto matchesNodeFileName = [node](const QRegularExpression &re)
        {
            return node->fileName.contains(re);
        };
        return std::any_of(nameFiltersRegexps.begin(),
                           nameFiltersRegexps.end(),
                           matchesNodeFileName);
    }
#else
    Q_UNUSED(node);
#endif
    return true;
}

#if QT_CONFIG(regularexpression)
void QFileSystemModelPrivate::rebuildNameFilterRegexps()
{
    nameFiltersRegexps.clear();
    nameFiltersRegexps.reserve(nameFilters.size());
    const auto cs = (filters & QDir::CaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const auto convertWildcardToRegexp = [cs](const QString &nameFilter)
    {
        return QRegularExpression::fromWildcard(nameFilter, cs);
    };
    std::transform(nameFilters.constBegin(),
                   nameFilters.constEnd(),
                   std::back_inserter(nameFiltersRegexps),
                   convertWildcardToRegexp);
}
#endif

QT_END_NAMESPACE

#include "moc_qfilesystemmodel.cpp"
