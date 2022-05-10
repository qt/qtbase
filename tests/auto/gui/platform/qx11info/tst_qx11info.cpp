// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui>
#include <QtTest/QtTest>

#include <QtGui/private/qtx11extras_p.h>

class tst_QX11Info : public QObject
{
    Q_OBJECT

private slots:
    void staticFunctionsBeforeQGuiApplication();
    void startupId();
    void isPlatformX11();
    void appTime();
    void peeker();
    void isCompositingManagerRunning();
};

void tst_QX11Info::staticFunctionsBeforeQGuiApplication()
{
    QVERIFY(!QGuiApplication::instance());

    // none of these functions should crash if QGuiApplication hasn't
    // been constructed

    Display *display = QX11Info::display();
    QCOMPARE(display, (Display *)0);

#if 0
    const char *appClass = QX11Info::appClass();
    QCOMPARE(appClass, (const char *)0);
#endif
    int appScreen = QX11Info::appScreen();
    QCOMPARE(appScreen, 0);
#if 0
    int appDepth = QX11Info::appDepth();
    QCOMPARE(appDepth, 32);
    int appCells = QX11Info::appCells();
    QCOMPARE(appCells, 0);
    Qt::HANDLE appColormap = QX11Info::appColormap();
    QCOMPARE(appColormap, static_cast<Qt::HANDLE>(0));
    void *appVisual = QX11Info::appVisual();
    QCOMPARE(appVisual, static_cast<void *>(0));
#endif
    unsigned long appRootWindow = QX11Info::appRootWindow();
    QCOMPARE(appRootWindow, static_cast<unsigned long>(0));

#if 0
    bool appDefaultColormap = QX11Info::appDefaultColormap();
    QCOMPARE(appDefaultColormap, true);
    bool appDefaultVisual = QX11Info::appDefaultVisual();
    QCOMPARE(appDefaultVisual, true);
#endif

    int appDpiX = QX11Info::appDpiX();
    int appDpiY = QX11Info::appDpiY();
    QCOMPARE(appDpiX, 75);
    QCOMPARE(appDpiY, 75);

    unsigned long appTime = QX11Info::appTime();
    unsigned long appUserTime = QX11Info::appUserTime();
    QCOMPARE(appTime, 0ul);
    QCOMPARE(appUserTime, 0ul);
    // setApp*Time do nothing without QGuiApplication
    QX11Info::setAppTime(1234);
    QX11Info::setAppUserTime(5678);
    appTime = QX11Info::appTime();
    appUserTime = QX11Info::appUserTime();
    QCOMPARE(appTime, 0ul);
    QCOMPARE(appTime, 0ul);

    QX11Info::isCompositingManagerRunning();
}

static const char idFromEnv[] = "startupid_TIME123456";
void initialize()
{
    qputenv("DESKTOP_STARTUP_ID", idFromEnv);
}
Q_CONSTRUCTOR_FUNCTION(initialize)

void tst_QX11Info::startupId()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("This test is only for X11, not Wayland.");

    // This relies on the fact that no widget was shown yet,
    // so please make sure this method is always the first test.
    QCOMPARE(QString(QX11Info::nextStartupId()), QString(idFromEnv));
    QWindow w;
    w.show();
    QVERIFY(QX11Info::nextStartupId().isEmpty());

    QByteArray idSecondWindow = "startupid2_TIME234567";
    QX11Info::setNextStartupId(idSecondWindow);
    QCOMPARE(QX11Info::nextStartupId(), idSecondWindow);

    QWindow w2;
    w2.show();
    QVERIFY(QX11Info::nextStartupId().isEmpty());
}

void tst_QX11Info::isPlatformX11()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("This test is only for X11, not Wayland.");

    QVERIFY(QX11Info::isPlatformX11());
}

void tst_QX11Info::appTime()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("This test is only for X11, not Wayland.");

    // No X11 event received yet
    QCOMPARE(QX11Info::appTime(), 0ul);
    QCOMPARE(QX11Info::appUserTime(), 0ul);

    // Trigger some X11 events
    QWindow window;
    window.show();
    QTest::qWait(100);
    QVERIFY(QX11Info::appTime() > 0);

    unsigned long t0 = QX11Info::appTime();
    unsigned long t1 = t0 + 1;
    QX11Info::setAppTime(t1);
    QCOMPARE(QX11Info::appTime(), t1);

    unsigned long u0 = QX11Info::appUserTime();
    unsigned long u1 = u0 + 1;
    QX11Info::setAppUserTime(u1);
    QCOMPARE(QX11Info::appUserTime(), u1);
}

