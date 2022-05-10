// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VIEW_H
#define VIEW_H

#include <QtWidgets>
#include <QtSql>

class ImageItem;
class InformationWindow;

//! [0]
class View : public QGraphicsView
{
    Q_OBJECT

public:
    View(const QString &items, const QString &images, QWidget *parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
//! [0]

//! [1]
private slots:
    void updateImage(int id, const QString &fileName);
//! [1]

//! [2]
private:
    void addItems();
    InformationWindow *findWindow(int id) const;
    void showInformation(ImageItem *image);

    QGraphicsScene *scene;
    QList<InformationWindow *> informationWindows;
//! [2] //! [3]
    QSqlRelationalTableModel *itemTable;
};
//! [3]

#endif
