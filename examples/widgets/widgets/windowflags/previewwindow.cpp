// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "previewwindow.h"

#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

//! [0]
PreviewWindow::PreviewWindow(QWidget *parent)
    : QWidget(parent)
{
    textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    closeButton = new QPushButton(tr("&Close"));
    connect(closeButton, &QPushButton::clicked,
            this, &PreviewWindow::close);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textEdit);
    layout->addWidget(closeButton);
    setLayout(layout);

    setWindowTitle(tr("Preview"));
}
//! [0]

//! [1]
void PreviewWindow::setWindowFlags(Qt::WindowFlags flags)
{
    QWidget::setWindowFlags(flags);

    QString text;

    Qt::WindowFlags type = (flags & Qt::WindowType_Mask);
    if (type == Qt::Window)
        text = "Qt::Window";
    else if (type == Qt::Dialog)
        text = "Qt::Dialog";
    else if (type == Qt::Sheet)
        text = "Qt::Sheet";
    else if (type == Qt::Drawer)
        text = "Qt::Drawer";
    else if (type == Qt::Popup)
        text = "Qt::Popup";
    else if (type == Qt::Tool)
        text = "Qt::Tool";
    else if (type == Qt::ToolTip)
        text = "Qt::ToolTip";
    else if (type == Qt::SplashScreen)
        text = "Qt::SplashScreen";

    if (flags & Qt::MSWindowsFixedSizeDialogHint)
        text += "\n| Qt::MSWindowsFixedSizeDialogHint";
    if (flags & Qt::X11BypassWindowManagerHint)
        text += "\n| Qt::X11BypassWindowManagerHint";
    if (flags & Qt::FramelessWindowHint)
        text += "\n| Qt::FramelessWindowHint";
    if (flags & Qt::NoDropShadowWindowHint)
        text += "\n| Qt::NoDropShadowWindowHint";
    if (flags & Qt::WindowTitleHint)
        text += "\n| Qt::WindowTitleHint";
    if (flags & Qt::WindowSystemMenuHint)
        text += "\n| Qt::WindowSystemMenuHint";
    if (flags & Qt::WindowMinimizeButtonHint)
        text += "\n| Qt::WindowMinimizeButtonHint";
    if (flags & Qt::WindowMaximizeButtonHint)
        text += "\n| Qt::WindowMaximizeButtonHint";
    if (flags & Qt::WindowCloseButtonHint)
        text += "\n| Qt::WindowCloseButtonHint";
    if (flags & Qt::WindowContextHelpButtonHint)
        text += "\n| Qt::WindowContextHelpButtonHint";
    if (flags & Qt::WindowShadeButtonHint)
        text += "\n| Qt::WindowShadeButtonHint";
    if (flags & Qt::WindowStaysOnTopHint)
        text += "\n| Qt::WindowStaysOnTopHint";
    if (flags & Qt::WindowStaysOnBottomHint)
        text += "\n| Qt::WindowStaysOnBottomHint";
    if (flags & Qt::CustomizeWindowHint)
        text += "\n| Qt::CustomizeWindowHint";

    textEdit->setPlainText(text);
}
//! [1]
