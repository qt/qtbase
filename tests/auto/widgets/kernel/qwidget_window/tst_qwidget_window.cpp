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
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformdrag.h>
#include <private/qhighdpiscaling_p.h>

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

// Compare a window position that may go through scaling in the platform plugin with fuzz.
static inline bool qFuzzyCompareWindowPosition(const QPoint &p1, const QPoint p2, int fuzz)
{
    return (p1 - p2).manhattanLength() <= fuzz;
}

static QString msgPointMismatch(const QPoint &p1, const QPoint p2)
{
    QString result;
    QDebug(&result) << p1 << "!=" << p2 << ", manhattanLength=" << (p1 - p2).manhattanLength();
    return result;
}

class tst_QWidget_window : public QObject
{
    Q_OBJECT

public:
    tst_QWidget_window();

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
    void tst_exposeObscuredMapped_QTBUG39220();
    void tst_paintEventOnResize_QTBUG50796();

#if QT_CONFIG(draganddrop)
    void tst_dnd();
    void tst_dnd_events();
    void tst_dnd_propagation();
#endif

    void tst_qtbug35600();
    void tst_updateWinId_QTBUG40681();
    void tst_recreateWindow_QTBUG40817();

    void tst_resize_count();
    void tst_move_count();

    void tst_showhide_count();

    void tst_eventfilter_on_toplevel();

    void QTBUG_50561_QCocoaBackingStore_paintDevice_crash();

    void setWindowState_data();
    void setWindowState();

    void nativeShow();

    void QTBUG_56277_resize_on_showEvent();

    void mouseMoveWithPopup_data();
    void mouseMoveWithPopup();

private:
    QSize m_testWidgetSize;
    const int m_fuzz;
};

tst_QWidget_window::tst_QWidget_window() :
     m_fuzz(int(QHighDpiScaling::factor(QGuiApplication::primaryScreen())))
{
    const int screenWidth =  QGuiApplication::primaryScreen()->geometry().width();
    const int width = qMax(200, 100 * ((screenWidth + 500) / 1000));
    m_testWidgetSize = QSize(width, width);
}

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
    const QPoint pos(100, 100);
    w.move(pos);
    w.show();
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Winrt does not support move", Abort);
#endif
    QVERIFY2(qFuzzyCompareWindowPosition(w.pos(), pos, m_fuzz),
             qPrintable(msgPointMismatch(w.pos(), pos)));
}

void tst_QWidget_window::tst_show_move()
{
    QWidget w;
    w.show();
    const QPoint pos(100, 100);
    w.move(pos);
    QVERIFY2(qFuzzyCompareWindowPosition(w.pos(), pos, m_fuzz),
             qPrintable(msgPointMismatch(w.pos(), pos)));
}

void tst_QWidget_window::tst_show_move_hide_show()
{
    QWidget w;
    w.show();
    const QPoint pos(100, 100);
    w.move(pos);
    w.hide();
    w.show();
    QVERIFY2(qFuzzyCompareWindowPosition(w.pos(), pos, m_fuzz),
             qPrintable(msgPointMismatch(w.pos(), pos)));
}

void tst_QWidget_window::tst_resize_show()
{
    QWidget w;
    w.resize(m_testWidgetSize);
    w.show();
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Winrt does not support resize", Abort);
#endif
    QCOMPARE(w.size(), m_testWidgetSize);
}

void tst_QWidget_window::tst_show_resize()
{
    QWidget w;
    w.show();
    w.resize(m_testWidgetSize);
    QCOMPARE(w.size(), m_testWidgetSize);
}

void tst_QWidget_window::tst_show_resize_hide_show()
{
    QWidget w;
    w.show();
    w.resize(m_testWidgetSize);
    w.hide();
    w.show();
    QCOMPARE(w.size(), m_testWidgetSize);
}

class PaintTestWidget : public QWidget
{
public:
    int paintEventCount;

    explicit PaintTestWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , paintEventCount(0)
    {}

