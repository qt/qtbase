// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QIcon>
#include <QPainter>
#include <QToolButton>

namespace src_gui_image_qicon {

struct MyWidget : public QWidget
{
    void drawIcon(QPainter *painter, const QRect &rect);
    bool isChecked() { return true; }
    QIcon icon;
};

void wrapper0() {

//! [0]
QToolButton *button = new QToolButton;
button->setIcon(QIcon("open.png"));
//! [0]

//! [addFile]
QIcon openIcon("open.png");
openIcon.addFile("open-disabled.png", QIcon::Disabled);
//! [addFile]

//! [1]
button->setIcon(QIcon());
//! [1]

} // wrapper0


//! [2]
void MyWidget::drawIcon(QPainter *painter, const QRect &rect)
{
    icon.paint(painter, rect, Qt::AlignCenter, isEnabled() ? QIcon::Normal
                                                           : QIcon::Disabled,
                                               isChecked() ? QIcon::On
                                                           : QIcon::Off);
}
//! [2]


void wrapper1() {

//! [fromTheme]
QIcon undoicon = QIcon::fromTheme(QIcon::ThemeIcon::EditUndo);
//! [fromTheme]

//! [iconFont]
QIcon::setThemeName("Material Symbols Outlined");
QIcon muteIcon = QIcon::fromTheme(u"volume_off"_s);
//! [iconFont]

} // wrapper1


//! [4]
QIcon undoicon = QIcon::fromTheme(QIcon::ThemeIcon::EditUndo, QIcon(":/undo.png"));
//! [4]


void wrapper2(){
//! [5]
QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths() << "my/search/path");
//! [5]

} // wrapper2
} // src_gui_image_qicon
