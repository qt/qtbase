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
#include <QtCore/QTextStream>
#include <QtCore/QStringList>
#include <QtCore/QMimeData>
#include <QtCore/QPoint>
#include <qeventloop.h>
#include <qlist.h>

#include <qlistwidget.h>
#include <qpushbutton.h>
#include <qboxlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <private/qwindow_p.h>

static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

class tst_QWidget_window : public QObject
{
    Q_OBJECT

public:
    tst_QWidget_window(){};

public slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

private slots:
    void tst_min_max_size();
    void tst_min_max_size_data();
    void tst_move_show();
    void tst_show_move();
    void tst_show_move_hide_show();

    void tst_resize_show();
    void tst_show_resize();
    void tst_show_resize_hide_show();

    void tst_windowFilePathAndwindowTitle_data();
    void tst_windowFilePathAndwindowTitle();
    void tst_windowFilePath_data();
    void tst_windowFilePath();

    void tst_showWithoutActivating();
    void tst_paintEventOnSecondShow();

#ifndef QT_NO_DRAGANDDROP
    void tst_dnd();
#endif

    void tst_qtbug35600();
    void tst_updateWinId_QTBUG40681();
    void tst_recreateWindow_QTBUG40817();

    void tst_resize_count();
    void tst_move_count();

    void tst_eventfilter_on_toplevel();

    void QTBUG_50561_QCocoaBackingStore_paintDevice_crash();
};

void tst_QWidget_window::initTestCase()
{
}

void tst_QWidget_window::cleanupTestCase()
{
}

void tst_QWidget_window::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

/* Test if the maximum/minimum size constraints
 * are propagated from the widget to the QWidgetWindow
 * independently of whether they were set before or after
 * window creation (QTBUG-26745). */

void tst_QWidget_window::tst_min_max_size_data()
{
    QTest::addColumn<bool>("setMinMaxSizeBeforeShow");
    QTest::newRow("Set min/max size after show") << false;
    QTest::newRow("Set min/max size before show") << true;
}

void tst_QWidget_window::tst_min_max_size()
{
    QFETCH(bool, setMinMaxSizeBeforeShow);
    const QSize minSize(300, 400);
    const QSize maxSize(1000, 500);
    QWidget w1;
    setFrameless(&w1);
    (new QVBoxLayout(&w1))->addWidget(new QPushButton("Test"));
    if (setMinMaxSizeBeforeShow) {
        w1.setMinimumSize(minSize);
        w1.setMaximumSize(maxSize);
    }
    w1.show();
    if (!setMinMaxSizeBeforeShow) {
        w1.setMinimumSize(minSize);
        w1.setMaximumSize(maxSize);
    }
    QVERIFY(QTest::qWaitForWindowExposed(&w1));
    QCOMPARE(w1.windowHandle()->minimumSize(),minSize);
    QCOMPARE(w1.windowHandle()->maximumSize(), maxSize);
}

void tst_QWidget_window::tst_move_show()
{
    QWidget w;
    w.move(100, 100);
    w.show();
    QCOMPARE(w.pos(), QPoint(100, 100));
//    QCoreApplication::processEvents(QEventLoop::AllEvents, 3000);
}

void tst_QWidget_window::tst_show_move()
{
    QWidget w;
    w.show();
    w.move(100, 100);
    QCOMPARE(w.pos(), QPoint(100, 100));
//    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
}

void tst_QWidget_window::tst_show_move_hide_show()
{
    QWidget w;
    w.show();
    w.move(100, 100);
    w.hide();
    w.show();
    QCOMPARE(w.pos(), QPoint(100, 100));
//    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
}

void tst_QWidget_window::tst_resize_show()
{
    QWidget w;
    w.resize(200, 200);
    w.show();
    QCOMPARE(w.size(), QSize(200, 200));
//    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
}

void tst_QWidget_window::tst_show_resize()
{
    QWidget w;
    w.show();
    w.resize(200, 200);
    QCOMPARE(w.size(), QSize(200, 200));
//    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
}

void tst_QWidget_window::tst_show_resize_hide_show()
{
    QWidget w;
    w.show();
    w.resize(200, 200);
    w.hide();
    w.show();
    QCOMPARE(w.size(), QSize(200, 200));
//    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
}

class PaintTestWidget : public QWidget
{
public:
    int paintEventCount;

