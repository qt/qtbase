// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDebug>
#include <QTextBrowser>
#include <qtest.h>

class tst_QTextBrowser : public QObject
{
    Q_OBJECT
private slots:
    void largeDocumentsLazyLayout();
};

void tst_QTextBrowser::largeDocumentsLazyLayout()
{
    QString sl;
    for (int i = 0; i < 150000; ++i) {
        sl.append("long long text\n");
    }

    QBENCHMARK {
        QTextBrowser browser;
        browser.setPlainText(sl);
        browser.show();
    }
}

QTEST_MAIN(tst_QTextBrowser)

#include "main.moc"
