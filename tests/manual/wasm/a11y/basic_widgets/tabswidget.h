// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TABDIALOG_H
#define TABDIALOG_H
#include <QTabWidget>
#include <QtWidgets>

class GeneralTab : public QWidget
{
  Q_OBJECT

public:

    explicit GeneralTab(QWidget *parent = nullptr);
};

class EditViewTab : public QWidget
{

  Q_OBJECT
private:
   bool b_connected = false;
   QPlainTextEdit* textEdit =nullptr;
   QToolBar* m_toolbar= nullptr;
public:
   void showEvent( QShowEvent* event ) ;
    QPlainTextEdit* getTextEdit(){return textEdit;}
    explicit EditViewTab( QWidget *parent = nullptr);
signals:
    void connectToToolBar();
};

#endif
