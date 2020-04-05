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

#include "tst_qsortfilterproxymodel.h"

class tst_QSortFilterProxyModelRegularExpression : public tst_QSortFilterProxyModel
{
    Q_OBJECT
public:
    tst_QSortFilterProxyModelRegularExpression();
private slots:
    void tst_invalid();
    void tst_caseSensitivity();
};

tst_QSortFilterProxyModelRegularExpression::tst_QSortFilterProxyModelRegularExpression() :
    tst_QSortFilterProxyModel()
{
    m_filterType = FilterType::RegularExpression;
}

void tst_QSortFilterProxyModelRegularExpression::tst_invalid()
{
    const QLatin1String pattern("test");
    QSortFilterProxyModel model;
    model.setFilterRegularExpression(pattern);
    QCOMPARE(model.filterRegularExpression(), QRegularExpression(pattern));
    model.setFilterRegExp(pattern);
    QCOMPARE(model.filterRegularExpression(), QRegularExpression());
}

void tst_QSortFilterProxyModelRegularExpression::tst_caseSensitivity()
{
    const QLatin1String pattern("test");
    QStringListModel model({ "test", "TesT" });
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);

    proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel.setFilterRegularExpression(pattern);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(proxyModel.rowCount(), 2);

    proxyModel.setFilterCaseSensitivity(Qt::CaseSensitive);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(proxyModel.rowCount(), 1);
    proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(proxyModel.rowCount(), 2);
}

QTEST_MAIN(tst_QSortFilterProxyModelRegularExpression)
#include "tst_qsortfilterproxymodel_regularexpression.moc"
