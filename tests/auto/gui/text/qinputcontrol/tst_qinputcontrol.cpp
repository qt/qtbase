// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <private/qinputcontrol_p.h>
#include <QtGui/QKeyEvent>

class tst_QInputControl: public QObject
{
    Q_OBJECT
private slots:
    void isAcceptableInput_data();
    void isAcceptableInput();
    void tabOnlyAcceptableInputForTextEdit();
};

void tst_QInputControl::isAcceptableInput_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<bool>("acceptable");

    QTest::newRow("empty-string") << QString() << Qt::KeyboardModifiers() << false;
    QTest::newRow("zwnj") << QString(QChar(0x200C)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("zwnj-with-ctrl") << QString(QChar(0x200C)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("zwnj-with-ctrl-shift") << QString(QChar(0x200C)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("zwj") << QString(QChar(0x200D)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("zwj-with-ctrl") << QString(QChar(0x200D)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("zwj-with-ctrl-shift") << QString(QChar(0x200D)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("printable-latin") << QString(QLatin1Char('a')) << Qt::KeyboardModifiers() << true;
    QTest::newRow("printable-latin-with-ctrl") << QString(QLatin1Char('a')) << Qt::KeyboardModifiers(Qt::ControlModifier) << false;
    QTest::newRow("printable-latin-with-ctrl-shift") << QString(QLatin1Char('a')) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << false;
    QTest::newRow("printable-hebrew") << QString(QChar(0x2135)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("private-use-area") << QString(QChar(0xE832)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("good-surrogate-0") << QString::fromUtf16(u"\U0001F44D") << Qt::KeyboardModifiers() << true;
    {
        const QChar data[] = { QChar(0xD800), QChar(0xDC00) };
        const QString str = QString(data, 2);
        QTest::newRow("good-surrogate-1") << str << Qt::KeyboardModifiers() << true;
    }
    {
        const QChar data[] = { QChar(0xD800), QChar(0xDFFF) };
        const QString str = QString(data, 2);
        QTest::newRow("good-surrogate-2") << str << Qt::KeyboardModifiers() << true;
    }
    {
        const QChar data[] = { QChar(0xDBFF), QChar(0xDC00) };
        const QString str = QString(data, 2);
        QTest::newRow("good-surrogate-3") << str << Qt::KeyboardModifiers() << true;
    }
    {
        const QChar data[] = { QChar(0xDBFF), QChar(0xDFFF) };
        const QString str = QString(data, 2);
        QTest::newRow("good-surrogate-4") << str << Qt::KeyboardModifiers() << true;
    }
    {
        const QChar data[] = { QChar(0xD7FF), QChar(0xDC00) };
        const QString str = QString(data, 2);
        QTest::newRow("bad-surrogate-1") << str << Qt::KeyboardModifiers() << false;
    }
    {
        const QChar data[] = { QChar(0xD7FF), QChar(0xDFFF) };
        const QString str = QString(data, 2);
        QTest::newRow("bad-surrogate-2") << str << Qt::KeyboardModifiers() << false;
    }
    {
        const QChar data[] = { QChar(0xDC00), QChar(0xDC00) };
        const QString str = QString(data, 2);
        QTest::newRow("bad-surrogate-3") << str << Qt::KeyboardModifiers() << false;
    }
    {
        const QChar data[] = { QChar(0xD800), QChar(0xE000) };
        const QString str = QString(data, 2);
        QTest::newRow("bad-surrogate-4") << str << Qt::KeyboardModifiers() << false;
    }
    {
        const QChar data[] = { QChar(0xD800) };
        const QString str = QString(data, 1);
        QTest::newRow("bad-surrogate-5") << str << Qt::KeyboardModifiers() << false;
    }
    QTest::newRow("multiple-printable") << QStringLiteral("foobar") << Qt::KeyboardModifiers() << true;
    QTest::newRow("rlm") << QString(QChar(0x200F)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("rlm-with-ctrl") << QString(QChar(0x200F)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("rlm-with-ctrl-shift") << QString(QChar(0x200F)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("lrm") << QString(QChar(0x200E)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("lrm-with-ctrl") << QString(QChar(0x200E)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("lrm-with-ctrl-shift") << QString(QChar(0x200E)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("rlo") << QString(QChar(0x202E)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("rlo-with-ctrl") << QString(QChar(0x202E)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("rlo-with-ctrl-shift") << QString(QChar(0x202E)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("lro") << QString(QChar(0x202D)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("lro-with-ctrl") << QString(QChar(0x202D)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("lro-with-ctrl-shift") << QString(QChar(0x202D)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("lre") << QString(QChar(0x202B)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("lre-with-ctrl") << QString(QChar(0x202B)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("lre-with-ctrl-shift") << QString(QChar(0x202B)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("rle") << QString(QChar(0x202A)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("rle-with-ctrl") << QString(QChar(0x202A)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("rle-with-ctrl-shift") << QString(QChar(0x202A)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;
    QTest::newRow("pdf") << QString(QChar(0x202C)) << Qt::KeyboardModifiers() << true;
    QTest::newRow("pdf-with-ctrl") << QString(QChar(0x202C)) << Qt::KeyboardModifiers(Qt::ControlModifier) << true;
    QTest::newRow("pdf-with-ctrl-shift") << QString(QChar(0x202C)) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier) << true;

}

void tst_QInputControl::isAcceptableInput()
{
    QFETCH(QString, text);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(bool, acceptable);

    QKeyEvent keyEvent(QKeyEvent::KeyPress, Qt::Key_unknown, modifiers, text);

    {
        QInputControl inputControl(QInputControl::TextEdit);
        QCOMPARE(inputControl.isAcceptableInput(&keyEvent), acceptable);
    }

    {
        QInputControl inputControl(QInputControl::LineEdit);
        QCOMPARE(inputControl.isAcceptableInput(&keyEvent), acceptable);
    }
}

void tst_QInputControl::tabOnlyAcceptableInputForTextEdit()
{
    QKeyEvent keyEvent(QKeyEvent::KeyPress, Qt::Key_unknown, Qt::KeyboardModifiers(), QLatin1String("\t"));

    {
        QInputControl inputControl(QInputControl::TextEdit);
        QCOMPARE(inputControl.isAcceptableInput(&keyEvent), true);
    }

    {
        QInputControl inputControl(QInputControl::LineEdit);
        QCOMPARE(inputControl.isAcceptableInput(&keyEvent), false);
    }
}

QTEST_MAIN(tst_QInputControl)
#include "tst_qinputcontrol.moc"

