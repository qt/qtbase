// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QTest>
#include <QErrorMessage>
#include <QDebug>
#include <QCheckBox>

#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>

class tst_QErrorMessage : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase_data();
    void init();

    void dontShowAgain();
    void dontShowCategoryAgain();
    void baseClassSetVisible();

};

void tst_QErrorMessage::initTestCase_data()
{
    QTest::addColumn<bool>("useNativeDialog");
    QTest::newRow("widget") << false;
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        if (theme->usePlatformNativeDialog(QPlatformTheme::MessageDialog))
            QTest::newRow("native") << true;
    }
}

void tst_QErrorMessage::init()
{
    QFETCH_GLOBAL(bool, useNativeDialog);
    qApp->setAttribute(Qt::AA_DontUseNativeDialogs, !useNativeDialog);
}

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
    QVERIFY(checkBox->isChecked());
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

void tst_QErrorMessage::baseClassSetVisible()
{
    QErrorMessage errorMessage;
    errorMessage.QDialog::setVisible(true);
    QCOMPARE(errorMessage.isVisible(), true);
    errorMessage.close();
}

QTEST_MAIN(tst_QErrorMessage)
#include "tst_qerrormessage.moc"
