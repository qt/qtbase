// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QApplication>

#include <qtest.h>


class tst_qapplication : public QObject
{
    Q_OBJECT
private slots:
    void ctor();

    void compress_layout_requests_during_delivery_data();
    void compress_layout_requests_during_delivery();
};

/*
    Test the performance of the QApplication constructor.

    Note: results from the second start on can be misleading,
    since all global statics are already initialized.
*/
void tst_qapplication::ctor()
{
    // simulate reasonable argc, argv
    int argc = 1;
    char *argv[] = { const_cast<char*>("tst_qapplication") };
    QBENCHMARK {
        QApplication app(argc, argv);
    }
}

namespace {
// Posts a LayoutRequest to target whenever it receives a QEvent::User,
// i.e. while sendPostedEvents() is busy delivering the queue.
class LayoutRequestPoster : public QObject
{
public:
    explicit LayoutRequestPoster(QObject *target) : target(target) {}

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::User) {
            QCoreApplication::postEvent(target, new QEvent(QEvent::LayoutRequest));
            return true;
        }
        return QObject::event(e);
    }

private:
    QObject *target;
};
} // unnamed namespace

void tst_qapplication::compress_layout_requests_during_delivery_data()
{
    QTest::addColumn<int>("size");
    QTest::newRow("100 events") << 100;
    QTest::newRow("1000 events") << 1000;
    QTest::newRow("10000 events") << 10000;
}

void tst_qapplication::compress_layout_requests_during_delivery()
{
    QFETCH(int, size);

    int argc = 1;
    char *argv[] = { const_cast<char*>("tst_qapplication") };
    QApplication app(argc, argv);

    QObject target;
    LayoutRequestPoster poster(&target);

    QBENCHMARK {
        for (int i = 0; i < size; ++i)
            QCoreApplication::postEvent(&poster, new QEvent(QEvent::User));
        // pending LayoutRequest at the end of the queue: every LayoutRequest
        // posted while delivering the events above gets compressed into it
        QCoreApplication::postEvent(&target, new QEvent(QEvent::LayoutRequest),
                                    Qt::LowEventPriority);
        QCoreApplication::sendPostedEvents();
    }
}

QTEST_APPLESS_MAIN(tst_qapplication)

#include "main.moc"