    explicit PaintTestWidget(QWidget *parent = Q_NULLPTR)
        : QWidget(parent)
        , paintEventCount(0)
    {}

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE
    {
        ++paintEventCount;
        QWidget::paintEvent(event);
    }
};

void tst_QWidget_window::tst_windowFilePathAndwindowTitle_data()
{
    QTest::addColumn<bool>("setWindowTitleBefore");
    QTest::addColumn<bool>("setWindowTitleAfter");
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("applicationName");
    QTest::addColumn<QString>("indyWindowTitle");
    QTest::addColumn<QString>("finalTitleBefore");
    QTest::addColumn<QString>("finalTitleAfter");

    QString validPath = QApplication::applicationFilePath();
    QString fileNameOnly = QFileInfo(validPath).fileName() + QLatin1String("[*]");
    QString windowTitle = QLatin1String("Here is a Window Title");
    QString defaultPlatString = fileNameOnly;

    QTest::newRow("never Set Title nor AppName") << false << false << validPath << QString() << windowTitle << defaultPlatString << defaultPlatString;
    QTest::newRow("set title after only, but no AppName") << false << true << validPath << QString() << windowTitle << defaultPlatString << windowTitle;
    QTest::newRow("set title before only, not AppName") << true << false << validPath << QString() << windowTitle << windowTitle << windowTitle;
    QTest::newRow("always set title, not appName") << true << true << validPath << QString() << windowTitle << windowTitle << windowTitle;

    QString appName = QLatin1String("Killer App"); // Qt4 used to make it part of windowTitle(), Qt5 doesn't anymore, the QPA plugin takes care of it.
    QString platString = fileNameOnly;

    QTest::newRow("never Set Title, yes AppName") << false << false << validPath << appName << windowTitle << platString << platString;
    QTest::newRow("set title after only, yes AppName") << false << true << validPath << appName << windowTitle << platString << windowTitle;
    QTest::newRow("set title before only, yes AppName") << true << false << validPath << appName << windowTitle << windowTitle << windowTitle;
    QTest::newRow("always set title, yes appName") << true << true << validPath << appName << windowTitle << windowTitle << windowTitle;
}

void tst_QWidget_window::tst_windowFilePathAndwindowTitle()
{
    QFETCH(bool, setWindowTitleBefore);
    QFETCH(bool, setWindowTitleAfter);
    QFETCH(QString, filePath);
    QFETCH(QString, applicationName);
    QFETCH(QString, indyWindowTitle);
    QFETCH(QString, finalTitleBefore);
    QFETCH(QString, finalTitleAfter);


    QWidget widget;
    QCOMPARE(widget.windowFilePath(), QString());

    if (!applicationName.isEmpty())
        qApp->setApplicationName(applicationName);
    else
        qApp->setApplicationName(QString());

    if (setWindowTitleBefore) {
        widget.setWindowTitle(indyWindowTitle);
    }
    widget.setWindowFilePath(filePath);
    QCOMPARE(widget.windowTitle(), finalTitleBefore);
    QCOMPARE(widget.windowFilePath(), filePath);

    if (setWindowTitleAfter) {
        widget.setWindowTitle(indyWindowTitle);
    }
    QCOMPARE(widget.windowTitle(), finalTitleAfter);
    QCOMPARE(widget.windowFilePath(), filePath);
}

void tst_QWidget_window::tst_windowFilePath_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("result");
    QTest::addColumn<bool>("again");
    QTest::addColumn<QString>("filePath2");
    QTest::addColumn<QString>("result2");

    QString validPath = QApplication::applicationFilePath();
    QString invalidPath = QLatin1String("::**Never a Real Path**::");

    QTest::newRow("never Set Path") << QString() << QString() << false << QString() << QString();
    QTest::newRow("never EVER Set Path") << QString() << QString() << true << QString() << QString();
    QTest::newRow("Valid Path") << validPath << validPath << false << QString() << QString();
    QTest::newRow("invalid Path") << invalidPath << invalidPath << false << QString() << QString();
    QTest::newRow("Valid Path then empty") << validPath << validPath << true << QString() << QString();
    QTest::newRow("invalid Path then empty") << invalidPath << invalidPath << true << QString() << QString();
    QTest::newRow("invalid Path then valid") << invalidPath << invalidPath << true << validPath << validPath;
    QTest::newRow("valid Path then invalid") << validPath << validPath << true << invalidPath << invalidPath;
}