    void paintEvent(QPaintEvent *event) override
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
            && platformName != QStringLiteral("ios")
            && platformName != QStringLiteral("tvos")
            && platformName != QStringLiteral("watchos"))
        QSKIP("Qt::WA_ShowWithoutActivating is currently supported only on xcb, windows, and ios/tvos/watchos platforms.");

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

void tst_QWidget_window::tst_exposeObscuredMapped_QTBUG39220()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const auto integration = QGuiApplicationPrivate::platformIntegration();
    if (!integration->hasCapability(QPlatformIntegration::MultipleWindows)
        || !integration->hasCapability(QPlatformIntegration::NonFullScreenWindows)
        || QGuiApplication::platformName() == QLatin1String("winrt")) {
        QSKIP("The platform does not have the required capabilities");
    }
    // QTBUG-39220: Fully obscured parent widgets may not receive expose
    // events (as is the case for frameless, obscured parents on Windows).
    // Ensure Qt::WA_Mapped is set so updating works.
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = availableGeometry.size() / 6;
    QWidget topLevel;
    setFrameless(&topLevel);
    topLevel.resize(size);
    const QPoint sizeP(size.width(), size.height());
    topLevel.move(availableGeometry.center() - sizeP / 2);
    QWidget *child = new QWidget(&topLevel);
    child->resize(size);
    child->move(0, 0);
    QVERIFY(child->winId());
    topLevel.show();
    QTRY_VERIFY(child->testAttribute(Qt::WA_Mapped));
    QVERIFY(topLevel.testAttribute(Qt::WA_Mapped));
}

void tst_QWidget_window::tst_paintEventOnResize_QTBUG50796()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const QRect availableGeo = QGuiApplication::primaryScreen()->availableGeometry();

    QWidget root;
    root.setGeometry(availableGeo.width()/2 - 100, availableGeo.height()/2 - 100,
                     200, 200);

    PaintTestWidget *native = new PaintTestWidget(&root);
    native->winId(); // We're testing native widgets
    native->setGeometry(10, 10, 50, 50);

    root.show();
    QVERIFY(QTest::qWaitForWindowExposed(&root));
    QVERIFY(QTest::qWaitForWindowActive(&root));
    QVERIFY(native->isVisible());

    native->paintEventCount = 0;
    native->resize(native->width() + 10, native->height() + 10);
    QTest::qWait(50); // Wait for paint events
    QTRY_COMPARE(native->paintEventCount, 1); // Only one paint event must occur
}

#if QT_CONFIG(draganddrop)

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
    "acceptingDropsWidget1::dragMoveEvent at 1,11 action=1 MIME_DATA_ADDRESS 'testmimetext'",
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
    "acceptingDropsWidget2::dragMoveEvent at 1,11 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,221 accepted",
    "acceptingDropsWidget2::dragMoveEvent at 1,31 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,241 accepted",
    "acceptingDropsWidget2::dropEvent at 1,51 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,261 accepted",
    "acceptingDropsWidget3::dragEnterEvent at 1,21 action=1 MIME_DATA_ADDRESS 'testmimetext'",
    "Event at 11,281 accepted",
    "acceptingDropsWidget3::dragLeaveEvent QDragLeaveEvent",
    "Event at 11,301 ignored",
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
    DnDEventLoggerWidget(QStringList *log, QWidget *w = nullptr, bool ignoreDragMove = false)
        : QWidget(w), m_log(log), m_ignoreDragMove(ignoreDragMove)
    {}
protected:
    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *);

