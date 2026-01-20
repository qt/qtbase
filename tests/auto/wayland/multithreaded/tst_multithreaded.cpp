// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2020 UBports Foundataion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <unistd.h>
#include <poll.h>

#include <wayland-client-core.h>

#include <QtGui/QScreen>
#include <QAbstractEventDispatcher>
#include <qpa/qplatformnativeinterface.h>

#include "mockcompositor.h"

#include <memory>

using namespace MockCompositor;

/*
 * This class simulate a thread from another library which use poll() to wait
 * for data from Wayland compositor.
 */

class ExternalWaylandReaderThread : public QThread
{
public:
    ExternalWaylandReaderThread(struct wl_display *disp)
        : QThread()
        , m_disp(disp)
    {
        setObjectName(QStringLiteral("ExternalWaylandReader"));
    }

    ~ExternalWaylandReaderThread()
    {
        if (m_pipefd[1] != -1 && write(m_pipefd[1], "q", 1) == -1)
            qWarning("Failed to write to the pipe: %s.", strerror(errno));

        wait();
    }

protected:
    void run() override
    {
        // we use this pipe to make the loop exit otherwise if we simply used a flag on the loop condition, if stop() gets
        // called while poll() is blocking the thread will never quit since there are no wayland messages coming anymore.
        struct Pipe
        {
            Pipe(int *fds)
                : fds(fds)
            {
                if (::pipe(fds) != 0)
                    qWarning("Pipe creation failed. Quitting may hang.");
            }
            ~Pipe()
            {
                if (fds[0] != -1) {
                    close(fds[0]);
                    close(fds[1]);
                }
            }

            int *fds;
        } pipe(m_pipefd);

        struct wl_event_queue *a_queue = wl_display_create_queue(m_disp);
        struct pollfd fds[2] = { { wl_display_get_fd(m_disp), POLLIN, 0 },
                                 { m_pipefd[0], POLLIN, 0 } };

        while (true) {
            // No wl_proxy is assigned to this queue, thus guaranteed to be always empty.
            Q_ASSERT(wl_display_prepare_read_queue(m_disp, a_queue) == 0);
            wl_display_flush(m_disp);

            // Wakeup every 10 seconds so that if Qt blocks in _read_events(),
            // it won't last forever.
            poll(fds, /* nfds */ 2, 10000);

            if (fds[0].revents & POLLIN) {
                wl_display_read_events(m_disp);
            } else {
                wl_display_cancel_read(m_disp);
            }

            if (fds[1].revents & POLLIN) {
                char pipeIn;
                if (read(m_pipefd[0], &pipeIn, 1) == 1 && pipeIn == 'q')
                    break;
            }
        }

        wl_event_queue_destroy(a_queue);
    }

private:
    struct wl_display *m_disp;
    int m_pipefd[2] = { -1, -1 };
};

class tst_multithreaded : public QObject, private DefaultCompositor
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        m_config.autoConfigure = true;
        m_config.autoEnter = false;
    }
    void init()
    {
        // a test case is given new simulated thread.
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        struct wl_display *wl_dpy =
                (struct wl_display *)native->nativeResourceForWindow("display", NULL);

        m_extThread.reset(new ExternalWaylandReaderThread(wl_dpy));
        m_extThread->start();
    }
    void cleanup()
    {
        QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
    }

    void mainThreadIsNotBlocked();

public:
    std::unique_ptr<ExternalWaylandReaderThread> m_extThread;
};

void tst_multithreaded::mainThreadIsNotBlocked()
{
    QElapsedTimer timer;
    timer.start();

    QTest::qWait(100);
    QVERIFY(timer.elapsed() < 200);
}

QCOMPOSITOR_TEST_MAIN(tst_multithreaded)
#include "tst_multithreaded.moc"
