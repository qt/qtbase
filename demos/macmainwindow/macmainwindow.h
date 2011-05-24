/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef MACMAINWINDOW_H
#define MACMAINWINDOW_H

#include <QtGui>

#ifdef Q_WS_MAC

#import <qmaccocoaviewcontainer_mac.h>

#ifdef QT_MAC_USE_COCOA
class SearchWidget : public QMacCocoaViewContainer
{
    Q_OBJECT
public:
    SearchWidget(QWidget *parent = 0);
    ~SearchWidget();

    QSize sizeHint() const;
private:
};

#else
#include <Carbon/Carbon.h>

// The SearchWidget class wraps a native HISearchField.
class SearchWidget : public QWidget
{
    Q_OBJECT
private:
    HIViewRef searchField;
    CFStringRef searchFieldText;

public:
    QSize sizeHint() const;
    SearchWidget(QWidget *parent = 0);
    ~SearchWidget();
};

#endif

QMenu *createMenu(QWidget *parent);

class SearchWrapper : public QWidget
{
Q_OBJECT
public:
    SearchWrapper(QWidget *parent = 0);
    QSize sizeHint() const;
    QWidget *s;
};

class Spacer : public QWidget
{
Q_OBJECT
public:
    Spacer(QWidget *parent = 0);
    QSize sizeHint() const;
};

class MacSplitterHandle : public QSplitterHandle
{
Q_OBJECT
public:
    MacSplitterHandle(Qt::Orientation orientation, QSplitter *parent);
    void paintEvent(QPaintEvent *);
    QSize sizeHint() const;
};

class MacSplitter : public QSplitter
{
public:
    QSplitterHandle *createHandle();
};

class MacMainWindow : public QMainWindow
{
Q_OBJECT
public:
    MacMainWindow();
    ~MacMainWindow();
    QAbstractItemModel *createItemModel();
    void resizeEvent(QResizeEvent *e);
    QAbstractItemModel *createDocumentModel();
public:
    QSplitter *splitter;
    QSplitter *horizontalSplitter;
    QTreeView *sidebar;
    QListView *documents;
    QTextEdit *textedit;
    QVBoxLayout *layout;
    SearchWidget *searchWidget;
    QToolBar * toolBar;
};

#endif // Q_WS_MAC

#endif //MACMAINWINDOW_H
