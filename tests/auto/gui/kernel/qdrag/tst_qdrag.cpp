// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QMimeData>

#include <qcoreapplication.h>
#include <qcursor.h>
#include <qdebug.h>
#include <qdrag.h>
#include <qevent.h>
#include <qguiapplication.h>
#include <qpixmap.h>
#include <qpointer.h>
#include <qregularexpression.h>
#include <qtimer.h>
#include <qwindow.h>

#include <qpa/qwindowsysteminterface.h>

#include <functional>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

/*
    QDrag::exec() runs a nested event loop that, on most platforms, only a real
    user can terminate. The exception is the in-process QSimpleDrag,
    which QPlatformIntegration provides by default, so that synthetic input is enough.

    A settable QPlatformCursor is also needed, because QSimpleDrag::startDrag()
    resolves the source window from QCursor::pos() rather than from the last
    mouse event. The offscreen plugin has both, which makes it the one platform
    where these tests are deterministic. ("minimal" has no QPlatformCursor)
*/
static bool haveInProcessDrag()
{
    return QGuiApplication::platformName() == "offscreen"_L1;
}

#define SKIP_IF_NO_IN_PROCESS_DRAG() \
    do { \
        if (!haveInProcessDrag()) \
            QSKIP("Needs the in-process QSimpleDrag; run with -platform offscreen"); \
    } while (false)

/*
    Input is posted synchrously rather than delivered with QTest::mouse*()/key*(),
    which is not usable from inside a drag: processEvents() clears the event
    dispatcher's interrupt flag and drains its wakeup pipe, so if the event
    ended the drag, the QEventLoop::exit() it triggered is swallowed and the
    drag loop keeps blocking until some later event happens to wake it.

    Posting instead lets the drag's own event loop drain the queue, which both preserves
    ordering (QCursor::setPos() posts too, so mixing the two reorders them) and leaves the
    exit request intact. This is what the FIXME on qt_handleMouseEvent() suggests testlib
    should do; see QTBUG-63146.
*/
static void postMouseMove(QWindow *window, const QPoint &local)
{
    QWindowSystemInterface::handleMouseEvent(window, local, window->mapToGlobal(local),
                                            Qt::LeftButton, Qt::NoButton, QEvent::MouseMove);
}

static void postMouseRelease(QWindow *window, const QPoint &local)
{
    QWindowSystemInterface::handleMouseEvent(window, local, window->mapToGlobal(local),
                                            Qt::NoButton, Qt::LeftButton,
                                            QEvent::MouseButtonRelease);
}

static void postKeyPress(QWindow *window, Qt::Key key)
{
    QWindowSystemInterface::handleKeyEvent(window, QEvent::KeyPress, key, Qt::NoModifier);
}

// Records the drag events it receives
class DropWindow : public QWindow
{
    Q_OBJECT
public:
    explicit DropWindow(const QRect &rect, const QString &name)
    {
        setObjectName(name);
        setFlag(Qt::FramelessWindowHint);
        setGeometry(rect);
    }

    // Set to Qt::IgnoreAction to refuse the drag.
    Qt::DropAction acceptedAction = Qt::CopyAction;
    QStringList log;
    int enterCount = 0;
    int moveCount = 0;
    int leaveCount = 0;
    int dropCount = 0;
    QObject *lastSource = nullptr;
    Qt::DropActions lastSupportedActions = {};
    // Invoked once from dragMoveEvent, i.e. from inside the sender's QDrag::exec().
    std::function<void()> onMove;

protected:
    bool event(QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::DragEnter: {
            auto *e = static_cast<QDragEnterEvent *>(event);
            ++enterCount;
            log << u"enter"_s;
            lastSource = e->source();
            lastSupportedActions = e->possibleActions();
            if (acceptedAction != Qt::IgnoreAction) {
                e->setDropAction(acceptedAction);
                e->accept();
            }
            return true;
        }
        case QEvent::DragMove: {
            auto *e = static_cast<QDragMoveEvent *>(event);
            ++moveCount;
            log << u"move"_s;
            if (acceptedAction != Qt::IgnoreAction) {
                e->setDropAction(acceptedAction);
                e->accept();
            }
            if (onMove) {
                // Take a copy and clear first: the callback may destroy this window.
                const auto callback = onMove;
                onMove = nullptr;
                callback();
            }
            return true;
        }
        case QEvent::DragLeave:
            ++leaveCount;
            log << u"leave"_s;
            return true;
        case QEvent::Drop: {
            auto *e = static_cast<QDropEvent *>(event);
            ++dropCount;
            log << u"drop"_s;
            if (acceptedAction != Qt::IgnoreAction) {
                e->setDropAction(acceptedAction);
                e->accept();
            }
            return true;
        }
        default:
            break;
        }
        return QWindow::event(event);
    }
};