private:
    void formatDropEvent(const char *function, const QDropEvent *e, QTextStream &str) const;
    QStringList *m_log;
    bool m_ignoreDragMove;
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
    if (m_ignoreDragMove)
        return;
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
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

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

    QWidget *dropsAcceptingWidget3 = new DnDEventLoggerWidget(&log, &dndTestWidget, true);
    dropsAcceptingWidget3->setAcceptDrops(true);
    dropsAcceptingWidget3->setObjectName(QLatin1String("acceptingDropsWidget3"));
    // 260 + 40 = 300 = widget size, must not be more than that.
    // otherwise it will break WinRT because there the tlw is maximized every time
    // and this window will receive one more event
    dropsAcceptingWidget3->resize(180, 40);
    dropsAcceptingWidget3->move(10, 260);

    dndTestWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dndTestWidget));
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
        if (position.y() >= 250 && position.y() < 270) {
            QDropEvent e(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
            qApp->sendEvent(window, &e);
            log.push_back(msgEventAccepted(e));
        } else {
            QDragMoveEvent e(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
            qApp->sendEvent(window, &e);
            log.push_back(msgEventAccepted(e));
        }
        if (position.y() > 290)
            break;
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

    if (log.size() != expectedLog.size()) {
        for (int i = 0; i < log.size() && i < expectedLog.size(); ++i)
            QCOMPARE(log.at(i), expectedLog.at(i));
        const int iMin = std::min(log.size(), expectedLog.size());
        for (int i = iMin; i < log.size(); ++i)
            qDebug() << "log[" << i << "]:" << log.at(i);
        for (int i = iMin; i < expectedLog.size(); ++i)
            qDebug() << "exp[" << i << "]:" << log.at(i);
    }
    QCOMPARE(log, expectedLog);
}

class DnDEventRecorder : public QWidget
{
    Q_OBJECT
public:
    QString _dndEvents;
    DnDEventRecorder() { setAcceptDrops(true); }

protected:
    void mousePressEvent(QMouseEvent *)
    {
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/x-dnditemdata", "some data");
        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec();
    }

    void dragEnterEvent(QDragEnterEvent *e)
    {
        e->accept();
        _dndEvents.append(QStringLiteral("DragEnter "));
    }
    void dragMoveEvent(QDragMoveEvent *e)
    {
        e->accept();
        _dndEvents.append(QStringLiteral("DragMove "));
        emit dragMoveReceived();
    }
    void dragLeaveEvent(QDragLeaveEvent *e)
    {
        e->accept();
        _dndEvents.append(QStringLiteral("DragLeave "));
    }
    void dropEvent(QDropEvent *e)
    {
        e->accept();
        _dndEvents.append(QStringLiteral("DropEvent "));
    }

signals:
    void dragMoveReceived();
};

void tst_QWidget_window::tst_dnd_events()
{
    // Note: This test is somewhat a hack as testing DnD with qtestlib is not
    // supported at the moment. The test verifies that we get an expected event
    // sequence on dnd operation that does not move a mouse. This logic is implemented
    // in QGuiApplication, so we have to go via QWindowSystemInterface API (QTest::mouse*).
    const auto platformName = QGuiApplication::platformName().toLower();
    // The test is known to work with XCB and platforms that use the default dnd
    // implementation QSimpleDrag (e.g. qnx). Running on XCB should be sufficient to
    // catch regressions at cross platform code: QGuiApplication::processDrag/Leave().
    if (platformName != "xcb")
        return;

    const QString expectedDndEvents = "DragEnter DragMove DropEvent DragEnter DragMove "
                                      "DropEvent DragEnter DragMove DropEvent ";
    DnDEventRecorder dndWidget;
    dndWidget.setGeometry(100, 100, 200, 200);
    dndWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dndWidget));
    QVERIFY(QTest::qWaitForWindowActive(&dndWidget));

    // ### FIXME - QTBUG-35117 ???
    auto targetCenter = QPoint(dndWidget.width(), dndWidget.height()) / 2;
    auto targetCenterGlobal = dndWidget.mapToGlobal(targetCenter);
    QCursor::setPos(targetCenterGlobal);
    QVERIFY(QTest::qWaitFor([&]() { return QCursor::pos() == targetCenterGlobal; }));
    QCoreApplication::processEvents(); // clear mouse events generated from cursor

    auto window = dndWidget.window()->windowHandle();

    // Some dnd implementation rely on running internal event loops, so we have to use
    // the following queued signal hack to simulate mouse clicks in the widget.
    QObject::connect(&dndWidget, &DnDEventRecorder::dragMoveReceived, this, [=]() {
        QTest::mouseRelease(window, Qt::LeftButton);
    }, Qt::QueuedConnection);

    QTest::mousePress(window, Qt::LeftButton);
    QTest::mousePress(window, Qt::LeftButton);
    QTest::mousePress(window, Qt::LeftButton);

    QCOMPARE(dndWidget._dndEvents, expectedDndEvents);

    dndWidget._dndEvents.clear();
    dndWidget.disconnect();
    int step = 0;
    QObject::connect(&dndWidget, &DnDEventRecorder::dragMoveReceived, this, [window, &step]() {
        switch (step++) {
        case 0:
            QTest::keyPress(window, Qt::Key_Shift, Qt::ShiftModifier);
            break;
        case 1:
            QTest::keyRelease(window, Qt::Key_Shift, Qt::NoModifier);
            break;
        default:
            QTest::mouseRelease(window, Qt::LeftButton);
            break;
        }
    }, Qt::QueuedConnection);

    QTest::mousePress(window, Qt::LeftButton);
    const QString expectedDndWithModsEvents = "DragEnter DragMove DragMove DragMove DropEvent ";
    QCOMPARE(dndWidget._dndEvents, expectedDndWithModsEvents);
}

