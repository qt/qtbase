/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qdialog.h>
#include <qapplication.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstyle.h>
#include <QVBoxLayout>
#include <QSizeGrip>



QT_FORWARD_DECLARE_CLASS(QDialog)

class tst_QDialog : public QObject
{
    Q_OBJECT
public:
    tst_QDialog();

public slots:
    void initTestCase();
    void cleanupTestCase();
private slots:
    void getSetCheck();
    void showExtension_data();
    void showExtension();
    void defaultButtons();
    void showMaximized();
    void showMinimized();
    void showFullScreen();
#ifndef Q_OS_WINCE
    void showAsTool();
    void toolDialogPosition();
#endif
    void deleteMainDefault();
    void deleteInExec();
#ifndef QT_NO_SIZEGRIP
    void showSizeGrip();
#endif
    void setVisible();
    void reject();

private:
    QDialog *testWidget;
};

// Testing get/set functions
void tst_QDialog::getSetCheck()
{
    QDialog obj1;
    // QWidget* QDialog::extension()
    // void QDialog::setExtension(QWidget*)
    QWidget *var1 = new QWidget;
    obj1.setExtension(var1);
    QCOMPARE(var1, obj1.extension());
    obj1.setExtension((QWidget *)0);
    QCOMPARE((QWidget *)0, obj1.extension());
    // No delete var1, since setExtension takes ownership

    // int QDialog::result()
    // void QDialog::setResult(int)
    obj1.setResult(0);
    QCOMPARE(0, obj1.result());
    obj1.setResult(INT_MIN);
    QCOMPARE(INT_MIN, obj1.result());
    obj1.setResult(INT_MAX);
    QCOMPARE(INT_MAX, obj1.result());
}

// work around function being protected
class DummyDialog : public QDialog {
public:
    DummyDialog(): QDialog(0) {}
    void showExtension( bool b ) { QDialog::showExtension( b ); }
};

class ToolDialog : public QDialog
{
public:
    ToolDialog(QWidget *parent = 0) : QDialog(parent, Qt::Tool), mWasActive(false), tId(-1) {
    }
    bool wasActive() const { return mWasActive; }

    int exec() {
        tId = startTimer(300);
        return QDialog::exec();
    }
protected:
    void timerEvent(QTimerEvent *event) {
        if (tId == event->timerId()) {
            killTimer(tId);
            mWasActive = isActiveWindow();
            reject();
        }
    }

private:
    int mWasActive;
    int tId;
};

tst_QDialog::tst_QDialog()

{
}

void tst_QDialog::initTestCase()
{
    // Create the test class
    testWidget = new QDialog(0, Qt::X11BypassWindowManagerHint);
    testWidget->resize(200,200);
    testWidget->show();
    qApp->setActiveWindow(testWidget);
}

void tst_QDialog::cleanupTestCase()
{
    if (testWidget) {
        delete testWidget;
        testWidget = 0;
    }
}

void tst_QDialog::showExtension_data()
{
    QTest::addColumn<QSize>("dlgSize");
    QTest::addColumn<QSize>("extSize");
    QTest::addColumn<bool>("horizontal");
    QTest::addColumn<QSize>("result");

    //next we fill it with data
    QTest::newRow( "data0" )  << QSize(200,100) << QSize(50,50) << false << QSize(200,150);
    QTest::newRow( "data1" )  << QSize(200,100) << QSize(220,50) << false << QSize(220,150);
    QTest::newRow( "data2" )  << QSize(200,100) << QSize(50,50) << true << QSize(250,100);
    QTest::newRow( "data3" )  << QSize(200,100) << QSize(50,120) << true << QSize(250,120);
}

void tst_QDialog::showExtension()
{
    QFETCH( QSize, dlgSize );
    QFETCH( QSize, extSize );
    QFETCH( bool, horizontal );

    // set geometry of main dialog and extension widget
    testWidget->setFixedSize( dlgSize );
    QWidget *ext = new QWidget( testWidget );
    ext->setFixedSize( extSize );
    testWidget->setExtension( ext );
    testWidget->setOrientation( horizontal ? Qt::Horizontal : Qt::Vertical );

    QCOMPARE( testWidget->size(), dlgSize );
    QPoint oldPosition = testWidget->pos();

    // show
    ((DummyDialog*)testWidget)->showExtension( true );
//     while ( testWidget->size() == dlgSize )
// 	qApp->processEvents();

    QTEST( testWidget->size(), "result"  );

    QCOMPARE(testWidget->pos(), oldPosition);

    // hide extension. back to old size ?
    ((DummyDialog*)testWidget)->showExtension( false );
    QCOMPARE( testWidget->size(), dlgSize );

    testWidget->setExtension( 0 );
}