class tst_QDrag : public QObject
{
Q_OBJECT

private slots:
    void init();

    void getSetCheck();

    void execBetweenWindows();
    void execRefused();
    void leaveOnWindowChange();
    void escapeCancels();
    void cancelFromApi();
    void dragObjectIsDeletedAfterExec();
    void dragDeletedDuringExec();
    void sourceDestroyedDuringExec();
    void iconWindowIsNeverADropTarget();

private:
    // Shows a window and moves the "cursor" into it, so that QSimpleDrag::startDrag()
    // picks it as the source window.
    static bool showAndGrabCursor(QWindow *window, const QPoint &localPos);
    // Presses the left button in source, then runs drag->exec(), invoking insideLoop
    // from within the nested event loop. Returns the drop action exec() reported.
    static Qt::DropAction runDrag(QDrag *drag, DropWindow *source, const QPoint &pressPos,
                                  Qt::DropActions supportedActions,
                                  std::function<void()> insideLoop);
};

void tst_QDrag::init()
{
    // runDrag() warns when its watchdog has to break into a stuck nested event loop.
    // Without this the tests would still fail, but with unrelated-looking messages.
    QTest::failOnWarning(QRegularExpression(u"^watchdog:"_s));
}

bool tst_QDrag::showAndGrabCursor(QWindow *window, const QPoint &localPos)
{
    window->show();
    if (!QTest::qWaitForWindowExposed(window))
        return false;
    // QSimpleDrag::startDrag() resolves the source window from QCursor::pos(), not from
    // the last mouse event, so this is required -- but only here, before the drag: once
    // it is running, QSimpleDrag tracks the mouse events themselves. QOffscreenCursor
    // updates the reported position immediately, with no round trip to a compositor.
    QCursor::setPos(window->mapToGlobal(localPos));
    QCoreApplication::processEvents();
    return QCursor::pos() == window->mapToGlobal(localPos);
}

Qt::DropAction tst_QDrag::runDrag(QDrag *drag, DropWindow *source, const QPoint &pressPos,
                                  Qt::DropActions supportedActions,
                                  std::function<void()> insideLoop)
{
    QTest::mousePress(source, Qt::LeftButton, {}, pressPos);

    // insideLoop is armed from the source window's first drag event. QSimpleDrag::startDrag()
    // delivers that synchronously, right after installing the application event filter
    // that makes synthetic input drive the drag; startDrag() returns immediately
    // afterwards, so the queued call lands in the nested event loop.
    if (insideLoop) {
        source->onMove = [source, insideLoop = std::move(insideLoop)] {
            QTimer::singleShot(0, source, insideLoop);
        };
    }

    // Fail rather than hang if nothing terminates the loop. Local, so it cannot outlive
    // this call and fire during a later test. QPointer because these tests deliberately
    // destroy the QDrag mid-flight.
    QPointer<QDrag> guard(drag);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    watchdog.setInterval(5s);
    QObject::connect(&watchdog, &QTimer::timeout, &watchdog, [guard] {
        qWarning("watchdog: nested drag event loop did not terminate");
        if (guard)
            guard->cancel();
    });
    watchdog.start();

    return drag->exec(supportedActions);
}