class DropTarget : public QWidget
{
public:
    explicit DropTarget()
    {
        setAcceptDrops(true);

        const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
        auto width = availableGeometry.width() / 6;
        auto height = availableGeometry.height() / 4;

        setGeometry(availableGeometry.x() + 200, availableGeometry.y() + 200, width, height);

        QLabel *label = new QLabel(QStringLiteral("Test"), this);
        label->setGeometry(40, 40, 60, 60);
        label->setAcceptDrops(true);
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        event->accept();
        mDndEvents.append("enter ");
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        event->acceptProposedAction();
    }

    void dragLeaveEvent(QDragLeaveEvent *) override
    {
        mDndEvents.append("leave ");
    }

    void dropEvent(QDropEvent *event) override
    {
        event->accept();
        mDndEvents.append("drop ");
    }

    QString mDndEvents;
};

void tst_QWidget_window::tst_dnd_propagation()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMimeData mimeData;
    mimeData.setText(QLatin1String("testmimetext"));

    DropTarget target;
    target.show();
    QVERIFY(QTest::qWaitForWindowActive(&target));

    Qt::DropActions supportedActions = Qt::DropAction::CopyAction;
    QWindow *window = target.windowHandle();

    auto posInsideDropTarget = QHighDpi::toNativePixels(QPoint(20, 20), window->screen());
    auto posInsideLabel      = QHighDpi::toNativePixels(QPoint(60, 60), window->screen());

    // Enter DropTarget.
    QWindowSystemInterface::handleDrag(window, &mimeData, posInsideDropTarget, supportedActions, {}, {});
    // Enter QLabel. This will propagate because default QLabel does
    // not accept the drop event in dragEnterEvent().
    QWindowSystemInterface::handleDrag(window, &mimeData, posInsideLabel, supportedActions, {}, {});
    // Drop on QLabel. DropTarget will get dropEvent(), because it accepted the event.
    QWindowSystemInterface::handleDrop(window, &mimeData, posInsideLabel, supportedActions, {}, {});

    QGuiApplication::processEvents();

    QCOMPARE(target.mDndEvents, "enter leave enter drop ");
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
    w.setMinimumWidth(m_testWidgetSize.width());

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
    tab.setMinimumWidth(m_testWidgetSize.width());

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
    void resizeEvent(QResizeEvent *) override
    {
        resizeCount++;
    }
};