void tst_QDialog::defaultButtons()
{
    QLineEdit *lineEdit = new QLineEdit(testWidget);
    QPushButton *push = new QPushButton("Button 1", testWidget);
    QPushButton *pushTwo = new QPushButton("Button 2", testWidget);
    QPushButton *pushThree = new QPushButton("Button 3", testWidget);
    pushThree->setAutoDefault(false);

    //we need to show the buttons. Otherwise they won't get the focus
    push->show();
    pushTwo->show();
    pushThree->show();

    push->setDefault(true);
    QVERIFY(push->isDefault());

    pushTwo->setFocus();
    QVERIFY(pushTwo->isDefault());
    pushThree->setFocus();
    QVERIFY(push->isDefault());
    lineEdit->setFocus();
    QVERIFY(push->isDefault());

    pushTwo->setDefault(true);
    QVERIFY(pushTwo->isDefault());

    pushTwo->setFocus();
    QVERIFY(pushTwo->isDefault());
    lineEdit->setFocus();
    QVERIFY(pushTwo->isDefault());
}

void tst_QDialog::showMaximized()
{
    QDialog dialog(0);
    dialog.setSizeGripEnabled(true);
#ifndef QT_NO_SIZEGRIP
    QSizeGrip *sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
#endif

    dialog.showMaximized();
    QVERIFY(dialog.isMaximized());
    QVERIFY(dialog.isVisible());
#if !defined(QT_NO_SIZEGRIP) && !defined(Q_OS_MAC) && !defined(Q_OS_IRIX) && !defined(Q_OS_HPUX)
    QVERIFY(!sizeGrip->isVisible());
#endif

    dialog.showNormal();
    QVERIFY(!dialog.isMaximized());
    QVERIFY(dialog.isVisible());
#ifndef QT_NO_SIZEGRIP
    QVERIFY(sizeGrip->isVisible());
#endif

    dialog.showMaximized();
    QVERIFY(dialog.isMaximized());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isMaximized());
    QVERIFY(!dialog.isVisible());

    dialog.show();
    QVERIFY(dialog.isMaximized());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isMaximized());
    QVERIFY(!dialog.isVisible());

    dialog.showMaximized();
    QVERIFY(dialog.isMaximized());
    QVERIFY(dialog.isVisible());
}

void tst_QDialog::showMinimized()
{
    QDialog dialog(0);

    dialog.showMinimized();
    QVERIFY(dialog.isMinimized());
    QVERIFY(dialog.isVisible());

    dialog.showNormal();
    QVERIFY(!dialog.isMinimized());
    QVERIFY(dialog.isVisible());

    dialog.showMinimized();
    QVERIFY(dialog.isMinimized());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isMinimized());
    QVERIFY(!dialog.isVisible());

    dialog.show();
    QVERIFY(dialog.isMinimized());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isMinimized());
    QVERIFY(!dialog.isVisible());

    dialog.showMinimized();
    QVERIFY(dialog.isMinimized());
    QVERIFY(dialog.isVisible());
}

void tst_QDialog::showFullScreen()
{
    QDialog dialog(0, Qt::X11BypassWindowManagerHint);
    dialog.setSizeGripEnabled(true);
#ifndef QT_NO_SIZEGRIP
    QSizeGrip *sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
#endif

    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());
#ifndef QT_NO_SIZEGRIP
    QVERIFY(!sizeGrip->isVisible());
#endif

    dialog.showNormal();
    QVERIFY(!dialog.isFullScreen());
    QVERIFY(dialog.isVisible());
#ifndef QT_NO_SIZEGRIP
    QVERIFY(sizeGrip->isVisible());
#endif

    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(!dialog.isVisible());

    dialog.show();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(!dialog.isVisible());

    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(!dialog.isVisible());
}

// No real support for Qt::Tool on WinCE
#ifndef Q_OS_WINCE
void tst_QDialog::showAsTool()
{
#if defined(Q_OS_UNIX)
    QSKIP("Qt/X11: Skipped since activeWindow() is not respected by all window managers");
#endif
    ToolDialog dialog(testWidget);
    testWidget->activateWindow();
    dialog.exec();
    QTest::qWait(100);
    if (testWidget->style()->styleHint(QStyle::SH_Widget_ShareActivation, 0, testWidget)) {
        QCOMPARE(dialog.wasActive(), true);
    } else {
        QCOMPARE(dialog.wasActive(), false);
    }
}
#endif

// No real support for Qt::Tool on WinCE
#ifndef Q_OS_WINCE
// Verify that pos() returns the same before and after show()
// for a dialog with the Tool window type.
void tst_QDialog::toolDialogPosition()
{
    QDialog dialog(0, Qt::Tool);
    dialog.move(QPoint(100,100));
    const QPoint beforeShowPosition = dialog.pos();
    dialog.show();
    const QPoint afterShowPosition = dialog.pos();
    QCOMPARE(afterShowPosition, beforeShowPosition);
}
#endif