void tst_QWidget_window::tst_windowFilePath()
{
    QFETCH(QString, filePath);
    QFETCH(QString, result);
    QFETCH(bool, again);
    QFETCH(QString, filePath2);
    QFETCH(QString, result2);

    QWidget widget;
    QCOMPARE(widget.windowFilePath(), QString());
    widget.setWindowFilePath(filePath);
    QCOMPARE(widget.windowFilePath(), result);
    if (again) {
        widget.setWindowFilePath(filePath2);
        QCOMPARE(widget.windowFilePath(), result2);
    }
}

void tst_QWidget_window::tst_showWithoutActivating()
{
    QString platformName = QGuiApplication::platformName().toLower();
    if (platformName == "cocoa")
        QSKIP("Cocoa: This fails. Figure out why.");
    else if (platformName != QStringLiteral("xcb")
            && platformName != QStringLiteral("windows")
            && platformName != QStringLiteral("ios"))
        QSKIP("Qt::WA_ShowWithoutActivating is currently supported only on xcb, windows, and ios platforms.");

    QWidget w1;
    w1.setAttribute(Qt::WA_ShowWithoutActivating);
    w1.show();
    QVERIFY(!QTest::qWaitForWindowActive(&w1));

    QWidget w2;
    w2.show();
    QVERIFY(QTest::qWaitForWindowActive(&w2));

    QWidget w3;
    w3.setAttribute(Qt::WA_ShowWithoutActivating);
    w3.show();
    QVERIFY(!QTest::qWaitForWindowActive(&w3));

    w3.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&w3));
}

void tst_QWidget_window::tst_paintEventOnSecondShow()
{
    PaintTestWidget w;
    w.show();
    w.hide();

    w.paintEventCount = 0;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QApplication::processEvents();
    QTRY_VERIFY(w.paintEventCount > 0);
}

#ifndef QT_NO_DRAGANDDROP

/* DnD test for QWidgetWindow (handleDrag*Event() functions).
 * Simulates a drop onto a QWidgetWindow of a top level widget
 * that has 3 child widgets in a vertical layout with a frame. Only the lower 2
 * child widgets accepts drops (QTBUG-22987), the bottom child has another child
 * that does not accept drops.
 * Sends a series of DnD events to the QWidgetWindow,
 * entering the top level at the top frame and move
 * down in steps of 5 pixels, drop onto the bottom widget.
 * The test compares the sequences of events received by the widgets in readable format.
 * It also checks whether the address of the mimedata received is the same as the
 * sending one, that is, no conversion/serialization of text mime data occurs in the
 * process. */

static const char *expectedLogC[] = {
    "Event at 11,1 ignored",
    "Event at 11,21 ignored",
    "Event at 11,41 ignored",
    "Event at 11,61 ignored",
    "Event at 11,81 ignored",
    "Event at 11,101 ignored",
    "acceptingDropsWidget1::dragEnterEvent at 1,11 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,121 accepted",
    "acceptingDropsWidget1::dragMoveEvent at 1,31 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,141 accepted",
    "acceptingDropsWidget1::dragMoveEvent at 1,51 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,161 accepted",
    "acceptingDropsWidget1::dragMoveEvent at 1,71 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,181 accepted",
    "acceptingDropsWidget1::dragLeaveEvent QDragLeaveEvent",
    "Event at 11,201 ignored",
    "acceptingDropsWidget2::dragEnterEvent at 1,11 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,221 accepted",
    "acceptingDropsWidget2::dragMoveEvent at 1,31 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,241 accepted",
    "acceptingDropsWidget2::dropEvent at 1,51 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,261 accepted",
    "acceptingDropsWidget1::dragEnterEvent at 10,10 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 0,0 accepted",
    "acceptingDropsWidget1::dragMoveEvent at 11,11 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 1,1 accepted",
    "acceptingDropsWidget1::dropEvent at 12,12 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 2,2 accepted"
};

// A widget that logs the DnD events it receives into a QStringList.
class DnDEventLoggerWidget : public QWidget
{
public:
    DnDEventLoggerWidget(QStringList *log, QWidget *w = 0) : QWidget(w), m_log(log) {}

protected:
    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *);

private:
    void formatDropEvent(const char *function, const QDropEvent *e, QTextStream &str) const;
    QStringList *m_log;
};