// Testing get/set functions
void tst_QDrag::getSetCheck()
{
    QDrag obj1(0);
    // QMimeData * QDrag::mimeData()
    // void QDrag::setMimeData(QMimeData *)
    QMimeData *var1 = new QMimeData;
    obj1.setMimeData(var1);
    QCOMPARE(var1, obj1.mimeData());
    obj1.setMimeData(var1);
    QCOMPARE(var1, obj1.mimeData());
    obj1.setMimeData((QMimeData *)0);
    QCOMPARE((QMimeData *)0, obj1.mimeData());
    // delete var1; // No delete, since QDrag takes ownership

    // Both of these return early at QDrag::exec()'s "no mime data" check, without ever
    // reaching QDragManager or the platform, so this stays valid -- and terminating --
    // whichever QPlatformDrag is installed.
    Qt::DropAction result = obj1.exec();
    QCOMPARE(result, Qt::IgnoreAction);
    result = obj1.exec(Qt::MoveAction | Qt::LinkAction);
    QCOMPARE(result, Qt::IgnoreAction);
}

// The happy path: drag from one window into another that accepts it, and drop.
void tst_QDrag::execBetweenWindows()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(100, 100, 200, 200), u"source"_s);
    DropWindow target(QRect(400, 100, 200, 200), u"target"_s);
    source.acceptedAction = Qt::IgnoreAction;   // only the target accepts
    target.acceptedAction = Qt::MoveAction;
    target.show();
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *mimeData = new QMimeData;
    mimeData->setData("application/x-tst-qdrag"_L1, "payload");
    auto *drag = new QDrag(&source);
    drag->setMimeData(mimeData);

    const QPoint dropPoint(50, 50);
    const Qt::DropAction result = runDrag(drag, &source, QPoint(50, 50),
                                          Qt::CopyAction | Qt::MoveAction, [&] {
        postMouseMove(&target, dropPoint);
        postMouseRelease(&target, dropPoint);
    });

    QCOMPARE(result, Qt::MoveAction);
    QCOMPARE(target.enterCount, 1);
    QCOMPARE_GE(target.moveCount, 1);
    QCOMPARE(target.dropCount, 1);
    QCOMPARE(target.leaveCount, 0);
    QCOMPARE(target.lastSource, &source);
    QCOMPARE(target.lastSupportedActions, Qt::CopyAction | Qt::MoveAction);
    // The source window is under the cursor when the drag starts, so it sees the drag
    // too, and must be left again on the way out.
    QCOMPARE(source.enterCount, source.leaveCount);
}

// A target that refuses the drag gets the events but exec() reports IgnoreAction.
void tst_QDrag::execRefused()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(100, 100, 200, 200), u"source"_s);
    DropWindow target(QRect(400, 100, 200, 200), u"target"_s);
    source.acceptedAction = Qt::IgnoreAction;
    target.acceptedAction = Qt::IgnoreAction;
    target.show();
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *drag = new QDrag(&source);
    drag->setMimeData(new QMimeData);

    const QPoint dropPoint(50, 50);
    const Qt::DropAction result = runDrag(drag, &source, QPoint(50, 50), Qt::CopyAction, [&] {
        postMouseMove(&target, dropPoint);
        postMouseRelease(&target, dropPoint);
    });

    QCOMPARE(result, Qt::IgnoreAction);
    QCOMPARE(target.enterCount, 1);
    // A refused drag is cancelled rather than dropped, so no QDropEvent.
    QCOMPARE(target.dropCount, 0);
    QCOMPARE(target.leaveCount, 1);
}

// Every window the drag enters must be left again exactly once. This is the mechanism
// behind the premature DropArea.onExited reported in QTBUG-124663.
void tst_QDrag::leaveOnWindowChange()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(0, 0, 150, 150), u"source"_s);
    DropWindow first(QRect(200, 0, 150, 150), u"first"_s);
    DropWindow second(QRect(400, 0, 150, 150), u"second"_s);
    source.acceptedAction = Qt::IgnoreAction;
    for (DropWindow *w : {&first, &second}) {
        w->acceptedAction = Qt::CopyAction;
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *drag = new QDrag(&source);
    drag->setMimeData(new QMimeData);

    runDrag(drag, &source, QPoint(50, 50), Qt::CopyAction, [&] {
        postMouseMove(&first, QPoint(50, 50));
        postMouseMove(&second, QPoint(50, 50));
        // ... and back to the first, which must leave the second.
        postMouseMove(&first, QPoint(50, 50));
        postMouseRelease(&first, QPoint(50, 50));
    });

    QCOMPARE(first.log, QStringList({u"enter"_s, u"move"_s, u"leave"_s,
                                     u"enter"_s, u"move"_s, u"drop"_s}));
    QCOMPARE(second.log, QStringList({u"enter"_s, u"move"_s, u"leave"_s}));
    QCOMPARE(source.enterCount, source.leaveCount);
}

