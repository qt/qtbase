// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QTimer>

static bool done = false;

static void cleanup()
{
     done = true;
     QEventLoop loop;
     QTimer::singleShot(100,&loop, &QEventLoop::quit);
     loop.exec();
}

struct tst_qAddPostRoutine : public QObject
{
public:
    tst_qAddPostRoutine();
    ~tst_qAddPostRoutine();
};

tst_qAddPostRoutine::tst_qAddPostRoutine()
{
    qAddPostRoutine(cleanup);
}

tst_qAddPostRoutine::~tst_qAddPostRoutine()
{
    Q_ASSERT(done);
}
int main(int argc, char *argv[])
{
    tst_qAddPostRoutine tc;
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}
