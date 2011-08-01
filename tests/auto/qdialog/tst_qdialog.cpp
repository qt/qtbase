/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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

#include "../../shared/util.h"

Q_DECLARE_METATYPE(QSize)


QT_FORWARD_DECLARE_CLASS(QDialog)

//TESTED_CLASS=
//TESTED_FILES=

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
    void showAsTool();
    void toolDialogPosition();
    void deleteMainDefault();
    void deleteInExec();
    void throwInExec();
    void showSizeGrip();
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
    QTest::newRow( "data0" )  << QSize(100,100) << QSize(50,50) << (bool)FALSE << QSize(100,150);
    QTest::newRow( "data1" )  << QSize(100,100) << QSize(120,50) << (bool)FALSE << QSize(120,150);
    QTest::newRow( "data2" )  << QSize(100,100) << QSize(50,50) << (bool)TRUE << QSize(150,100);
    QTest::newRow( "data3" )  << QSize(100,100) << QSize(50,120) << (bool)TRUE << QSize(150,120);
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

#ifdef Q_WS_S60
    const int htDiff = ext->size().height() - testWidget->size().height();
#endif
    // show
    ((DummyDialog*)testWidget)->showExtension( TRUE );
//     while ( testWidget->size() == dlgSize )
// 	qApp->processEvents();

#ifdef Q_WS_S60
    QPoint expectedPosition;
    if (!horizontal) {
        expectedPosition = QPoint(0, oldPosition.y() - extSize.height());
    } else {
        if (htDiff>0)
            expectedPosition = QPoint(0, oldPosition.y() - htDiff);
        else
            expectedPosition = oldPosition;
    }
#endif

    QTEST( testWidget->size(), "result"  );

#ifdef Q_WS_S60
    QCOMPARE(testWidget->pos(), expectedPosition);
#else
    QCOMPARE(testWidget->pos(), oldPosition);
#endif

    // hide extension. back to old size ?
    ((DummyDialog*)testWidget)->showExtension( FALSE );
    QCOMPARE( testWidget->size(), dlgSize );

    testWidget->setExtension( 0 );
}

void tst_QDialog::defaultButtons()
{
    QLineEdit *lineEdit = new QLineEdit(testWidget);
    QPushButton *push = new QPushButton("Button 1", testWidget);
    QPushButton *pushTwo = new QPushButton("Button 2", testWidget);
    QPushButton *pushThree = new QPushButton("Button 3", testWidget);
    pushThree->setAutoDefault(FALSE);

    //we need to show the buttons. Otherwise they won't get the focus
    push->show();
    pushTwo->show();
    pushThree->show();

    push->setDefault(TRUE);
    QVERIFY(push->isDefault());

    pushTwo->setFocus();
    QVERIFY(pushTwo->isDefault());
    pushThree->setFocus();
    QVERIFY(push->isDefault());
    lineEdit->setFocus();
    QVERIFY(push->isDefault());

    pushTwo->setDefault(TRUE);
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
    QSizeGrip *sizeGrip = qFindChild<QSizeGrip *>(&dialog);
    QVERIFY(sizeGrip);

    dialog.showMaximized();
    QVERIFY(dialog.isMaximized());
    QVERIFY(dialog.isVisible());
#if !defined(Q_WS_MAC) && !defined(Q_OS_IRIX) && !defined(Q_OS_HPUX)
    QVERIFY(!sizeGrip->isVisible());
#endif

    dialog.showNormal();
    QVERIFY(!dialog.isMaximized());
    QVERIFY(dialog.isVisible());
    QVERIFY(sizeGrip->isVisible());

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
    QSizeGrip *sizeGrip = qFindChild<QSizeGrip *>(&dialog);
    QVERIFY(sizeGrip);

    qApp->syncX();
    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());
    QVERIFY(!sizeGrip->isVisible());

    qApp->syncX();
    dialog.showNormal();
    QVERIFY(!dialog.isFullScreen());
    QVERIFY(dialog.isVisible());
    QVERIFY(sizeGrip->isVisible());

    qApp->syncX();
    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());

    qApp->syncX();
    dialog.hide();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(!dialog.isVisible());

    qApp->syncX();
    dialog.show();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());

    qApp->syncX();
    dialog.hide();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(!dialog.isVisible());

    qApp->syncX();
    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());

    qApp->syncX();
    dialog.hide();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(!dialog.isVisible());
}