void tst_QWidget_window::tst_resize_count()
{
    {
        ResizeWidget resize;
        resize.setWindowFlags(Qt::X11BypassWindowManagerHint);
        resize.show();
        QVERIFY(QTest::qWaitForWindowExposed(&resize));
#ifdef Q_OS_WINRT
        QEXPECT_FAIL("", "Winrt does not support resize", Abort);
#endif
        QCOMPARE(resize.resizeCount, 1);
        resize.resizeCount = 0;
        QSize size = resize.size();
        size.rwidth() += 10;
        resize.resize(size);
        QGuiApplication::sync();
        QTRY_COMPARE(resize.resizeCount, 1);

        resize.resizeCount = 0;

        ResizeWidget child(&resize);
        child.resize(m_testWidgetSize);
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
        parent.setWindowFlag(Qt::X11BypassWindowManagerHint);
        ResizeWidget child(&parent);
        child.resize(m_testWidgetSize);
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

/*!
    This test verifies that windows get a balanced number of show
    and hide events, no matter how the window was closed.
*/
void tst_QWidget_window::tst_showhide_count()
{
    class EventSpy : public QObject
    {
    public:
        EventSpy()
        {
            QApplication::instance()->installEventFilter(this);
        }

        int takeCount(QWidget *widget, QEvent::Type type) {
            const auto entry = Entry(widget, type);
            int count = counter[entry];
            counter[entry] = 0;
            return count;
        }
    protected:
        bool eventFilter(QObject *receiver, QEvent *event)
        {
            if (QWidget *widget = qobject_cast<QWidget*>(receiver)) {
                const auto entry = Entry(widget, event->type());
                ++counter[entry];
                return false;
            }
            return QObject::eventFilter(receiver, event);
        }
    private:
        using Entry = QPair<QWidget*, QEvent::Type>;
        QHash<Entry, int> counter;
    };

    EventSpy spy;

    QWidget w1;
    w1.setGeometry(100, 100, 200, 200);

    w1.show();
    QCOMPARE(spy.takeCount(&w1, QEvent::Show), 1);
    w1.hide();
    QCOMPARE(spy.takeCount(&w1, QEvent::Hide), 1);
    w1.close();
    QCOMPARE(spy.takeCount(&w1, QEvent::Close), 1);
    w1.show();
    QCOMPARE(spy.takeCount(&w1, QEvent::Show), 1);
    w1.close();
    QCOMPARE(spy.takeCount(&w1, QEvent::Hide), 1);
    QCOMPARE(spy.takeCount(&w1, QEvent::Close), 1);

    w1.show();
    QWidget *popup = new QWidget(&w1, Qt::Popup);
    popup->setGeometry(120, 120, 30, 30);
    popup->show();
    popup->close();
    QCOMPARE(spy.takeCount(popup, QEvent::Show), 1);
    QCOMPARE(spy.takeCount(popup, QEvent::Hide), 1);
    QCOMPARE(spy.takeCount(popup, QEvent::Close), 1);

    popup->show();

    // clicking outside the popup should close the popup
    QTest::mousePress(popup->window(), Qt::LeftButton, {}, QPoint(-10, -10));

    QCOMPARE(spy.takeCount(popup, QEvent::Show), 1);
    QCOMPARE(spy.takeCount(popup, QEvent::Hide), 1);
    QCOMPARE(spy.takeCount(popup, QEvent::Close), 1);
}


class MoveWidget : public QWidget
{
Q_OBJECT
public:
    MoveWidget(QWidget *parent = 0)
        : QWidget(parent)
        , moveCount(0)
    { }

    void moveEvent(QMoveEvent *) override
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
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (e->type() == filterEventType())
            ++eventCount;

        return QObject::eventFilter(o, e);
    }
};

void tst_QWidget_window::tst_eventfilter_on_toplevel()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

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
    w.setMinimumWidth(m_testWidgetSize.width());
    w.addToolBar(new QToolBar(&w));
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

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

void tst_QWidget_window::setWindowState_data()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QString platformName = QGuiApplication::platformName().toLower();

    QTest::addColumn<Qt::WindowStates>("state");
    QTest::newRow("0") << Qt::WindowStates();
    QTest::newRow("Qt::WindowMaximized") << Qt::WindowStates(Qt::WindowMaximized);
    QTest::newRow("Qt::WindowMinimized") << Qt::WindowStates(Qt::WindowMinimized);
    QTest::newRow("Qt::WindowFullScreen") << Qt::WindowStates(Qt::WindowFullScreen);

    if (platformName != "xcb" && platformName != "windows" && !platformName.startsWith("wayland")
        && platformName != "offscreen")
        return; // Combination of states is not preserved on all platforms.
    if (platformName == "xcb" && qgetenv("XDG_CURRENT_DESKTOP") != "KDE"
        && qgetenv("XDG_CURRENT_DESKTOP") != "Unity")
        return; // Not all window managers support state combinations.

    QTest::newRow("Qt::WindowMaximized|Qt::WindowMinimized")
        << (Qt::WindowMaximized | Qt::WindowMinimized);
    QTest::newRow("Qt::WindowFullScreen|Qt::WindowMinimized")
        << (Qt::WindowFullScreen | Qt::WindowMinimized);
    QTest::newRow("Qt::WindowMaximized|Qt::WindowFullScreen")
        << (Qt::WindowMaximized | Qt::WindowFullScreen);
    QTest::newRow("Qt::WindowMaximized|Qt::WindowFullScreen|Qt::WindowMinimized")
        << (Qt::WindowMaximized | Qt::WindowFullScreen | Qt::WindowMinimized);
}