void DnDEventLoggerWidget::formatDropEvent(const char *function, const QDropEvent *e, QTextStream &str) const
{
    str << objectName() << "::" << function  << " at " << e->pos().x() << ',' << e->pos().y()
        << " action=" << e->dropAction()
        << ' ' << quintptr(e->mimeData()) << " '" << e->mimeData()->text() << '\'';
}

void DnDEventLoggerWidget::dragEnterEvent(QDragEnterEvent *e)
{
    e->accept();
    QString message;
    QTextStream str(&message);
    formatDropEvent("dragEnterEvent", e, str);
    m_log->push_back(message);
}

void DnDEventLoggerWidget::dragMoveEvent(QDragMoveEvent *e)
{
    e->accept();
    QString message;
    QTextStream str(&message);
    formatDropEvent("dragMoveEvent", e, str);
    m_log->push_back(message);
}

void DnDEventLoggerWidget::dragLeaveEvent(QDragLeaveEvent *e)
{
    e->accept();
    m_log->push_back(objectName() + QLatin1String("::") + QLatin1String("dragLeaveEvent") + QLatin1String(" QDragLeaveEvent"));
}

void DnDEventLoggerWidget::dropEvent(QDropEvent *e)
{
    e->accept();
    QString message;
    QTextStream str(&message);
    formatDropEvent("dropEvent", e, str);
    m_log->push_back(message);
}

static QString msgEventAccepted(const QDropEvent &e)
{
    QString message;
    QTextStream str(&message);
    str << "Event at " << e.pos().x() << ',' << e.pos().y() << ' ' << (e.isAccepted() ? "accepted" : "ignored");
    return message;
}

