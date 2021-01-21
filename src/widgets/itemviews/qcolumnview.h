/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCOLUMNVIEW_H
#define QCOLUMNVIEW_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractitemview.h>

QT_REQUIRE_CONFIG(columnview);

QT_BEGIN_NAMESPACE

class QColumnViewPrivate;

class Q_WIDGETS_EXPORT QColumnView : public QAbstractItemView {

Q_OBJECT
    Q_PROPERTY(bool resizeGripsVisible READ resizeGripsVisible WRITE setResizeGripsVisible)

Q_SIGNALS:
    void updatePreviewWidget(const QModelIndex &index);

public:
    explicit QColumnView(QWidget *parent = nullptr);
    ~QColumnView();

    // QAbstractItemView overloads
    QModelIndex indexAt(const QPoint &point) const override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QSize sizeHint() const override;
    QRect visualRect(const QModelIndex &index) const override;
    void setModel(QAbstractItemModel *model) override;
    void setSelectionModel(QItemSelectionModel * selectionModel) override;
    void setRootIndex(const QModelIndex &index) override;
    void selectAll() override;

    // QColumnView functions
    void setResizeGripsVisible(bool visible);
    bool resizeGripsVisible() const;

    QWidget *previewWidget() const;
    void setPreviewWidget(QWidget *widget);

    void setColumnWidths(const QList<int> &list);
    QList<int> columnWidths() const;

protected:
    QColumnView(QColumnViewPrivate &dd, QWidget *parent = nullptr);

    // QAbstractItemView overloads
    bool isIndexHidden(const QModelIndex &index) const override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void resizeEvent(QResizeEvent *event) override;
    void setSelection(const QRect & rect, QItemSelectionModel::SelectionFlags command) override;
    QRegion visualRegionForSelection(const QItemSelection &selection) const override;
    int horizontalOffset() const override;
    int verticalOffset() const override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    // QColumnView functions
    void scrollContentsBy(int dx, int dy) override;
    virtual QAbstractItemView* createColumn(const QModelIndex &rootIndex);
    void initializeColumn(QAbstractItemView *column) const;

private:
    Q_DECLARE_PRIVATE(QColumnView)
    Q_DISABLE_COPY(QColumnView)
    Q_PRIVATE_SLOT(d_func(), void _q_gripMoved(int))
    Q_PRIVATE_SLOT(d_func(), void _q_changeCurrentColumn())
    Q_PRIVATE_SLOT(d_func(), void _q_clicked(const QModelIndex &))
};

QT_END_NAMESPACE

#endif // QCOLUMNVIEW_H