void tst_QWidget_window::setWindowState()
{
    QFETCH(Qt::WindowStates, state);

    // This tests make sure that the states are preserved when the window is shown.

    QWidget w;
    w.setWindowState(state);
    QCOMPARE(w.windowState(), state);
    w.show();
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("0", "Winrt windows are maximized by default", Abort);
    QEXPECT_FAIL("Qt::WindowMinimized", "Winrt windows are maximized by default", Abort);
    QEXPECT_FAIL("Qt::WindowFullScreen", "Winrt windows are maximized by default", Abort);
#endif
    QCOMPARE(w.windowState(), state);
    QCOMPARE(w.windowHandle()->windowStates(), state);
    if (!(state & Qt::WindowMinimized))
        QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_COMPARE(w.windowState(), state);
    QCOMPARE(w.windowHandle()->windowStates(), state);

    // Minimizing keeps other states
    w.showMinimized();
    QCOMPARE(w.windowState(), state | Qt::WindowMinimized);
    QTest::qWait(200);
    QCOMPARE(w.windowState(), state | Qt::WindowMinimized);
    QCOMPARE(w.windowHandle()->windowStates(), state | Qt::WindowMinimized);
}

void tst_QWidget_window::nativeShow()
{
    // Verify that a native widget can be shown using the QWindow::setVisible() API
    QWidget w;
    w.winId();
    w.windowHandle()->setVisible(true);
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY(w.isVisible());

    // ... and that we can hide it
    w.windowHandle()->setVisible(false);
    QTRY_VERIFY(!w.isVisible());
}

class ResizedOnShowEventWidget : public QWidget
{
public:
    void showEvent(QShowEvent *) override
    {
        const auto *primaryScreen = QApplication::primaryScreen();
        auto newSize = primaryScreen->availableGeometry().size() / 4;
        if (newSize == geometry().size())
            newSize -= QSize(10, 10);
        resize(newSize);
    }
};

void tst_QWidget_window::QTBUG_56277_resize_on_showEvent()
{
    const auto platformName = QGuiApplication::platformName().toLower();
    if (platformName != "cocoa" && platformName != "windows")
        QSKIP("This can only be consistently tested on desktop platforms with well-known behavior.");

    ResizedOnShowEventWidget w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    const auto *screen = w.windowHandle()->screen();
    const auto geometry = w.geometry();
    const int frameHeight = geometry.top() - w.frameGeometry().top();
    const int topmostY = screen->availableGeometry().top() + frameHeight;
    QVERIFY(geometry.top() > topmostY || geometry.left() > screen->availableGeometry().left());
}

void tst_QWidget_window::mouseMoveWithPopup_data()
{
    QTest::addColumn<Qt::WindowType>("windowType");

    QTest::addRow("Dialog") << Qt::Dialog;
    QTest::addRow("Popup") << Qt::Popup;
}

