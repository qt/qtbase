// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qurl.h>
#include <qtest.h>

class tst_QUrl : public QObject
{
    Q_OBJECT

private slots:
    void emptyUrl();
    void relativeUrl();
    void absoluteUrl();
    void isRelative_data();
    void isRelative();
    void toLocalFile_data();
    void toLocalFile();
    void toString_data();
    void toString();
    void resolved_data();
    void resolved();
    void equality_data();
    void equality();
    void qmlPropertyWriteUseCase();

private:
    void generateFirstRunData();
};

void tst_QUrl::emptyUrl()
{
    QBENCHMARK {
        QUrl url;
    }
}

void tst_QUrl::relativeUrl()
{
    QBENCHMARK {
        QUrl url("pics/avatar.png");
    }
}

void tst_QUrl::absoluteUrl()
{
    QBENCHMARK {
        QUrl url("/tmp/avatar.png");
    }
}

void tst_QUrl::generateFirstRunData()
{
    QTest::addColumn<bool>("firstRun");

    QTest::newRow("construction + first run") << true;
    QTest::newRow("subsequent runs") << false;
}

void tst_QUrl::isRelative_data()
{
    generateFirstRunData();
}

void tst_QUrl::isRelative()
{
    QFETCH(bool, firstRun);
    if (firstRun) {
        QBENCHMARK {
            QUrl url("pics/avatar.png");
            url.isRelative();
        }
    } else {
        QUrl url("pics/avatar.png");
        QBENCHMARK {
            url.isRelative();
        }
    }
}

void tst_QUrl::toLocalFile_data()
{
    generateFirstRunData();
}

void tst_QUrl::toLocalFile()
{
    QFETCH(bool, firstRun);
    if (firstRun) {
        QBENCHMARK {
            QUrl url("/tmp/avatar.png");
            url.toLocalFile();
        }
    } else {
        QUrl url("/tmp/avatar.png");
        QBENCHMARK {
            url.toLocalFile();
        }
    }
}

void tst_QUrl::toString_data()
{
    generateFirstRunData();
}

void tst_QUrl::toString()
{
    QFETCH(bool, firstRun);
    if(firstRun) {
        QBENCHMARK {
            QUrl url("pics/avatar.png");
            url.toString();
        }
    } else {
        QUrl url("pics/avatar.png");
        QBENCHMARK {
            url.toString();
        }
    }
}

void tst_QUrl::resolved_data()
{
   generateFirstRunData();
}

void tst_QUrl::resolved()
{
    QFETCH(bool, firstRun);
    QUrl expect("/home/user/pics/avatar.png"), actual;
    if (firstRun) {
        QBENCHMARK {
            QUrl baseUrl("/home/user/");
            QUrl url("pics/avatar.png");
            actual = baseUrl.resolved(url);
        }
    } else {
        QUrl baseUrl("/home/user/");
        QUrl url("pics/avatar.png");
        QBENCHMARK {
            actual = baseUrl.resolved(url);
        }
    }
    QCOMPARE(actual, expect);
}

void tst_QUrl::equality_data()
{
   generateFirstRunData();
}

void tst_QUrl::equality()
{
    QFETCH(bool, firstRun);
    if(firstRun) {
        QBENCHMARK {
            QUrl url("pics/avatar.png");
            QUrl url2("pics/avatar2.png");
            //url == url2;
        }
    } else {
        QUrl url("pics/avatar.png");
        QUrl url2("pics/avatar2.png");
        QBENCHMARK {
            [[maybe_unused]] auto r = url == url2;
        }
    }
}

void tst_QUrl::qmlPropertyWriteUseCase()
{
    QUrl base("file:///home/user/qt/examples/declarative/samegame/SamegameCore/");
    QString str("pics/redStar.png");

    QBENCHMARK {
        QUrl u = QUrl(str);
        if (!u.isEmpty() && u.isRelative())
            u = base.resolved(u);
    }
}

QTEST_MAIN(tst_QUrl)

#include "tst_bench_qurl.moc"
