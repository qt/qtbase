// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QTest>
#include <QErrorMessage>
#include <QDebug>
#include <QCheckBox>

class tst_QErrorMessage : public QObject
{
    Q_OBJECT

private slots:
    void dontShowAgain();
    void dontShowCategoryAgain();

};

void tst_QErrorMessage::dontShowAgain()
{
    QString plainString = QLatin1String("foo");
    QString htmlString = QLatin1String("foo<br>bar");
    QCheckBox *checkBox = nullptr;

    QErrorMessage errorMessageDialog(0);

    // show an error with plain string
    errorMessageDialog.showMessage(plainString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    errorMessageDialog.close();

    errorMessageDialog.showMessage(plainString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(false);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(plainString);
    QVERIFY(!errorMessageDialog.isVisible());

    // show an error with an html string
    errorMessageDialog.showMessage(htmlString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(!checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(false);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString);
    QVERIFY(!errorMessageDialog.isVisible());
}

void tst_QErrorMessage::dontShowCategoryAgain()
{
    QString htmlString = QLatin1String("foo<br>bar");
    QString htmlString2 = QLatin1String("foo2<br>bar2");
    QCheckBox *checkBox = nullptr;

    QErrorMessage errorMessageDialog(0);

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString2,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(false);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString2,"Cat 1");
    QVERIFY(!errorMessageDialog.isVisible());

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(!errorMessageDialog.isVisible());

    errorMessageDialog.showMessage(htmlString);
    QVERIFY(errorMessageDialog.isVisible());

    errorMessageDialog.showMessage(htmlString,"Cat 2");
    QVERIFY(errorMessageDialog.isVisible());
}

QTEST_MAIN(tst_QErrorMessage)
#include "tst_qerrormessage.moc"
