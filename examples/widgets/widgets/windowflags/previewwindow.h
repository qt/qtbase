// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTextEdit;
QT_END_NAMESPACE

//! [0]
class PreviewWindow : public QWidget
{
    Q_OBJECT

public:
    PreviewWindow(QWidget *parent = nullptr);

    void setWindowFlags(Qt::WindowFlags flags);

private:
    QTextEdit *textEdit;
    QPushButton *closeButton;
};
//! [0]

#endif