void tst_QWidget_window::mouseMoveWithPopup()
{
    QFETCH(Qt::WindowType, windowType);

    class Window : public QWidget
    {
    public:
        Window(QWidget *parent = nullptr, Qt::WindowFlags flags = {})
        : QWidget(parent, flags|Qt::CustomizeWindowHint|Qt::FramelessWindowHint)
        {}

        QSize sizeHint() const
        {
            if (parent())
                return QSize(150, 100);
            return QSize(250, 250);
        }

        Window *popup = nullptr;
        Qt::WindowType type = Qt::Popup;
        int mousePressCount = 0;
        int mouseMoveCount = 0;
        int mouseReleaseCount = 0;
        void resetCounters()
        {
            mousePressCount = 0;
            mouseMoveCount = 0;
            mouseReleaseCount = 0;
        }
    protected:
        void mousePressEvent(QMouseEvent *event)
        {
            ++mousePressCount;

            if (event->button() == Qt::RightButton) {
                if (!popup)
                    popup = new Window(this, type);
                popup->move(event->globalPos());
                popup->show();
                if (!QTest::qWaitForWindowExposed(popup)) {
                    delete popup;
                    popup = nullptr;
                    QSKIP("Failed to expose popup window!");
                }
            } else {
                QWidget::mousePressEvent(event);
            }
        }
        void mouseReleaseEvent(QMouseEvent *event)
        {
            ++mouseReleaseCount;
            QWidget::mouseReleaseEvent(event);
        }
        void mouseMoveEvent(QMouseEvent *event)
        {
            ++mouseMoveCount;
            QWidget::mouseMoveEvent(event);
        }
    };
    Window topLevel;
    topLevel.setObjectName("topLevel");
    topLevel.type = windowType;
    topLevel.show();
    if (!QTest::qWaitForWindowExposed(&topLevel))
        QSKIP("Failed to expose window!");

    QCOMPARE(QApplication::activePopupWidget(), nullptr);
    QCOMPARE(QApplication::activeWindow(), &topLevel);

    QPoint mousePos = topLevel.geometry().center();
    QWindow *window = nullptr;
    Qt::MouseButtons buttons = {};
    auto mouseAction = [&](Qt::MouseButton button, QPoint offset = {}) -> QEvent::Type
    {
        QEvent::Type type;
        if (offset != QPoint()) {
            type = QEvent::MouseMove;
        } else if (buttons & button) {
            type = QEvent::MouseButtonRelease;
            buttons &= ~button;
        } else {
            Q_ASSERT(button != Qt::NoButton);
            type = QEvent::MouseButtonPress;
            buttons |= button;
            window = QApplication::activeWindow()->windowHandle();
        }

        mousePos += offset;

        if (!window)
            return QEvent::None;

        bool result = QWindowSystemInterface::handleMouseEvent(window, window->mapFromGlobal(mousePos),
                                                               mousePos, buttons, button, type);
        QCoreApplication::processEvents();
        if (type == QEvent::MouseButtonRelease && buttons == Qt::NoButton)
            window = nullptr;

        if (!result)
            return QEvent::None;
        return type;
    };

    QCOMPARE(mouseAction(Qt::RightButton), QEvent::MouseButtonPress);
    QCOMPARE(topLevel.mousePressCount, 1);
    QVERIFY(topLevel.popup);
    QCOMPARE(topLevel.popup->mousePressCount, 0);
    topLevel.popup->setObjectName(windowType == Qt::Popup ? "Popup" : "Dialog");
    QCOMPARE(QApplication::activePopupWidget(), windowType == Qt::Popup ? topLevel.popup : nullptr);
    // if popup, then popup gets the mouse move even though it didn't get any press
    QCOMPARE(mouseAction(Qt::NoButton, QPoint(10, 10)), QEvent::MouseMove);
    QCOMPARE(topLevel.mouseMoveCount, windowType == Qt::Popup ? 0 : 1);
    QCOMPARE(topLevel.popup->mouseMoveCount, windowType == Qt::Popup ? 1 : 0);
    // if popup, then popup gets the release even though it didn't get any press
    QCOMPARE(mouseAction(Qt::RightButton), QEvent::MouseButtonRelease);
    QCOMPARE(topLevel.mouseReleaseCount, windowType == Qt::Popup ? 0 : 1);
    QCOMPARE(topLevel.popup->mouseReleaseCount, windowType == Qt::Popup ? 1 : 0);

    Q_ASSERT(buttons == Qt::NoButton);
    topLevel.resetCounters();
    topLevel.popup->resetCounters();

    // nested popup, same procedure
    QCOMPARE(mouseAction(Qt::RightButton), QEvent::MouseButtonPress);
    QVERIFY(topLevel.popup);
    QCOMPARE(topLevel.popup->mousePressCount, 1);
    QVERIFY(topLevel.popup->popup);
    topLevel.popup->popup->setObjectName("NestedPopup");
    QCOMPARE(QApplication::activePopupWidget(), topLevel.popup->popup);
    QCOMPARE(topLevel.popup->popup->mousePressCount, 0);

    // nested popup is always a popup and grabs the mouse, so first popup gets nothing
    QCOMPARE(mouseAction(Qt::NoButton, QPoint(10, 10)), QEvent::MouseMove);
    QCOMPARE(topLevel.popup->mouseMoveCount, 0);
    QCOMPARE(topLevel.popup->popup->mouseMoveCount, 1);

    // nested popup gets the release, as before
    QCOMPARE(mouseAction(Qt::RightButton), QEvent::MouseButtonRelease);
    QCOMPARE(topLevel.popup->mouseReleaseCount, 0);
    QCOMPARE(topLevel.popup->popup->mouseReleaseCount, 1);

    Q_ASSERT(buttons == Qt::NoButton);

    // move mouse back into first popup
    mouseAction({}, QPoint(-15, -15));
    QVERIFY(!topLevel.popup->popup->geometry().contains(mousePos));
    QVERIFY(topLevel.popup->geometry().contains(mousePos));

    topLevel.popup->resetCounters();
    topLevel.popup->popup->resetCounters();

    // closing the nested popup by clicking into the first popup/dialog; the nested popup gets the press
    QCOMPARE(mouseAction(Qt::LeftButton), QEvent::MouseButtonPress);
    QCOMPARE(topLevel.popup->popup->mousePressCount, 1);
    QVERIFY(!topLevel.popup->popup->isVisible());
    QCOMPARE(QApplication::activePopupWidget(), windowType == Qt::Popup ? topLevel.popup : nullptr);
    QCOMPARE(QApplication::activeWindow(), windowType == Qt::Popup ? &topLevel : topLevel.popup);

    // the move event following a press that closed the active popup should NOT be delivered to the first popup
    QCOMPARE(mouseAction({}, QPoint(-10, -10)), QEvent::MouseMove);
    // dialogs might or might not get the event - platform specific behavior in Qt 5
    if (topLevel.popup->mouseMoveCount != 0)
        QEXPECT_FAIL("Dialog", "Platform specific behavior", Continue);
    QCOMPARE(topLevel.popup->mouseMoveCount, 0);
    QCOMPARE(topLevel.popup->popup->mouseMoveCount, 0);

    // but the release event will still be delivered to the first popup - dialogs might not get it
    QCOMPARE(mouseAction(Qt::LeftButton), QEvent::MouseButtonRelease);
    if (topLevel.popup->mouseMoveCount != 1
        && (QGuiApplication::platformName().startsWith(QLatin1String("xcb"), Qt::CaseInsensitive)
            || QGuiApplication::platformName().startsWith(QLatin1String("offscreen"), Qt::CaseInsensitive)))
        QEXPECT_FAIL("Dialog", "Platform specific behavior", Continue);
    QCOMPARE(topLevel.popup->mouseReleaseCount, 1);
}

QTEST_MAIN(tst_QWidget_window)
#include "tst_qwidget_window.moc"