class PeekerTest : public QWindow
{
public:
    PeekerTest()
    {
        setGeometry(100, 100, 400, 400);
        m_peekerFirstId = QX11Info::generatePeekerId();
        QVERIFY(m_peekerFirstId >= 0);
        m_peekerSecondId = QX11Info::generatePeekerId();
        QVERIFY(m_peekerSecondId == m_peekerFirstId + 1);
        // Get X atoms
        xcb_connection_t *c = QX11Info::connection();
        const char *first  = "QT_XCB_PEEKER_TEST_FIRST";
        const char *second = "QT_XCB_PEEKER_TEST_SECOND";
        xcb_intern_atom_reply_t *reply =
                xcb_intern_atom_reply(c, xcb_intern_atom(c, false, strlen(first), first), 0);
        QVERIFY2(reply != nullptr, first);
        m_atomFirst = reply->atom;
        free(reply);
        reply = xcb_intern_atom_reply(c, xcb_intern_atom(c, false, strlen(second), second), 0);
        QVERIFY2(reply != nullptr, second);
        m_atomSecond = reply->atom;
        free(reply);
    }

protected:
    void triggerPropertyNotifyEvents()
    {
        xcb_window_t rootWindow = QX11Info::appRootWindow();
        xcb_connection_t *connection = QX11Info::connection();
        xcb_change_property(connection, XCB_PROP_MODE_APPEND, rootWindow, m_atomFirst,
                            XCB_ATOM_INTEGER, 32, 0, nullptr);
        xcb_change_property(connection, XCB_PROP_MODE_APPEND, rootWindow, m_atomSecond,
                            XCB_ATOM_INTEGER, 32, 0, nullptr);
        xcb_flush(connection);
    }

    static bool checkForPropertyNotifyByAtom(xcb_generic_event_t *event, void *data)
    {
        bool isPropertyNotify = (event->response_type & ~0x80) == XCB_PROPERTY_NOTIFY;
        if (isPropertyNotify) {
            xcb_property_notify_event_t *pn =
                    reinterpret_cast<xcb_property_notify_event_t *>(event);
            xcb_atom_t *atom = static_cast<xcb_atom_t *>(data);
            if (pn->atom == *atom)
                return true;
        }
        return false;
    }

    bool sanityCheckPeeking(xcb_generic_event_t *event)
    {
        m_countWithCaching++;
        bool isPropertyNotify = (event->response_type & ~0x80) == XCB_PROPERTY_NOTIFY;
        if (isPropertyNotify) {
            xcb_property_notify_event_t *pn =
                    reinterpret_cast<xcb_property_notify_event_t *>(event);
            if (pn->atom == m_atomFirst) {
                if (m_indexFirst == -1) {
                    m_indexFirst = m_countWithCaching;
                    // continue peeking, maybe the second event is already in the queue
                    return false;
                }
                m_foundFirstEventAgain = true;
                // Return so we can fail the test with QVERIFY2, for more details
                // see QTBUG-62354
                return true;
            }
            // Let it peek to the end, even when the second event was found
            if (pn->atom == m_atomSecond) {
                m_indexSecond = m_countWithCaching;
                if (m_indexFirst == -1) {
                    m_foundSecondBeforeFirst = true;
                    // Same as above, see QTBUG-62354
                    return true;
                }
            }
        }
        return false;
    }

