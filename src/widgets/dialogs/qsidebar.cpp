// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsidebar_p.h"

#include <qaction.h>
#include <qurl.h>
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#include <qmimedata.h>
#include <qevent.h>
#include <qdebug.h>
#include <qfilesystemmodel.h>
#include <qabstractfileiconprovider.h>
#include <qfiledialog.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void QSideBarDelegate::initStyleOption(QStyleOptionViewItem *option,
                                         const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option,index);
    QVariant value = index.data(QUrlModel::EnabledRole);
    if (value.isValid()) {
        //If the bookmark/entry is not enabled then we paint it in gray
        if (!qvariant_cast<bool>(value))
            option->state &= ~QStyle::State_Enabled;
    }
}

/*!
    \internal
    \class QUrlModel
    QUrlModel lets you have indexes from a QFileSystemModel to a list.  When QFileSystemModel
    changes them QUrlModel will automatically update.

    Example usage: File dialog sidebar and combo box
 */
QUrlModel::QUrlModel(QObject *parent) : QStandardItemModel(parent), showFullPath(false), fileSystemModel(nullptr)
{
}

/*!
    \reimp
*/
QStringList QUrlModel::mimeTypes() const
{
    return QStringList("text/uri-list"_L1);
}

/*!
    \reimp
*/
Qt::ItemFlags QUrlModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);
    if (index.isValid()) {
        flags &= ~Qt::ItemIsEditable;
        // ### some future version could support "moving" urls onto a folder
        flags &= ~Qt::ItemIsDropEnabled;
    }

    if (index.data(Qt::DecorationRole).isNull())
        flags &= ~Qt::ItemIsEnabled;

    return flags;
}

/*!
    \reimp
*/
QMimeData *QUrlModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> list;
    for (const auto &index : indexes) {
        if (index.column() == 0)
           list.append(index.data(UrlRole).toUrl());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(list);
    return data;
}

#if QT_CONFIG(draganddrop)

/*!
    Decide based upon the data if it should be accepted or not

    We only accept dirs and not files
*/
bool QUrlModel::canDrop(QDragEnterEvent *event)
{
    if (!event->mimeData()->formats().contains(mimeTypes().constFirst()))
        return false;

    const QList<QUrl> list = event->mimeData()->urls();
    for (const auto &url : list) {
        const QModelIndex idx = fileSystemModel->index(url.toLocalFile());
        if (!fileSystemModel->isDir(idx))
            return false;
    }
    return true;
}

/*!
    \reimp
*/
bool QUrlModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column, const QModelIndex &parent)
{
    if (!data->formats().contains(mimeTypes().constFirst()))
        return false;
    Q_UNUSED(action);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    addUrls(data->urls(), row);
    return true;
}

#endif // QT_CONFIG(draganddrop)

/*!
    \reimp

    If the role is the UrlRole then handle otherwise just pass to QStandardItemModel
*/
bool QUrlModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (value.userType() == QMetaType::QUrl) {
        QUrl url = value.toUrl();
        QModelIndex dirIndex = fileSystemModel->index(url.toLocalFile());
        //On windows the popup display the "C:\", convert to nativeSeparators
        if (showFullPath)
            QStandardItemModel::setData(index, QDir::toNativeSeparators(fileSystemModel->data(dirIndex, QFileSystemModel::FilePathRole).toString()));
        else {
            QStandardItemModel::setData(index, QDir::toNativeSeparators(fileSystemModel->data(dirIndex, QFileSystemModel::FilePathRole).toString()), Qt::ToolTipRole);
            QStandardItemModel::setData(index, fileSystemModel->data(dirIndex).toString());
        }
        QStandardItemModel::setData(index, fileSystemModel->data(dirIndex, Qt::DecorationRole),
                                               Qt::DecorationRole);
        QStandardItemModel::setData(index, url, UrlRole);
        return true;
    }
    return QStandardItemModel::setData(index, value, role);
}

