// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QPoint>

class QComboBox;
class QLabel;
class QLineEdit;
class QMouseEvent;
class QTextEdit;
class DragWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void setDragResult(const QString &actionText);
    void setMimeTypes(const QStringList &types);

private:
    QComboBox *mimeTypeCombo;
    DragWidget *dragWidget;
};

#endif
