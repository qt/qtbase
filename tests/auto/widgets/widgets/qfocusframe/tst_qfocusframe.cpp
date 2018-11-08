/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qfocusframe.h>
#include <qtableview.h>
#include <qstandarditemmodel.h>

class tst_QFocusFrame : public QObject
{
Q_OBJECT

public:
    tst_QFocusFrame();
    virtual ~tst_QFocusFrame();

private slots:
    void getSetCheck();
    void focusFrameInsideScrollview();
};

tst_QFocusFrame::tst_QFocusFrame()
{
}

tst_QFocusFrame::~tst_QFocusFrame()
{
}

// Testing get/set functions
void tst_QFocusFrame::getSetCheck()
{
    QFocusFrame *obj1 = new QFocusFrame();
    // QWidget * QFocusFrame::widget()
    // void QFocusFrame::setWidget(QWidget *)
    QWidget var1;
    QWidget *var2 = new QWidget(&var1);
    obj1->setWidget(var2);
    QCOMPARE(var2, obj1->widget());
    obj1->setWidget((QWidget *)0);
    QCOMPARE((QWidget *)0, obj1->widget());
    delete obj1;
}

void tst_QFocusFrame::focusFrameInsideScrollview()
{
    // Make sure that the focus frame follows the widget, even
    // if the widget is inside a QAbstractItemView. A QAbstractItemView will scroll
    // all the children, including the focus frame, when it scrolls, which
    // is why special considerations are taken inside the focus frame to
    // prevent the frame to scroll away from the widget it tracks.

    if (qApp->style()->objectName() != QLatin1String("macintosh"))
        QSKIP("This test is only valid when using a style that has a focus frame");

    QWidget window;
    window.setGeometry(100, 100, 500, 500);

    QTableView tableView(&window);
    tableView.resize(window.size());
    QStandardItemModel *itemModel = new QStandardItemModel();
    for (int i = 0; i < 50; ++i)
        itemModel->appendRow(new QStandardItem("Value"));
    tableView.setModel(itemModel);
    tableView.edit(itemModel->index(8, 0));

    window.show();
    QFocusFrame *focusFrame = nullptr;
    QTRY_VERIFY(focusFrame = window.findChild<QFocusFrame *>());
    const QPoint initialOffset = focusFrame->widget()->mapToGlobal(QPoint()) - focusFrame->mapToGlobal(QPoint());

    tableView.scrollTo(itemModel->index(40, 0));
    QPoint offsetAfterScroll = focusFrame->widget()->mapToGlobal(QPoint()) - focusFrame->mapToGlobal(QPoint());
    QCOMPARE(offsetAfterScroll, initialOffset);

    tableView.scrollTo(itemModel->index(0, 0));
    offsetAfterScroll = focusFrame->widget()->mapToGlobal(QPoint()) - focusFrame->mapToGlobal(QPoint());
    QCOMPARE(offsetAfterScroll, initialOffset);
}

QTEST_MAIN(tst_QFocusFrame)
#include "tst_qfocusframe.moc"
