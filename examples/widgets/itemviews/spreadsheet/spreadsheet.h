/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QLineEdit;
class QToolBar;
class QTableWidgetItem;
class QTableWidget;
QT_END_NAMESPACE

class SpreadSheet : public QMainWindow
{
    Q_OBJECT
public:
    SpreadSheet(int rows, int cols, QWidget *parent = nullptr);

public slots:
    void updateStatus(QTableWidgetItem *item);
    void updateColor(QTableWidgetItem *item);
    void updateLineEdit(QTableWidgetItem *item);
    void returnPressed();
    void selectColor();
    void selectFont();
    void clear();
    void showAbout();

    void print();

    void actionSum();
    void actionSubtract();
    void actionAdd();
    void actionMultiply();
    void actionDivide();

protected:
    void setupContextMenu();
    void setupContents();

    void setupMenuBar();
    void createActions();

    void actionMath_helper(const QString &title, const QString &op);
    bool runInputDialog(const QString &title,
                        const QString &c1Text,
                        const QString &c2Text,
                        const QString &opText,
                        const QString &outText,
                        QString *cell1, QString *cell2, QString *outCell);
private:
    QToolBar *toolBar;
    QAction *colorAction;
    QAction *fontAction;
    QAction *firstSeparator;
    QAction *cell_sumAction;
    QAction *cell_addAction;
    QAction *cell_subAction;
    QAction *cell_mulAction;
    QAction *cell_divAction;
    QAction *secondSeparator;
    QAction *clearAction;
    QAction *aboutSpreadSheet;
    QAction *exitAction;

    QAction *printAction;

    QLabel *cellLabel;
    QTableWidget *table;
    QLineEdit *formulaInput;

};

void decode_pos(const QString &pos, int *row, int *col);
QString encode_pos(int row, int col);


#endif // SPREADSHEET_H