// Escape aborts the drag: QBasicDrag::eventFilter -> cancel() -> exitDndEventLoop().
void tst_QDrag::escapeCancels()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(100, 100, 200, 200), u"source"_s);
    DropWindow target(QRect(400, 100, 200, 200), u"target"_s);
    source.acceptedAction = Qt::IgnoreAction;
    target.acceptedAction = Qt::CopyAction;
    target.show();
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *drag = new QDrag(&source);
    drag->setMimeData(new QMimeData);

    const Qt::DropAction result = runDrag(drag, &source, QPoint(50, 50), Qt::CopyAction, [&] {
        postMouseMove(&target, QPoint(50, 50));
        postKeyPress(&target, Qt::Key_Escape);
    });

    QCOMPARE(result, Qt::IgnoreAction);
    QCOMPARE(target.enterCount, 1);
    QCOMPARE(target.dropCount, 0);
    QCOMPARE(target.leaveCount, 1);
}

// QDrag::cancel() is public API used, among other places, as a watchdog by
// tst_QWidget_window::tst_dnd_events(), but has never had a test of its own.
void tst_QDrag::cancelFromApi()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(100, 100, 200, 200), u"source"_s);
    DropWindow target(QRect(400, 100, 200, 200), u"target"_s);
    source.acceptedAction = Qt::IgnoreAction;
    target.acceptedAction = Qt::CopyAction;
    target.show();
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *drag = new QDrag(&source);
    drag->setMimeData(new QMimeData);

    const Qt::DropAction result = runDrag(drag, &source, QPoint(50, 50), Qt::CopyAction, [&] {
        postMouseMove(&target, QPoint(50, 50));
        // cancel() is a direct call rather than an event, so the move above has to have
        // been handled first: it must find the drag already over the target, otherwise
        // it leaves the source instead.
        QCoreApplication::processEvents();
        drag->cancel();
    });

    QCOMPARE(result, Qt::IgnoreAction);
    QCOMPARE(target.enterCount, 1);
    QCOMPARE(target.dropCount, 0);
    QCOMPARE(target.leaveCount, 1);
}

// QDragManager::drag() deletes the QDrag once exec() returns, unless the platform
// implementation claims ownership of it (QPlatformDrag::ownsDragObject()). QSimpleDrag
// does not, so the QDrag must be gone after the deferred deletes are processed.
void tst_QDrag::dragObjectIsDeletedAfterExec()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(100, 100, 200, 200), u"source"_s);
    source.acceptedAction = Qt::CopyAction;
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *drag = new QDrag(&source);
    drag->setMimeData(new QMimeData);
    QPointer<QDrag> guard(drag);

    runDrag(drag, &source, QPoint(50, 50), Qt::CopyAction, [&] {
        postMouseRelease(&source, QPoint(60, 60));
    });

    // deleteLater() only queues the deletion, so flush it explicitly.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(guard.isNull());
}

// QTBUG-124663, at the QtGui level. A QDrag is a QObject child of whatever was passed to
// its constructor, so anything that destroys that parent from inside the nested event
// loop destroys the QDrag mid-flight. The drag must unwind cleanly and exec() must
// report IgnoreAction rather than touching freed memory.
void tst_QDrag::dragDeletedDuringExec()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(100, 100, 200, 200), u"source"_s);
    DropWindow target(QRect(400, 100, 200, 200), u"target"_s);
    source.acceptedAction = Qt::IgnoreAction;
    target.acceptedAction = Qt::CopyAction;
    target.show();
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *drag = new QDrag(&source);
    drag->setMimeData(new QMimeData);
    QPointer<QDrag> guard(drag);

    // Delete the QDrag from a drag-move handler, i.e. from as deep inside the nested
    // loop as an application can get.
    target.onMove = [&] { delete drag; };

    const Qt::DropAction result = runDrag(drag, &source, QPoint(50, 50), Qt::CopyAction, [&] {
        // No release: destroying the QDrag must be enough to end the drag. If it is
        // not, the watchdog in runDrag() fails the test instead of hanging.
        postMouseMove(&target, QPoint(50, 50));
    });

    QVERIFY(guard.isNull());
    QCOMPARE(result, Qt::IgnoreAction);
    QCOMPARE(target.dropCount, 0);
    QCOMPARE(target.leaveCount, 1);   // the target must not be left in an entered state
}

