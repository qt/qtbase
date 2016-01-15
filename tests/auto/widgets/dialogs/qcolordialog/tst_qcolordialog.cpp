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
#include <QtGui/QtGui>
#include <QtWidgets/QColorDialog>

QT_FORWARD_DECLARE_CLASS(QtTestEventThread)

class tst_QColorDialog : public QObject
{
    Q_OBJECT
public:
    tst_QColorDialog();
    virtual ~tst_QColorDialog();

public slots:
    void postKeyReturn();
    void testGetRgba();
    void testNativeActiveModalWidget();

private slots:
    void defaultOkButton();
    void native_activeModalWidget();
    void task247349_alpha();
    void QTBUG_43548_initialColor();
};

class TestNativeDialog : public QColorDialog
{
    Q_OBJECT
public:
    QWidget *m_activeModalWidget;

    TestNativeDialog(QWidget *parent = 0)
        : QColorDialog(parent), m_activeModalWidget(0)
    {
        QTimer::singleShot(1, this, SLOT(test_activeModalWidgetSignal()));
    }

public slots:
    void test_activeModalWidgetSignal()
    {
        m_activeModalWidget = qApp->activeModalWidget();
    }
};

tst_QColorDialog::tst_QColorDialog()
{
}

tst_QColorDialog::~tst_QColorDialog()
{
}

void tst_QColorDialog::testNativeActiveModalWidget()
{
    // Check that QApplication::activeModalWidget retruns the
    // color dialog when it is executing, even when using a native
    // dialog:
    TestNativeDialog d;
    QTimer::singleShot(1000, &d, SLOT(hide()));
    d.exec();
    QCOMPARE(&d, d.m_activeModalWidget);
}

void tst_QColorDialog::native_activeModalWidget()
{
    QTimer::singleShot(3000, qApp, SLOT(quit()));
    QTimer::singleShot(0, this, SLOT(testNativeActiveModalWidget()));
    qApp->exec();
}

void tst_QColorDialog::postKeyReturn() {
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i=0; i<list.count(); ++i) {
        QColorDialog *dialog = qobject_cast<QColorDialog *>(list[i]);
        if (dialog) {
            QTest::keyClick( list[i], Qt::Key_Return, Qt::NoModifier );
            return;
        }
    }
}

void tst_QColorDialog::testGetRgba()
{
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "Sending QTest::keyClick to OSX color dialog helper fails, see QTBUG-24320", Continue);
#endif
    bool ok = false;
    QTimer::singleShot(500, this, SLOT(postKeyReturn()));
    QColorDialog::getRgba(0xffffffff, &ok);
    QVERIFY(ok);
}

void tst_QColorDialog::defaultOkButton()
{
    QTimer::singleShot(4000, qApp, SLOT(quit()));
    QTimer::singleShot(0, this, SLOT(testGetRgba()));
    qApp->exec();
}

void tst_QColorDialog::task247349_alpha()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    int alpha = 0x17;
    dialog.setCurrentColor(QColor(0x01, 0x02, 0x03, alpha));
    QCOMPARE(alpha, dialog.currentColor().alpha());
    QCOMPARE(alpha, qAlpha(dialog.currentColor().rgba()));
}

void tst_QColorDialog::QTBUG_43548_initialColor()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.setCurrentColor(QColor(Qt::red));
    QColor a(Qt::red);
    QCOMPARE(a, dialog.currentColor());
}

QTEST_MAIN(tst_QColorDialog)
#include "tst_qcolordialog.moc"