    void exposeEvent(QExposeEvent *) override
    {
        if (m_ignoreSubsequentExpose)
            return;
        // We don't want to execute this handler again in case there are more expose
        // events after calling QCoreApplication::processEvents() later in this test
        m_ignoreSubsequentExpose = true;

        xcb_flush(QX11Info::connection());
        bool found = QX11Info::peekEventQueue(checkForPropertyNotifyByAtom, &m_atomFirst);
        QVERIFY2(!found, "Found m_atomFirst which should not be in the queue yet");
        found = QX11Info::peekEventQueue(checkForPropertyNotifyByAtom, &m_atomSecond);
        QVERIFY2(!found, "Found m_atomSecond which should not be in the queue yet");

        triggerPropertyNotifyEvents();

        bool earlyExit = false;
        while (!earlyExit && (m_indexFirst == -1 || m_indexSecond == -1)) {

            earlyExit = QX11Info::peekEventQueue([](xcb_generic_event_t *event, void *data) {
                return static_cast<PeekerTest *>(data)->sanityCheckPeeking(event);
            }, this, QX11Info::PeekFromCachedIndex, m_peekerFirstId);

            if (m_countWithCaching == -1)
                QVERIFY2(!earlyExit, "Unexpected return value for an empty queue");
        }

        QVERIFY2(!m_foundFirstEventAgain,
                 "Found the same notify event twice, maybe broken index cache?");
        QVERIFY2(!m_foundSecondBeforeFirst, "Found second event before first");
        QVERIFY2(!earlyExit,
                 "Unexpected return value for a peeker that always scans to the end of the queue");

        found = QX11Info::peekEventQueue(checkForPropertyNotifyByAtom, &m_atomFirst,
                                         QX11Info::PeekDefault, m_peekerFirstId);
        QVERIFY2(found, "Failed to find m_atomFirst, when peeking from start and ignoring "
                        "the cached index associated with the passed in peeker id");
        // The above call updated index cache for m_peekerFirstId to the position of
        // event(m_atomFirst) + 1
        found = QX11Info::peekEventQueue(checkForPropertyNotifyByAtom, &m_atomSecond,
                                         QX11Info::PeekFromCachedIndex, m_peekerFirstId);
        QVERIFY2(found, "Unexpectedly failed to find m_atomSecond");

        QVERIFY(m_indexFirst <= m_countWithCaching);
        QVERIFY(m_indexSecond <= m_countWithCaching);
        QVERIFY(m_indexFirst < m_indexSecond);
        QX11Info::peekEventQueue([](xcb_generic_event_t *, void *data) {
            static_cast<PeekerTest *>(data)->m_countFromStart++;
            return false;
        }, this);
        QVERIFY(m_countWithCaching <= m_countFromStart);
        found = QX11Info::peekEventQueue(checkForPropertyNotifyByAtom, &m_atomFirst,
                                         QX11Info::PeekFromCachedIndex, m_peekerSecondId);
        QVERIFY2(found, "m_peekerSecondId failed to find the event");

        // Remove peeker id from within the peeker while using it for peeking
        QX11Info::peekEventQueue([](xcb_generic_event_t *, void *data) {
             PeekerTest *obj = static_cast<PeekerTest *>(data);
             QX11Info::removePeekerId(obj->m_peekerSecondId);
             return true;
        }, this, QX11Info::PeekFromCachedIndex, m_peekerSecondId);
        // Check that it really has been removed from the cache
        bool ok = QX11Info::peekEventQueue([](xcb_generic_event_t *, void *) {
             return true;
        }, nullptr, QX11Info::PeekFromCachedIndex, m_peekerSecondId);
        QVERIFY2(!ok, "Unexpected return value when attempting to peek from cached "
                      "index when peeker id has been removed from the cache");

        // Sanity check other input combinations
        QVERIFY(!QX11Info::removePeekerId(-99));
        ok = QX11Info::peekEventQueue([](xcb_generic_event_t *, void *) {
            return true;
        }, nullptr, QX11Info::PeekFromCachedIndex, -100);
        QVERIFY2(!ok, "PeekFromCachedIndex with invalid peeker id unexpectedly succeeded");
        ok = QX11Info::peekEventQueue([](xcb_generic_event_t *, void *) {
            return true;
        }, nullptr, QX11Info::PeekDefault, -100);
        QVERIFY2(!ok, "PeekDefault with invalid peeker id unexpectedly succeeded");
        ok = QX11Info::peekEventQueue([](xcb_generic_event_t *, void *) {
            return true;
        }, nullptr, QX11Info::PeekFromCachedIndex);
        QVERIFY2(!ok, "PeekFromCachedIndex without peeker id unexpectedly succeeded");
        ok = QX11Info::peekEventQueue([](xcb_generic_event_t *, void *) {
            return true;
        }, nullptr, QX11Info::PeekDefault);
        QVERIFY2(ok, "PeekDefault without peeker id unexpectedly failed");

        QCoreApplication::processEvents(); // Flush buffered events

        found = QX11Info::peekEventQueue(checkForPropertyNotifyByAtom, &m_atomFirst);
        QVERIFY2(!found, "Found m_atomFirst in the queue after flushing");
        found = QX11Info::peekEventQueue(checkForPropertyNotifyByAtom, &m_atomSecond);
        QVERIFY2(!found, "Found m_atomSecond in the queue after flushing");

        QVERIFY(QX11Info::removePeekerId(m_peekerFirstId));
        QVERIFY2(!QX11Info::removePeekerId(m_peekerFirstId),
                 "Removing the same peeker id twice unexpectedly succeeded");
#if 0
        qDebug() << "Buffered event queue size (caching)    : " << m_countWithCaching + 1;
        qDebug() << "Buffered event queue size (from start) : " << m_countFromStart + 1;
        qDebug() << "PropertyNotify[FIRST]  at              : " << m_indexFirst;
        qDebug() << "PropertyNotify[SECOND] at              : " << m_indexSecond;
#endif
    }

private:
    xcb_atom_t m_atomFirst = -1;
    xcb_atom_t m_atomSecond = -1;
    qint32 m_peekerFirstId = -1;
    qint32 m_peekerSecondId = -1;
    qint32 m_indexFirst = -1;
    qint32 m_indexSecond = -1;
    bool m_foundFirstEventAgain = false;
    qint32 m_countWithCaching = -1;
    qint32 m_countFromStart = -1;
    bool m_ignoreSubsequentExpose = false;
    bool m_foundSecondBeforeFirst = false;
};

void tst_QX11Info::peeker()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("This test is only for X11, not Wayland.");

    PeekerTest test;
    test.show();

    QVERIFY(QTest::qWaitForWindowExposed(&test));
}

void tst_QX11Info::isCompositingManagerRunning()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("This test is only for X11, not Wayland.");
    const bool b = QX11Info::isCompositingManagerRunning();
    Q_UNUSED(b);
    const bool b2 = QX11Info::isCompositingManagerRunning(0);
    Q_UNUSED(b2);
    // just check that it didn't crash (QTBUG-91913)
}

QTEST_APPLESS_MAIN(tst_QX11Info)

#include "tst_qx11info.moc"
