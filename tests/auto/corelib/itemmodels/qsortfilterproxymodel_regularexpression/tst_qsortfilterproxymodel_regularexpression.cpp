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

#include <QTest>
#include <QSignalSpy>
#include <QStringListModel>
#include <QSortFilterProxyModel>

class tst_QSortFilterProxyModelRegularExpression : public QObject
{
    Q_OBJECT
private slots:
    void tst_invalid();
    void tst_caseSensitivity();
    void tst_keepCaseSensitivity_QTBUG_92260();
    void tst_keepPatternOptions_QTBUG_92260();
    void tst_regexCaseSensitivityNotification();
};

void tst_QSortFilterProxyModelRegularExpression::tst_invalid()
{
    const QLatin1String pattern("test");
    QSortFilterProxyModel model;
    model.setFilterRegularExpression(pattern);
    QCOMPARE(model.filterRegularExpression(), QRegularExpression(pattern));
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

/*!
    This test ensures that when a string pattern is passed to setRegularEpxression,
    the options are properly reseted but that the case sensitivity is kept as is.

 */
void tst_QSortFilterProxyModelRegularExpression::tst_keepCaseSensitivity_QTBUG_92260()
{
    const QLatin1String pattern("test");
    QStringListModel model({ "test", "TesT" });
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);

    QRegularExpression patternWithOptions("Dummy",
                                          QRegularExpression::MultilineOption
                                                  | QRegularExpression::CaseInsensitiveOption);

    proxyModel.setFilterRegularExpression(patternWithOptions);
    QCOMPARE(proxyModel.filterRegularExpression(), patternWithOptions);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseInsensitive);

    proxyModel.setFilterRegularExpression(pattern);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(proxyModel.filterRegularExpression().patternOptions(),
             QRegularExpression::CaseInsensitiveOption);

    patternWithOptions.setPatternOptions(QRegularExpression::MultilineOption);
    proxyModel.setFilterRegularExpression(patternWithOptions);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(proxyModel.filterRegularExpression(), patternWithOptions);

    proxyModel.setFilterRegularExpression(pattern);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(proxyModel.filterRegularExpression().patternOptions(),
             QRegularExpression::NoPatternOption);
}

/*!
    This test ensures that when the case sensitivity is changed, it does not nuke
    the pattern options that were set before.
 */
void tst_QSortFilterProxyModelRegularExpression::tst_keepPatternOptions_QTBUG_92260()
{
    QStringListModel model({ "test", "TesT" });
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);

    QRegularExpression patternWithOptions("Dummy",
                                          QRegularExpression::MultilineOption
                                                  | QRegularExpression::CaseInsensitiveOption);

    proxyModel.setFilterRegularExpression(patternWithOptions);
    QCOMPARE(proxyModel.filterRegularExpression(), patternWithOptions);

    proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(proxyModel.filterRegularExpression().patternOptions(),
             patternWithOptions.patternOptions());

    proxyModel.setFilterCaseSensitivity(Qt::CaseSensitive);
    QCOMPARE(proxyModel.filterCaseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(proxyModel.filterRegularExpression().patternOptions(),
             QRegularExpression::MultilineOption);
}

/*!
    This test ensures that if the case sensitivity is changed during a call to
    setFilterRegularExpression, the notification signal will be emitted
*/
void tst_QSortFilterProxyModelRegularExpression::tst_regexCaseSensitivityNotification()
{
    QSortFilterProxyModel proxy;
    QSignalSpy spy(&proxy, &QSortFilterProxyModel::filterCaseSensitivityChanged);
    proxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(spy.count(), 1);
    QRegularExpression re("regex");
    QVERIFY(!re.patternOptions().testFlag(QRegularExpression::CaseInsensitiveOption));
    proxy.setFilterRegularExpression(re);
    QCOMPARE(proxy.filterCaseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(spy.count(), 2);
}

QTEST_MAIN(tst_QSortFilterProxyModelRegularExpression)
#include "tst_qsortfilterproxymodel_regularexpression.moc"
