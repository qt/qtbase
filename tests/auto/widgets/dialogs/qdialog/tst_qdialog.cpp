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

#include "../../../shared/highdpi.h"

#include <QTest>
#include <QTestEventLoop>

#include <qdialog.h>
#include <qapplication.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstyle.h>
#include <QVBoxLayout>
#include <QSizeGrip>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QWindow>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformtheme_p.h>

QT_FORWARD_DECLARE_CLASS(QDialog)

// work around function being protected
class DummyDialog : public QDialog
{
public:
    DummyDialog(): QDialog() {}
};

class tst_QDialog : public QObject
{
    Q_OBJECT
public:
    tst_QDialog();

private slots:
    void cleanup();
    void getSetCheck();
    void defaultButtons();
    void showMaximized();
    void showMinimized();
    void showFullScreen();
    void showAsTool();
    void toolDialogPosition();
    void deleteMainDefault();
    void deleteInExec();
#if QT_CONFIG(sizegrip)
    void showSizeGrip();
#endif
    void setVisible();
    void reject();
    void snapToDefaultButton();
    void transientParent_data();
    void transientParent();
    void dialogInGraphicsView();
};

// Testing get/set functions
void tst_QDialog::getSetCheck()
{
    QDialog obj1;
    // int QDialog::result()
    // void QDialog::setResult(int)
    obj1.setResult(0);
    QCOMPARE(0, obj1.result());
    obj1.setResult(INT_MIN);
    QCOMPARE(INT_MIN, obj1.result());
    obj1.setResult(INT_MAX);
    QCOMPARE(INT_MAX, obj1.result());
}

class ToolDialog : public QDialog
{
public:
    ToolDialog(QWidget *parent = nullptr)
        : QDialog(parent, Qt::Tool), mWasActive(false), mWasModalWindow(false), tId(-1) {}

    bool wasActive() const { return mWasActive; }
    bool wasModalWindow() const { return mWasModalWindow; }

    int exec() override
    {
        tId = startTimer(300);
        return QDialog::exec();
    }
protected:
    void timerEvent(QTimerEvent *event) override
    {
        if (tId == event->timerId()) {
            killTimer(tId);
            mWasActive = isActiveWindow();
            mWasModalWindow = QGuiApplication::modalWindow() == windowHandle();
            reject();
        }
    }

private:
    int mWasActive;
    bool mWasModalWindow;
    int tId;
};

tst_QDialog::tst_QDialog()
{
}

void tst_QDialog::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}


void tst_QDialog::defaultButtons()
{
    DummyDialog testWidget;
    testWidget.resize(200, 200);
    testWidget.setWindowTitle(QTest::currentTestFunction());
    QLineEdit *lineEdit = new QLineEdit(&testWidget);
    QPushButton *push = new QPushButton("Button 1", &testWidget);
    QPushButton *pushTwo = new QPushButton("Button 2", &testWidget);
    QPushButton *pushThree = new QPushButton("Button 3", &testWidget);
    pushThree->setAutoDefault(false);

    testWidget.show();
    QApplication::setActiveWindow(&testWidget);
    QVERIFY(QTest::qWaitForWindowExposed(&testWidget));

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
#if QT_CONFIG(sizegrip)
    QSizeGrip *sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
#endif

    dialog.showMaximized();
    QVERIFY(dialog.isMaximized());
    QVERIFY(dialog.isVisible());
#if QT_CONFIG(sizegrip) && !defined(Q_OS_DARWIN) && !defined(Q_OS_HPUX)
    QVERIFY(!sizeGrip->isVisible());
#endif

    dialog.showNormal();
    QVERIFY(!dialog.isMaximized());
    QVERIFY(dialog.isVisible());
#if QT_CONFIG(sizegrip)
    QVERIFY(sizeGrip->isVisible());
#endif

    dialog.showMaximized();
    QVERIFY(dialog.isMaximized());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isMaximized());
    QVERIFY(!dialog.isVisible());

    dialog.setVisible(true);
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

    dialog.setVisible(true);
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
#if QT_CONFIG(sizegrip)
    QSizeGrip *sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
#endif

    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());
#if QT_CONFIG(sizegrip)
    QVERIFY(!sizeGrip->isVisible());
#endif

    dialog.showNormal();
    QVERIFY(!dialog.isFullScreen());
    QVERIFY(dialog.isVisible());
#if QT_CONFIG(sizegrip)
    QVERIFY(sizeGrip->isVisible());
#endif

    dialog.showFullScreen();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(dialog.isVisible());

    dialog.hide();
    QVERIFY(dialog.isFullScreen());
    QVERIFY(!dialog.isVisible());

    dialog.setVisible(true);
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

