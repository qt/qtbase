/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <qtest.h>
#include <QListView>
#include <QStandardItemModel>


class tst_QListView : public QObject
{
    Q_OBJECT

public:
    tst_QListView() = default;
    virtual ~tst_QListView() = default;

private slots:
    void benchSetCurrentIndex();
};

void tst_QListView::benchSetCurrentIndex()
{
    QStandardItemModel sm(50000, 1);
    QListView lv;
    lv.setModel(&sm);
    const int rc = lv.model()->rowCount();
    for (int i = 0; i < rc; i+= 100)
        lv.setRowHidden(i, true);
    lv.setCurrentIndex(lv.model()->index(0, 0, QModelIndex()));
    lv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    QBENCHMARK_ONCE {
        while (lv.currentIndex().row() < rc - 20)
            lv.setCurrentIndex(lv.model()->index(lv.currentIndex().row() + 17,
                                                 lv.currentIndex().column(),
                                                 QModelIndex()));
    }
}


QTEST_MAIN(tst_QListView)
#include "tst_qlistview.moc"
