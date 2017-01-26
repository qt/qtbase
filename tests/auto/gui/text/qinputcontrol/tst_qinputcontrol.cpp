/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

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