void tst_QDialog::showAsTool()
{
#if defined(Q_OS_UNIX)
    QSKIP("Qt/X11: Skipped since activeWindow() is not respected by all window managers");
#endif
    DummyDialog testWidget;
    testWidget.resize(200, 200);
    testWidget.setWindowTitle(QTest::currentTestFunction());
    ToolDialog dialog(&testWidget);
    testWidget.show();
    testWidget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&testWidget));
    dialog.exec();
    if (testWidget.style()->styleHint(QStyle::SH_Widget_ShareActivation, 0, &testWidget)) {
        QCOMPARE(dialog.wasActive(), true);
    } else {
        QCOMPARE(dialog.wasActive(), false);
    }
}

// Verify that pos() returns the same before and after show()
// for a dialog with the Tool window type.
void tst_QDialog::toolDialogPosition()
{
    QDialog dialog(0, Qt::Tool);
    dialog.move(QPoint(100,100));
    const QPoint beforeShowPosition = dialog.pos();
    dialog.show();
    const int fuzz = int(dialog.devicePixelRatio());
    const QPoint afterShowPosition = dialog.pos();
    QVERIFY2(HighDpi::fuzzyCompare(afterShowPosition, beforeShowPosition, fuzz),
             HighDpi::msgPointMismatch(afterShowPosition, beforeShowPosition).constData());
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

#if QT_CONFIG(sizegrip)

void tst_QDialog::showSizeGrip()
{
    QDialog dialog(nullptr);
    dialog.show();
    new QWidget(&dialog);
    QVERIFY(!dialog.isSizeGripEnabled());

    dialog.setSizeGripEnabled(true);
    QPointer<QSizeGrip> sizeGrip = dialog.findChild<QSizeGrip *>();
    QVERIFY(sizeGrip);
    QVERIFY(sizeGrip->isVisible());
    QVERIFY(dialog.isSizeGripEnabled());

    dialog.setSizeGripEnabled(false);
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

#endif // QT_CONFIG(sizegrip)

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
        void reject() override
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

static QByteArray formatPoint(QPoint p)
{
    return QByteArray::number(p.x()) + ", " + QByteArray::number(p.y());
}

void tst_QDialog::snapToDefaultButton()
{
#ifdef QT_NO_CURSOR
    QSKIP("Test relies on there being a cursor");
#else
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("This platform does not support setting the cursor position.");

    const QRect dialogGeometry(QGuiApplication::primaryScreen()->availableGeometry().topLeft()
                               + QPoint(100, 100), QSize(200, 200));
    const QPoint startingPos = dialogGeometry.bottomRight() + QPoint(100, 100);
    QCursor::setPos(startingPos);
#ifdef Q_OS_MACOS
    // On OS X we use CGEventPost to move the cursor, it needs at least
    // some time before the event handled and the position really set.
    QTest::qWait(100);
#endif
    QCOMPARE(QCursor::pos(), startingPos);
    QDialog dialog;
    QPushButton *button = new QPushButton(&dialog);
    button->setDefault(true);
    dialog.setGeometry(dialogGeometry);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    const QPoint localPos = button->mapFromGlobal(QCursor::pos());
    if (QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::DialogSnapToDefaultButton).toBool())
        QVERIFY2(button->rect().contains(localPos), formatPoint(localPos).constData());
    else
        QVERIFY2(!button->rect().contains(localPos), formatPoint(localPos).constData());
#endif // !QT_NO_CURSOR
}

void tst_QDialog::transientParent_data()
{
    QTest::addColumn<bool>("nativewidgets");
    QTest::newRow("Non-native") << false;
    QTest::newRow("Native") << true;
}

void tst_QDialog::transientParent()
{
    QFETCH(bool, nativewidgets);
    QWidget topLevel;
    topLevel.resize(200, 200);
    topLevel.move(QGuiApplication::primaryScreen()->availableGeometry().center() - QPoint(100, 100));
    QVBoxLayout *layout = new QVBoxLayout(&topLevel);
    QWidget *innerWidget = new QWidget(&topLevel);
    layout->addWidget(innerWidget);
    if (nativewidgets)
        innerWidget->winId();
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QDialog dialog(innerWidget);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    // Transient parent should always be the top level, also when using
    // native child widgets.
    QCOMPARE(dialog.windowHandle()->transientParent(), topLevel.windowHandle());
}

void tst_QDialog::dialogInGraphicsView()
{
    // QTBUG-49124: A dialog embedded into QGraphicsView has Qt::WA_DontShowOnScreen
    // set (as has a native dialog). It must not trigger the modal handling though
    // as not to lock up.
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    view.resize(availableGeometry.size() / 2);
    view.move(availableGeometry.left() + availableGeometry.width() / 4,
              availableGeometry.top() + availableGeometry.height() / 4);
    ToolDialog *dialog = new ToolDialog;
    scene.addWidget(dialog);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    for (int i = 0; i < 3; ++i) {
        dialog->exec();
        QVERIFY(!dialog->wasModalWindow());
    }
}

QTEST_MAIN(tst_QDialog)
#include "tst_qdialog.moc"