void tst_QWidget_window::tst_dnd()
{
    QStringList log;
    DnDEventLoggerWidget dndTestWidget(&log);

    dndTestWidget.setObjectName(QLatin1String("dndTestWidget"));
    dndTestWidget.setWindowTitle(dndTestWidget.objectName());
    dndTestWidget.resize(200, 300);

    QWidget *dropsRefusingWidget1 = new DnDEventLoggerWidget(&log, &dndTestWidget);
    dropsRefusingWidget1->setObjectName(QLatin1String("dropsRefusingWidget1"));
    dropsRefusingWidget1->resize(180, 80);
    dropsRefusingWidget1->move(10, 10);

    QWidget *dropsAcceptingWidget1 = new DnDEventLoggerWidget(&log, &dndTestWidget);
    dropsAcceptingWidget1->setAcceptDrops(true);
    dropsAcceptingWidget1->setObjectName(QLatin1String("acceptingDropsWidget1"));
    dropsAcceptingWidget1->resize(180, 80);
    dropsAcceptingWidget1->move(10, 110);

    // Create a native widget on top of dropsAcceptingWidget1 to check QTBUG-27336
    QWidget *nativeWidget = new QWidget(dropsAcceptingWidget1);
    nativeWidget->resize(160, 60);
    nativeWidget->move(10, 10);
    nativeWidget->winId();

    QWidget *dropsAcceptingWidget2 = new DnDEventLoggerWidget(&log, &dndTestWidget);
    dropsAcceptingWidget2->setAcceptDrops(true);
    dropsAcceptingWidget2->setObjectName(QLatin1String("acceptingDropsWidget2"));
    dropsAcceptingWidget2->resize(180, 80);
    dropsAcceptingWidget2->move(10, 210);

    QWidget *dropsRefusingWidget2 = new DnDEventLoggerWidget(&log, dropsAcceptingWidget2);
    dropsRefusingWidget2->setObjectName(QLatin1String("dropsRefusingDropsWidget2"));
    dropsRefusingWidget2->resize(160, 60);
    dropsRefusingWidget2->move(10, 10);

    dndTestWidget.show();
    qApp->setActiveWindow(&dndTestWidget);
    QVERIFY(QTest::qWaitForWindowActive(&dndTestWidget));

    QMimeData mimeData;
    mimeData.setText(QLatin1String("testmimetext"));

    // Simulate DnD events on the QWidgetWindow.
    QPoint position = QPoint(11, 1);
    QDragEnterEvent e(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QWindow *window = dndTestWidget.windowHandle();
    qApp->sendEvent(window, &e);
    log.push_back(msgEventAccepted(e));
    while (true) {
        position.ry() += 20;
        if (position.y() >= 250) {
            QDropEvent e(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
            qApp->sendEvent(window, &e);
            log.push_back(msgEventAccepted(e));
            break;
        } else {
            QDragMoveEvent e(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
            qApp->sendEvent(window, &e);
            log.push_back(msgEventAccepted(e));
        }
    }

    window = nativeWidget->windowHandle();
    QDragEnterEvent enterEvent(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    qApp->sendEvent(window, &enterEvent);
    log.push_back(msgEventAccepted(enterEvent));

    QDragMoveEvent moveEvent(QPoint(1, 1), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    qApp->sendEvent(window, &moveEvent);
    log.push_back(msgEventAccepted(moveEvent));

    QDropEvent dropEvent(QPoint(2, 2), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    qApp->sendEvent(window, &dropEvent);
    log.push_back(msgEventAccepted(dropEvent));

    // Compare logs.
    QStringList expectedLog;
    const int expectedLogSize = int(sizeof(expectedLogC) / sizeof(expectedLogC[0]));
    const QString mimeDataAddress = QString::number(quintptr(&mimeData));
    const QString mimeDataAddressPlaceHolder = QLatin1String("MIME_DATA_ADDRESS");
    for (int i= 0; i < expectedLogSize; ++i)
        expectedLog.push_back(QString::fromLatin1(expectedLogC[i]).replace(mimeDataAddressPlaceHolder, mimeDataAddress));

    QCOMPARE(log, expectedLog);
}
#endif

void tst_QWidget_window::tst_qtbug35600()
{
    QWidget w;
    w.show();

    QWidget *wA = new QWidget;
    QHBoxLayout *layoutA = new QHBoxLayout;

    QWidget *wB = new QWidget;
    layoutA->addWidget(wB);

    QWidget *wC = new QWidget;
    layoutA->addWidget(wC);

    wA->setLayout(layoutA);

    QWidget *wD = new QWidget;
    wD->setAttribute(Qt::WA_NativeWindow);
    wD->setParent(wB);

    QWidget *wE = new QWidget(wC, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput);
    wE->show();

    wA->setParent(&w);

    // QTBUG-35600: program may crash here or on exit
}

void tst_QWidget_window::tst_updateWinId_QTBUG40681()
{
    QWidget w;
    QVBoxLayout *vl = new QVBoxLayout(&w);
    QLabel *lbl = new QLabel("HELLO1");
    lbl->setAttribute(Qt::WA_NativeWindow);
    lbl->setObjectName("label1");
    vl->addWidget(lbl);
    w.setMinimumWidth(200);

    w.show();

    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QCOMPARE(lbl->winId(), lbl->windowHandle()->winId());

     // simulate screen change and notification
    QWindow *win = w.windowHandle();
    w.windowHandle()->destroy();
    lbl->windowHandle()->destroy();
    w.windowHandle()->create();
    lbl->windowHandle()->create();
    QWindowPrivate *p = qt_window_private(win);
    p->emitScreenChangedRecursion(win->screen());

    QCOMPARE(lbl->winId(), lbl->windowHandle()->winId());
}

void tst_QWidget_window::tst_recreateWindow_QTBUG40817()
{
    QTabWidget tab;

    QWidget *w = new QWidget;
    tab.addTab(w, "Tab1");
    QVBoxLayout *vl = new QVBoxLayout(w);
    QLabel *lbl = new QLabel("HELLO1");
    lbl->setObjectName("label1");
    vl->addWidget(lbl);
    w = new QWidget;
    tab.addTab(w, "Tab2");
    vl = new QVBoxLayout(w);
    lbl = new QLabel("HELLO2");
    lbl->setAttribute(Qt::WA_NativeWindow);
    lbl->setObjectName("label2");
    vl->addWidget(lbl);

    tab.show();

    QVERIFY(QTest::qWaitForWindowExposed(&tab));

    QWindow *win = tab.windowHandle();
    win->destroy();
    QWindowPrivate *p = qt_window_private(win);
    p->create(true);
    win->show();

    tab.setCurrentIndex(1);
}

class ResizeWidget : public QWidget
{
Q_OBJECT
public:
    ResizeWidget(QWidget *parent = 0)
        : QWidget(parent)
        , resizeCount(0)
    { }

    int resizeCount;

protected:
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE
    {
        resizeCount++;
    }
};

void tst_QWidget_window::tst_resize_count()
{
    {
        ResizeWidget resize;
        resize.show();
        QVERIFY(QTest::qWaitForWindowExposed(&resize));
        QCOMPARE(resize.resizeCount, 1);
        resize.resizeCount = 0;
        QSize size = resize.size();
        size.rwidth() += 10;
        resize.resize(size);
        QGuiApplication::sync();
        QTRY_COMPARE(resize.resizeCount, 1);

        resize.resizeCount = 0;

        ResizeWidget child(&resize);
        child.resize(200,200);
        child.winId();
        child.show();
        QVERIFY(QTest::qWaitForWindowExposed(&child));
        QGuiApplication::sync();
        QTRY_COMPARE(child.resizeCount, 1);
        child.resizeCount = 0;
        size = child.size();
        size.rwidth() += 10;
        child.resize(size);
        QGuiApplication::sync();
        QCOMPARE(resize.resizeCount, 0);
        QCOMPARE(child.resizeCount, 1);
    }
    {
        ResizeWidget parent;
        ResizeWidget child(&parent);
        child.resize(200,200);
        child.winId();
        parent.show();
        QVERIFY(QTest::qWaitForWindowExposed(&parent));
        parent.resizeCount = 0;
        QGuiApplication::sync();
        QTRY_COMPARE(child.resizeCount, 1);
        child.resizeCount = 0;
        QSize size = child.size();
        size.rwidth() += 10;
        child.resize(size);
        QGuiApplication::sync();
        QCOMPARE(parent.resizeCount, 0);
        QCOMPARE(child.resizeCount, 1);
    }

}

class MoveWidget : public QWidget
{
Q_OBJECT
public:
    MoveWidget(QWidget *parent = 0)
        : QWidget(parent)
        , moveCount(0)
    { }

    void moveEvent(QMoveEvent *) Q_DECL_OVERRIDE
    {
        moveCount++;
    }

    int moveCount;
};

void tst_QWidget_window::tst_move_count()
{
    MoveWidget move;
    move.move(500,500);
    move.show();
    QVERIFY(QTest::qWaitForWindowExposed(&move));
    QTRY_VERIFY(move.moveCount >= 1);
    move.moveCount = 0;

    move.move(220,250);
    QTRY_VERIFY(move.moveCount >= 1);
}

class EventFilter : public QObject
{
public:
    int eventCount;

    EventFilter()
        : QObject(),
          eventCount(0)
    {
    }

    static QEvent::Type filterEventType()
    {
        static int type = QEvent::registerEventType();
        return static_cast<QEvent::Type>(type);
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) Q_DECL_OVERRIDE
    {
        if (e->type() == filterEventType())
            ++eventCount;

        return QObject::eventFilter(o, e);
    }
};

void tst_QWidget_window::tst_eventfilter_on_toplevel()
{
    QWidget w;
    EventFilter filter;
    w.installEventFilter(&filter);
    w.show();
    QVERIFY(QTest::qWaitForWindowActive(&w));
    QVERIFY(w.isWindow());
    QCOMPARE(filter.eventCount, 0);

    // send an event not handled in a special way by QWidgetWindow::event,
    // and check that it's received by the event filter
    QCoreApplication::postEvent(w.windowHandle(), new QEvent(EventFilter::filterEventType()));
    QTRY_COMPARE(filter.eventCount, 1);
}

class ApplicationStateSaver
{
public:
    ApplicationStateSaver()
    {
        QApplication::setAttribute(Qt::AA_NativeWindows, true);
        QApplication::setQuitOnLastWindowClosed(false);
    }

    ~ApplicationStateSaver()
    {
        QApplication::setAttribute(Qt::AA_NativeWindows, false);
        QApplication::setQuitOnLastWindowClosed(true);
    }
};

void tst_QWidget_window::QTBUG_50561_QCocoaBackingStore_paintDevice_crash()
{
    // Keep application state clean if testcase fails
    ApplicationStateSaver as;

    QMainWindow w;
    w.addToolBar(new QToolBar(&w));
    w.show();
    QTest::qWaitForWindowExposed(&w);

    // Simulate window system close
    QCloseEvent *e = new QCloseEvent;
    e->accept();
    qApp->postEvent(w.windowHandle(), e);
    qApp->processEvents();

    // Show again
    w.show();
    qApp->processEvents();

    // No crash, all good.
    // Wrap up and leave
    w.close();
}

QTEST_MAIN(tst_QWidget_window)
#include "tst_qwidget_window.moc"