void QUrlModel::setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex)
{
    setData(index, url, UrlRole);
    if (url.path().isEmpty()) {
        setData(index, fileSystemModel->myComputer());
        setData(index, fileSystemModel->myComputer(Qt::DecorationRole), Qt::DecorationRole);
    } else {
        QString newName;
        if (showFullPath) {
            //On windows the popup display the "C:\", convert to nativeSeparators
            newName = QDir::toNativeSeparators(dirIndex.data(QFileSystemModel::FilePathRole).toString());
        } else {
            newName = dirIndex.data().toString();
        }

        QIcon newIcon = qvariant_cast<QIcon>(dirIndex.data(Qt::DecorationRole));
        if (!dirIndex.isValid()) {
            const QAbstractFileIconProvider *provider = fileSystemModel->iconProvider();
            if (provider)
                newIcon = provider->icon(QAbstractFileIconProvider::Folder);
            newName = QFileInfo(url.toLocalFile()).fileName();
            if (!invalidUrls.contains(url))
                invalidUrls.append(url);
            //The bookmark is invalid then we set to false the EnabledRole
            setData(index, false, EnabledRole);
        } else {
            //The bookmark is valid then we set to true the EnabledRole
            setData(index, true, EnabledRole);
        }

        // Make sure that we have at least 32x32 images
        const QSize size = newIcon.actualSize(QSize(32,32));
        if (size.width() < 32) {
            QPixmap smallPixmap = newIcon.pixmap(QSize(32, 32));
            newIcon.addPixmap(smallPixmap.scaledToWidth(32, Qt::SmoothTransformation));
        }

        if (index.data().toString() != newName)
            setData(index, newName);
        QIcon oldIcon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (oldIcon.cacheKey() != newIcon.cacheKey())
            setData(index, newIcon, Qt::DecorationRole);
    }
}

void QUrlModel::setUrls(const QList<QUrl> &list)
{
    removeRows(0, rowCount());
    invalidUrls.clear();
    watching.clear();
    addUrls(list, 0);
}

/*!
    Add urls \a list into the list at \a row.  If move then movie
    existing ones to row.

    \sa dropMimeData()
*/
void QUrlModel::addUrls(const QList<QUrl> &list, int row, bool move)
{
    if (row == -1)
        row = rowCount();
    row = qMin(row, rowCount());
    const auto rend = list.crend();
    for (auto it = list.crbegin(); it != rend; ++it) {
        QUrl url = *it;
        if (!url.isValid() || url.scheme() != "file"_L1)
            continue;
        //this makes sure the url is clean
        const QString cleanUrl = QDir::cleanPath(url.toLocalFile());
        if (!cleanUrl.isEmpty())
            url = QUrl::fromLocalFile(cleanUrl);

        for (int j = 0; move && j < rowCount(); ++j) {
            QString local = index(j, 0).data(UrlRole).toUrl().toLocalFile();
#if defined(Q_OS_WIN)
            const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
            const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
            if (!cleanUrl.compare(local, cs)) {
                removeRow(j);
                if (j <= row)
                    row--;
                break;
            }
        }
        row = qMax(row, 0);
        QModelIndex idx = fileSystemModel->index(cleanUrl);
        if (!fileSystemModel->isDir(idx))
            continue;
        insertRows(row, 1);
        setUrl(index(row, 0), url, idx);
        watching.append({idx, cleanUrl});
    }
}

/*!
    Return the complete list of urls in a QList.
*/
QList<QUrl> QUrlModel::urls() const
{
    QList<QUrl> list;
    const int numRows = rowCount();
    list.reserve(numRows);
    for (int i = 0; i < numRows; ++i)
        list.append(data(index(i, 0), UrlRole).toUrl());
    return list;
}

/*!
    QFileSystemModel to get index's from, clears existing rows
*/
void QUrlModel::setFileSystemModel(QFileSystemModel *model)
{
    if (model == fileSystemModel)
        return;
    if (fileSystemModel != nullptr) {
        disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        disconnect(model, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));
        disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(layoutChanged()));
    }
    fileSystemModel = model;
    if (fileSystemModel != nullptr) {
        connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        connect(model, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(layoutChanged()));
    }
    clear();
    insertColumns(0, 1);
}

/*
    If one of the index's we are watching has changed update our internal data
*/
void QUrlModel::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex parent = topLeft.parent();
    for (int i = 0; i < watching.size(); ++i) {
        QModelIndex index = watching.at(i).index;
        if (index.model() && topLeft.model()) {
            Q_ASSERT(index.model() == topLeft.model());
        }
        if (   index.row() >= topLeft.row()
            && index.row() <= bottomRight.row()
            && index.column() >= topLeft.column()
            && index.column() <= bottomRight.column()
            && index.parent() == parent) {
                changed(watching.at(i).path);
        }
    }
}

/*!
    Re-get all of our data, anything could have changed!
 */