class Dialog : public QDialog
{
public:
    Dialog(QPushButton *&button)
    {
        button = new QPushButton(this);
    }
};

void tst_QDialog::deleteMainDefault()
{
    QPushButton *button;
    Dialog dialog(button);
    button->setDefault(true);
    delete button;
    dialog.show();
    QTestEventLoop::instance().enterLoop(2);
}

void tst_QDialog::deleteInExec()
{
    QDialog *dialog = new QDialog(0);
    QMetaObject::invokeMethod(dialog, "deleteLater", Qt::QueuedConnection);
    QCOMPARE(dialog->exec(), int(QDialog::Rejected));
}

#ifndef QT_NO_SIZEGRIP
// From Task 124269
void tst_QDialog::showSizeGrip()
{
    QDialog dialog(0);
    dialog.show();
    QWidget *ext = new QWidget(&dialog);
    QVERIFY(!dialog.extension());
    QVERIFY(!dialog.isSizeGripEnabled());

    dialog.setSizeGripEnabled(true);
    QPointer<QSizeGrip> sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
    QVERIFY(sizeGrip->isVisible());
    QVERIFY(dialog.isSizeGripEnabled());

    dialog.setExtension(ext);
    QVERIFY(dialog.extension() && !dialog.extension()->isVisible());
    QVERIFY(dialog.isSizeGripEnabled());

    // normal show/hide sequence
    dialog.showExtension(true);
    QVERIFY(dialog.extension() && dialog.extension()->isVisible());
    QVERIFY(!dialog.isSizeGripEnabled());
    QVERIFY(!sizeGrip);

    dialog.showExtension(false);
    QVERIFY(dialog.extension() && !dialog.extension()->isVisible());
    QVERIFY(dialog.isSizeGripEnabled());
    sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
    QVERIFY(sizeGrip->isVisible());

    // show/hide sequence with interleaved size grip update
    dialog.showExtension(true);
    QVERIFY(dialog.extension() && dialog.extension()->isVisible());
    QVERIFY(!dialog.isSizeGripEnabled());
    QVERIFY(!sizeGrip);

    dialog.setSizeGripEnabled(false);
    QVERIFY(!dialog.isSizeGripEnabled());

    dialog.showExtension(false);
    QVERIFY(dialog.extension() && !dialog.extension()->isVisible());
    QVERIFY(!dialog.isSizeGripEnabled());

    dialog.setSizeGripEnabled(true);
    sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
    QVERIFY(sizeGrip->isVisible());
    sizeGrip->hide();
    dialog.hide();
    dialog.show();
    QVERIFY(!sizeGrip->isVisible());
}
#endif

void tst_QDialog::setVisible()
{
    QWidget topLevel;
    topLevel.show();

    QDialog *dialog = new QDialog;
    dialog->setLayout(new QVBoxLayout);
    dialog->layout()->addWidget(new QPushButton("dialog button"));

    QWidget *widget = new QWidget(&topLevel);
    widget->setLayout(new QVBoxLayout);
    widget->layout()->addWidget(dialog);

    QVERIFY(!dialog->isVisible());
    QVERIFY(!dialog->isHidden());

    widget->show();
    QVERIFY(dialog->isVisible());
    QVERIFY(!dialog->isHidden());

    widget->hide();
    dialog->hide();
    widget->show();
    QVERIFY(!dialog->isVisible());
    QVERIFY(dialog->isHidden());
}

class TestRejectDialog : public QDialog
{
    public:
        TestRejectDialog() : cancelReject(false), called(0) {}
        void reject()
        {
            called++;
            if (!cancelReject)
                QDialog::reject();
        }
        bool cancelReject;
        int called;
};

void tst_QDialog::reject()
{
    TestRejectDialog dialog;
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QVERIFY(dialog.isVisible());
    dialog.reject();
    QTRY_VERIFY(!dialog.isVisible());
    QCOMPARE(dialog.called, 1);

    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QVERIFY(dialog.isVisible());
    QVERIFY(dialog.close());
    QTRY_VERIFY(!dialog.isVisible());
    QCOMPARE(dialog.called, 2);

    dialog.cancelReject = true;
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QVERIFY(dialog.isVisible());
    dialog.reject();
    QTRY_VERIFY(dialog.isVisible());
    QCOMPARE(dialog.called, 3);
    QVERIFY(!dialog.close());
    QTRY_VERIFY(dialog.isVisible());
    QCOMPARE(dialog.called, 4);
}


QTEST_MAIN(tst_QDialog)
#include "tst_qdialog.moc"