// Same, one level out: the QDrag's parent (its source) is destroyed, which destroys the
// QDrag with it. This is exactly what a view delegate being recycled mid-drag does.
void tst_QDrag::sourceDestroyedDuringExec()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow anchor(QRect(100, 100, 200, 200), u"anchor"_s);
    DropWindow target(QRect(400, 100, 200, 200), u"target"_s);
    anchor.acceptedAction = Qt::IgnoreAction;
    target.acceptedAction = Qt::CopyAction;
    target.show();
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(showAndGrabCursor(&anchor, QPoint(50, 50)));

    // A plain QObject stands in for the delegate: it is the QDrag's source and parent,
    // but is not the window, so destroying it does not tear down the scene.
    auto *dragSource = new QObject;
    auto *drag = new QDrag(dragSource);
    drag->setMimeData(new QMimeData);
    QPointer<QDrag> dragGuard(drag);

    target.onMove = [&] { delete dragSource; };

    const Qt::DropAction result = runDrag(drag, &anchor, QPoint(50, 50), Qt::CopyAction, [&] {
        postMouseMove(&target, QPoint(50, 50));
    });

    QVERIFY(dragGuard.isNull());
    QCOMPARE(result, Qt::IgnoreAction);
    QCOMPARE(target.leaveCount, 1);
}

// QBasicDrag drags a QShapedPixmapWindow along under the cursor to show the drag
// feedback. It is a visible top level window whose geometry contains the cursor, so a
// naive hit test finds it -- QSimpleDrag has its own topLevelAt() that excludes it. If
// that filter regresses, a drag stops delivering events to any real window.
void tst_QDrag::iconWindowIsNeverADropTarget()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    DropWindow source(QRect(100, 100, 200, 200), u"source"_s);
    DropWindow target(QRect(400, 100, 200, 200), u"target"_s);
    source.acceptedAction = Qt::IgnoreAction;
    target.acceptedAction = Qt::CopyAction;
    target.show();
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(showAndGrabCursor(&source, QPoint(50, 50)));

    auto *drag = new QDrag(&source);
    drag->setMimeData(new QMimeData);
    // A pixmap makes the icon window bigger than 1x1, and a zero hot spot puts its
    // top left corner exactly under the cursor.
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::red);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(0, 0));

    int iconWindowsUnderCursor = 0;
    const QPoint dropPoint(50, 50);
    // Count from inside the target's own drag-move handler: by then QBasicDrag has
    // moved the icon window to the cursor.
    target.onMove = [&] {
        const QPoint globalPos = target.mapToGlobal(dropPoint);
        for (QWindow *w : QGuiApplication::topLevelWindows()) {
            if (w != &source && w != &target && w->isVisible()
                && w->geometry().contains(globalPos)) {
                ++iconWindowsUnderCursor;
            }
        }
    };
    const Qt::DropAction result = runDrag(drag, &source, QPoint(50, 50), Qt::CopyAction, [&] {
        postMouseMove(&target, dropPoint);
        postMouseRelease(&target, dropPoint);
    });

    // Sanity check: the icon window really was there to be confused with the target.
    QCOMPARE(iconWindowsUnderCursor, 1);
    QCOMPARE(result, Qt::CopyAction);
    QCOMPARE(target.enterCount, 1);
    QCOMPARE(target.dropCount, 1);
}

QTEST_MAIN(tst_QDrag)

#include "tst_qdrag.moc"