void QUrlModel::layoutChanged()
{
    QStringList paths;
    paths.reserve(watching.size());
    for (const WatchItem &item : std::as_const(watching))
        paths.append(item.path);
    watching.clear();
    for (const auto &path : paths) {
        QModelIndex newIndex = fileSystemModel->index(path);
        watching.append({newIndex, path});
        if (newIndex.isValid())
            changed(path);
     }
}

/*!
    The following path changed data update our copy of that data

    \sa layoutChanged(), dataChanged()
*/
void QUrlModel::changed(const QString &path)
{
    for (int i = 0; i < rowCount(); ++i) {
        QModelIndex idx = index(i, 0);
        if (idx.data(UrlRole).toUrl().toLocalFile() == path) {
            setData(idx, idx.data(UrlRole).toUrl());
        }
    }
}

QSidebar::QSidebar(QWidget *parent) : QListView(parent)
{
}

void QSidebar::setModelAndUrls(QFileSystemModel *model, const QList<QUrl> &newUrls)
{
    setUniformItemSizes(true);
    urlModel = new QUrlModel(this);
    urlModel->setFileSystemModel(model);
    setModel(urlModel);
    setItemDelegate(new QSideBarDelegate(this));

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(clicked(QModelIndex)));
#if QT_CONFIG(draganddrop)
    setDragDropMode(QAbstractItemView::DragDrop);
#endif
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenu(QPoint)));
    urlModel->setUrls(newUrls);
    setCurrentIndex(this->model()->index(0,0));
}

QSidebar::~QSidebar()
{
}

#if QT_CONFIG(draganddrop)
void QSidebar::dragEnterEvent(QDragEnterEvent *event)
{
    if (urlModel->canDrop(event))
        QListView::dragEnterEvent(event);
}
#endif // QT_CONFIG(draganddrop)

QSize QSidebar::sizeHint() const
{
    if (model())
        return QListView::sizeHintForIndex(model()->index(0, 0)) + QSize(2 * frameWidth(), 2 * frameWidth());
    return QListView::sizeHint();
}

void QSidebar::selectUrl(const QUrl &url)
{
    disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
               this, SLOT(clicked(QModelIndex)));

    selectionModel()->clear();
    for (int i = 0; i < model()->rowCount(); ++i) {
        if (model()->index(i, 0).data(QUrlModel::UrlRole).toUrl() == url) {
            selectionModel()->select(model()->index(i, 0), QItemSelectionModel::Select);
            break;
        }
    }

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(clicked(QModelIndex)));
}

#if QT_CONFIG(menu)
/*!
    \internal

    \sa removeEntry()
*/
void QSidebar::showContextMenu(const QPoint &position)
{
    QList<QAction *> actions;
    if (indexAt(position).isValid()) {
        QAction *action = new QAction(QFileDialog::tr("Remove"), this);
        if (indexAt(position).data(QUrlModel::UrlRole).toUrl().path().isEmpty())
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(removeEntry()));
        actions.append(action);
    }
    if (actions.size() > 0)
        QMenu::exec(actions, mapToGlobal(position));
}
#endif // QT_CONFIG(menu)

/*!
    \internal

    \sa showContextMenu()
*/
void QSidebar::removeEntry()
{
    const QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    // Create a list of QPersistentModelIndex as the removeRow() calls below could
    // invalidate the indexes in "idxs"
    const QList<QPersistentModelIndex> persIndexes(idxs.cbegin(), idxs.cend());
    for (const QPersistentModelIndex &persistent : persIndexes) {
        if (!persistent.data(QUrlModel::UrlRole).toUrl().path().isEmpty())
            model()->removeRow(persistent.row());
    }
}

/*!
    \internal

    \sa goToUrl()
*/
void QSidebar::clicked(const QModelIndex &index)
{
    QUrl url = model()->index(index.row(), 0).data(QUrlModel::UrlRole).toUrl();
    emit goToUrl(url);
    selectUrl(url);
}

/*!
    \reimp
    Don't automatically select something
 */
void QSidebar::focusInEvent(QFocusEvent *event)
{
    QAbstractScrollArea::focusInEvent(event);
    viewport()->update();
}

/*!
    \reimp
 */
bool QSidebar::event(QEvent * event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Delete) {
            removeEntry();
            return true;
        }
    }
    return QListView::event(event);
}

QT_END_NAMESPACE

#include "moc_qsidebar_p.cpp"
