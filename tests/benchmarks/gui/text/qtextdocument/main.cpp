// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDebug>
#include <QTextDocument>
#include <qtest.h>

class tst_QTextDocument : public QObject
{
    Q_OBJECT
private slots:
    void mightBeRichText_data();
    void mightBeRichText();
};

void tst_QTextDocument::mightBeRichText_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("isMaybeRichText");
    QTest::newRow("empty") << QString() << false;
    QTest::newRow("simple") << QString::fromLatin1("<html><b>Foo</b></html>") << true;
    QTest::newRow("simple2") << QString::fromLatin1("<b>Foo</b>") << true;
    QTest::newRow("documentation-header") << QString("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
                                                     "<!DOCTYPE html\n"
                                                     "    PUBLIC ""-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">\n"
                                                     "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">")
                                          << true;
    QTest::newRow("br-nospace") << QString("Test <br/> new line") << true;
    QTest::newRow("br-space") << QString("Test <br /> new line") << true;
    QTest::newRow("br-invalidspace") << QString("Test <br/ > new line") << false;
    QTest::newRow("invalid closing tag") << QString("Test <br/ line") << false;
    QTest::newRow("no tags") << QString("Test line") << false;
}

void tst_QTextDocument::mightBeRichText()
{
    QFETCH(QString, source);
    QFETCH(bool, isMaybeRichText);
    QBENCHMARK {
        QCOMPARE(isMaybeRichText, Qt::mightBeRichText(source));
    }
}

QTEST_MAIN(tst_QTextDocument)

#include "main.moc"