void tst_QDialog::showAsTool()
{
#if defined(Q_WS_X11)
    QSKIP("Qt/X11: Skipped since activeWindow() is not respected by all window managers", SkipAll);
#elif defined(Q_OS_WINCE)
    QSKIP("No real support for Qt::Tool on WinCE", SkipAll);
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

// Verify that pos() returns the same before and after show()
// for a dialog with the Tool window type.
void tst_QDialog::toolDialogPosition()
{
#if defined(Q_OS_WINCE)
    QSKIP("No real support for Qt::Tool on WinCE", SkipAll);
#endif
    QDialog dialog(0, Qt::Tool);
    dialog.move(QPoint(100,100));
    const QPoint beforeShowPosition = dialog.pos();
    dialog.show();
    const QPoint afterShowPosition = dialog.pos();
    QCOMPARE(afterShowPosition, beforeShowPosition);
}

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

#ifndef QT_NO_EXCEPTIONS
class QDialogTestException : public std::exception { };

class ExceptionDialog : public QDialog
{
    Q_OBJECT
public:
    ExceptionDialog() : QDialog(0) { }
public slots:
    void throwException()
    {
        QDialogTestException e;
        throw e;
    }
};

void tst_QDialog::throwInExec()
{
#if defined(Q_WS_MAC) || (defined(Q_WS_WINCE) && defined(_ARM_))
    QSKIP("Throwing exceptions in exec() is not supported on this platform.", SkipAll);
#endif

#if defined(Q_OS_LINUX)
    // C++ exceptions can't be passed through glib callbacks.  Skip the test if
    // we're using the glib event loop.
    QByteArray dispatcher = QAbstractEventDispatcher::instance()->metaObject()->className();
    if (dispatcher.contains("Glib")) {
        QSKIP(
            qPrintable(QString(
                "Throwing exceptions in exec() won't work if %1 event dispatcher is used.\n"
                "Try running with QT_NO_GLIB=1 in environment."
            ).arg(QString::fromLatin1(dispatcher))),
            SkipAll
        );
    }
#endif

    int caughtExceptions = 0;
    try {
        ExceptionDialog dialog;
        QMetaObject::invokeMethod(&dialog, "throwException", Qt::QueuedConnection);
        QMetaObject::invokeMethod(&dialog, "reject", Qt::QueuedConnection);
        (void) dialog.exec();
    } catch(...) {
        ++caughtExceptions;
    }
#ifdef Q_OS_SYMBIAN
    //on symbian, the event loop absorbs exceptions
    QCOMPARE(caughtExceptions, 0);
#else
    QCOMPARE(caughtExceptions, 1);
#endif
}
#else
void tst_QDialog::throwInExec()
{
    QSKIP("Exceptions are disabled", SkipAll);
}
#endif //QT_NO_EXCEPTIONS

// From Task 124269
void tst_QDialog::showSizeGrip()
{
#ifndef QT_NO_SIZEGRIP
    QDialog dialog(0);
    dialog.show();
    QWidget *ext = new QWidget(&dialog);
    QVERIFY(!dialog.extension());
    QVERIFY(!dialog.isSizeGripEnabled());

    dialog.setSizeGripEnabled(true);
    QPointer<QSizeGrip> sizeGrip = qFindChild<QSizeGrip *>(&dialog);
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
    sizeGrip = qFindChild<QSizeGrip *>(&dialog);
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
    sizeGrip = qFindChild<QSizeGrip *>(&dialog);
    QVERIFY(sizeGrip);
    QVERIFY(sizeGrip->isVisible());
    sizeGrip->hide();
    dialog.hide();
    dialog.show();
    QVERIFY(!sizeGrip->isVisible());
#endif
}

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
    QTest::qWaitForWindowShown(&dialog);
    QTRY_VERIFY(dialog.isVisible());
    dialog.reject();
    QTRY_VERIFY(!dialog.isVisible());
    QCOMPARE(dialog.called, 1);

    dialog.show();
    QTest::qWaitForWindowShown(&dialog);
    QTRY_VERIFY(dialog.isVisible());
    QVERIFY(dialog.close());
    QTRY_VERIFY(!dialog.isVisible());
    QCOMPARE(dialog.called, 2);

    dialog.cancelReject = true;
    dialog.show();
    QTest::qWaitForWindowShown(&dialog);
    QTRY_VERIFY(dialog.isVisible());
    dialog.reject();
    QTRY_VERIFY(dialog.isVisible());
    QCOMPARE(dialog.called, 3);
    QVERIFY(!dialog.close());
    QTRY_VERIFY(dialog.isVisible());
    QCOMPARE(dialog.called, 4);
}


QTEST_MAIN(tst_QDialog)
#include "tst_qdialog.moc"
