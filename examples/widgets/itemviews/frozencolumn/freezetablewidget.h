// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FREEZETABLEWIDGET_H
#define FREEZETABLEWIDGET_H

#include <QTableView>

//! [Widget definition]
class FreezeTableWidget : public QTableView {
     Q_OBJECT

public:
      FreezeTableWidget(QAbstractItemModel * model);
      ~FreezeTableWidget();


protected:
      void resizeEvent(QResizeEvent *event) override;
      QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
      void scrollTo (const QModelIndex & index, ScrollHint hint = EnsureVisible) override;

private:
      QTableView *frozenTableView;
      void init();
      void updateFrozenTableGeometry();


private slots:
      void updateSectionWidth(int logicalIndex, int oldSize, int newSize);
      void updateSectionHeight(int logicalIndex, int oldSize, int newSize);

};
//! [Widget definition]
#endif // FREEZETABLEWIDGET_H
