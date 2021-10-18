/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <QPointer>
#include "QtCore/qapplicationstatic.h"

Q_APPLICATION_STATIC(QObject, tstObject)

class tst_qapplicationstatic : public QObject
{
    Q_OBJECT

private slots:
    void testCreateMultipleApplications() const;
};

void tst_qapplicationstatic::testCreateMultipleApplications() const
{
    for (int i = 0; i < 5; i++) {
        int argc = 1;
        char *argv[] = { (char *)"tst_qapplicationstatic" };
        auto app = new QCoreApplication(argc, argv);

        QVERIFY(tstObject);

        QPointer<QObject> tstObjectPointer(tstObject);
        QVERIFY(tstObjectPointer.get());

        QVERIFY2(tstObject->objectName().isEmpty(), "Got QObject from previous iteration, not correctly recreated");
        tstObject->setObjectName(QStringLiteral("tstObject"));
        QVERIFY(!tstObject->objectName().isEmpty());

        delete app;
        QVERIFY2(!tstObjectPointer.get(), "QObject wasn't destroyed on QCoreApplication destruction");
    }
}

QTEST_APPLESS_MAIN(tst_qapplicationstatic)
#include "tst_qapplicationstatic.moc"
