// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Notepad;
}
QT_END_NAMESPACE

class Notepad : public QMainWindow
{
    Q_OBJECT

public:
    explicit Notepad(QWidget *parent = nullptr);
    ~Notepad();

private slots:
    void newDocument();
    void open();
    void save();
    void saveAs();
    void print();
    void selectFont();
    void setFontBold(bool bold);
    void setFontUnderline(bool underline);
    void setFontItalic(bool italic);
    void about();

private:
    Ui::Notepad *ui;
    QString currentFile;
};

#endif // NOTEPAD_H
