/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "macmainwindow.h"
#import <AppKit/AppKit.h>
#include <QtGui>


#ifdef Q_DEAD_CODE_FROM_QT4_MAC

#include <Carbon/Carbon.h>

//![0]
SearchWidget::SearchWidget(QWidget *parent)
    : QMacCocoaViewContainer(0, parent)
{
    // Many Cocoa objects create temporary autorelease objects,
    // so create a pool to catch them.
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    // Create the NSSearchField, set it on the QCocoaViewContainer.
    NSSearchField *search = [[NSSearchField alloc] init];
    setCocoaView(search);

    // Use a Qt menu for the search field menu.
    QMenu *qtMenu = createMenu(this);
    NSMenu *nsMenu = qtMenu->macMenu(0);
    [[search cell] setSearchMenuTemplate:nsMenu];

    // Release our reference, since our super class takes ownership and we
    // don't need it anymore.
    [search release];

    // Clean up our pool as we no longer need it.
    [pool release];
}
//![0]

SearchWidget::~SearchWidget()
{
}

QSize SearchWidget::sizeHint() const
{
    return QSize(150, 40);
}

QMenu *createMenu(QWidget *parent)
{
    QMenu *searchMenu = new QMenu(parent);

    QAction * indexAction = searchMenu->addAction("Index Search");
    indexAction->setCheckable(true);
    indexAction->setChecked(true);

    QAction * fulltextAction = searchMenu->addAction("Full Text Search");
    fulltextAction->setCheckable(true);

    QActionGroup *searchActionGroup = new QActionGroup(parent);
    searchActionGroup->addAction(indexAction);
    searchActionGroup->addAction(fulltextAction);
    searchActionGroup->setExclusive(true);

    return searchMenu;
}

SearchWrapper::SearchWrapper(QWidget *parent)
:QWidget(parent)
{
    s = new SearchWidget(this);
    s->move(2,2);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

QSize SearchWrapper::sizeHint() const
{
    return s->sizeHint() + QSize(6, 2);
}

Spacer::Spacer(QWidget *parent)
:QWidget(parent)
{
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setSizePolicy(sizePolicy);
}

QSize Spacer::sizeHint() const
{
    return QSize(1, 1);
}

MacSplitterHandle::MacSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
: QSplitterHandle(orientation, parent) {   }

// Paint the horizontal handle as a gradient, paint
// the vertical handle as a line.
void MacSplitterHandle::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor topColor(145, 145, 145);
    QColor bottomColor(142, 142, 142);
    QColor gradientStart(252, 252, 252);
    QColor gradientStop(223, 223, 223);

    if (orientation() == Qt::Vertical) {
        painter.setPen(topColor);
        painter.drawLine(0, 0, width(), 0);
        painter.setPen(bottomColor);
        painter.drawLine(0, height() - 1, width(), height() - 1);

        QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height() -3));
        linearGrad.setColorAt(0, gradientStart);
        linearGrad.setColorAt(1, gradientStop);
        painter.fillRect(QRect(QPoint(0,1), size() - QSize(0, 2)), QBrush(linearGrad));
    } else {
        painter.setPen(topColor);
        painter.drawLine(0, 0, 0, height());
    }
}

QSize MacSplitterHandle::sizeHint() const
{
    QSize parent = QSplitterHandle::sizeHint();
    if (orientation() == Qt::Vertical) {
        return parent + QSize(0, 3);
    } else {
        return QSize(1, parent.height());
    }
}

QSplitterHandle *MacSplitter::createHandle()
{
    return new MacSplitterHandle(orientation(), this);
}

