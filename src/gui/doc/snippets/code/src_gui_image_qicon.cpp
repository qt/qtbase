// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QIcon>
#include <QPainter>
#include <QToolButton>

namespace src_gui_image_qicon {

struct MyWidget : public QWidget
{
    void drawIcon(QPainter *painter, QPoint pos);
    bool isChecked() { return true; }
    QIcon icon;
};

void wrapper0() {

//! [0]
QToolButton *button = new QToolButton;
button->setIcon(QIcon("open.xpm"));
//! [0]


//! [1]
button->setIcon(QIcon());
//! [1]

} // wrapper0


//! [2]
void MyWidget::drawIcon(QPainter *painter, QPoint pos)
{
    QPixmap pixmap = icon.pixmap(QSize(22, 22),
                                   isEnabled() ? QIcon::Normal
                                               : QIcon::Disabled,
                                   isChecked() ? QIcon::On
                                               : QIcon::Off);
    painter->drawPixmap(pos, pixmap);
}
//! [2]


void wrapper1() {

//! [3]
QIcon undoicon = QIcon::fromTheme("edit-undo");
//! [3]

} // wrapper1


//! [4]
QIcon undoicon = QIcon::fromTheme("edit-undo", QIcon(":/undo.png"));
//! [4]


void wrapper2(){
//! [5]
QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths() << "my/search/path");
//! [5]

} // wrapper2
} // src_gui_image_qicon