MacMainWindow::MacMainWindow()
{
    QSettings settings;
    restoreGeometry(settings.value("Geometry").toByteArray());

    setWindowTitle("Mac Main Window");

    splitter = new MacSplitter();

    // Set up the left-hand side blue side bar.
    sidebar = new QTreeView();
    sidebar->setFrameStyle(QFrame::NoFrame);
    sidebar->setAttribute(Qt::WA_MacShowFocusRect, false);
    sidebar->setAutoFillBackground(true);

    // Set the palette.
    QPalette palette = sidebar->palette();
    QColor macSidebarColor(231, 237, 246);
    QColor macSidebarHighlightColor(168, 183, 205);
    palette.setColor(QPalette::Base, macSidebarColor);
    palette.setColor(QPalette::Highlight, macSidebarHighlightColor);
    sidebar->setPalette(palette);

    sidebar->setModel(createItemModel());
    sidebar->header()->hide();
    sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebar->setTextElideMode(Qt::ElideMiddle);

    splitter->addWidget(sidebar);

    horizontalSplitter = new MacSplitter();
    horizontalSplitter->setOrientation(Qt::Vertical);
    splitter->addWidget(horizontalSplitter);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    // Set up the top document list view.
    documents = new QListView();
    documents->setFrameStyle(QFrame::NoFrame);
    documents->setAttribute(Qt::WA_MacShowFocusRect, false);
    documents->setModel(createDocumentModel());
    documents->setAlternatingRowColors(true);
    documents->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    horizontalSplitter->addWidget(documents);
    horizontalSplitter->setStretchFactor(0, 0);

    // Set up the text view.
    textedit = new QTextEdit();
    textedit->setFrameStyle(QFrame::NoFrame);
    textedit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    textedit->setText("<br><br><br><br><br><br><center><b>This demo shows how to create a \
                       Qt main window application that has the same appearance as other \
                       OS X applications such as Mail or iTunes. This includes \
                       customizing the item views and QSplitter and wrapping native widgets \
                       such as the search field.</b></center>");

    horizontalSplitter->addWidget(textedit);

    setCentralWidget(splitter);

    toolBar = addToolBar(tr("Search"));
    toolBar->addWidget(new Spacer());
    toolBar->addWidget(new SearchWrapper());

    setUnifiedTitleAndToolBarOnMac(true);
}

MacMainWindow::~MacMainWindow()
{
    QSettings settings;
    settings.setValue("Geometry", saveGeometry());
}

QAbstractItemModel *MacMainWindow::createItemModel()
{
    QStandardItemModel *model = new QStandardItemModel();
    QStandardItem *parentItem = model->invisibleRootItem();

    QStandardItem *documentationItem = new QStandardItem("Documentation");
    parentItem->appendRow(documentationItem);

    QStandardItem *assistantItem = new QStandardItem("Qt MainWindow Manual");
    documentationItem->appendRow(assistantItem);

    QStandardItem *designerItem = new QStandardItem("Qt Designer Manual");
    documentationItem->appendRow(designerItem);

    QStandardItem *qtItem = new QStandardItem("Qt Reference Documentation");
    qtItem->appendRow(new QStandardItem("Classes"));
    qtItem->appendRow(new QStandardItem("Overviews"));
    qtItem->appendRow(new QStandardItem("Tutorial & Examples"));
    documentationItem->appendRow(qtItem);

    QStandardItem *bookmarksItem = new QStandardItem("Bookmarks");
    parentItem->appendRow(bookmarksItem);
    bookmarksItem->appendRow(new QStandardItem("QWidget"));
    bookmarksItem->appendRow(new QStandardItem("QObject"));
    bookmarksItem->appendRow(new QStandardItem("QWizard"));

    return model;
}

void MacMainWindow::resizeEvent(QResizeEvent *)
{
    if (toolBar)
        toolBar->updateGeometry();
}

QAbstractItemModel *MacMainWindow::createDocumentModel()
{
    QStandardItemModel *model = new QStandardItemModel();
    QStandardItem *parentItem = model->invisibleRootItem();
    parentItem->appendRow(new QStandardItem("QWidget Class Reference"));
    parentItem->appendRow(new QStandardItem("QObject Class Reference"));
    parentItem->appendRow(new QStandardItem("QListView Class Reference"));

    return model;
}

#endif // Q_DEAD_CODE_FROM_QT4_MAC
