// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QSignalSpy>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QString>

Q_DECLARE_METATYPE(Qt::Key)
Q_DECLARE_METATYPE(Qt::KeyboardModifiers)

class tst_QKeySequenceEdit : public QObject
{
    Q_OBJECT

private slots:
    void testSetters();
    void testKeys_data();
    void testKeys();
    void testLineEditContents();
    void testMaximumSequenceLength();
    void testFinishingKeyCombinations_data();
    void testFinishingKeyCombinations();
};

void tst_QKeySequenceEdit::testSetters()
{
    QKeySequenceEdit edit;
    QSignalSpy spy(&edit, SIGNAL(keySequenceChanged(QKeySequence)));
    QCOMPARE(edit.keySequence(), QKeySequence());

    edit.setKeySequence(QKeySequence::New);
    QCOMPARE(edit.keySequence(), QKeySequence(QKeySequence::New));

    edit.clear();
    QCOMPARE(edit.keySequence(), QKeySequence());

    QCOMPARE(spy.size(), 2);
}

void tst_QKeySequenceEdit::testKeys_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QKeySequence>("keySequence");

    QTest::newRow("1") << Qt::Key_N << Qt::KeyboardModifiers(Qt::ControlModifier) << QKeySequence("Ctrl+N");
    QTest::newRow("2") << Qt::Key_N << Qt::KeyboardModifiers(Qt::AltModifier) << QKeySequence("Alt+N");
    QTest::newRow("3") << Qt::Key_N << Qt::KeyboardModifiers(Qt::ShiftModifier) << QKeySequence("Shift+N");
    QTest::newRow("4") << Qt::Key_N << Qt::KeyboardModifiers(Qt::ControlModifier  | Qt::ShiftModifier) << QKeySequence("Ctrl+Shift+N");
}

void tst_QKeySequenceEdit::testMaximumSequenceLength()
{
    //
    // GIVEN:
    //  - A QKeySequenceEdit with maxKeyCount == 1
    //  - A QKeySequence with more than one key
    //
    QKeySequenceEdit edit;
    edit.setMaximumSequenceLength(1);
    QCOMPARE(edit.maximumSequenceLength(), 1);

    QKeySequence multi("Ctrl+X, S");
    QCOMPARE(multi.count(), 2);

    //
    // WHEN: setting the key sequence on the edit
    //
    QTest::ignoreMessage(QtWarningMsg,
                         "QKeySequenceEdit: setting a key sequence of length 2 when "
                         "maximumSequenceLength is 1, truncating.");
    edit.setKeySequence(multi);

    //
    // THEN:
    //  - the maxKeyCount property doesn't change
    //  - the key sequence is truncated to maxKeyCount
    //  - and won't un-truncate by increasing maxKeyCount
    //
    QCOMPARE(edit.maximumSequenceLength(), 1);
    const auto edited = edit.keySequence();
    QCOMPARE(edited.count(), 1);
    QCOMPARE(edited, QKeySequence("Ctrl+X"));
    edit.setMaximumSequenceLength(2);
    QCOMPARE(edit.keySequence(), edited);
}

void tst_QKeySequenceEdit::testKeys()
{
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QKeySequence, keySequence);
    QKeySequenceEdit edit;

    QSignalSpy spy(&edit, SIGNAL(editingFinished()));
    QTest::keyPress(&edit, key, modifiers);
    QTest::keyRelease(&edit, key, modifiers);

    QCOMPARE(spy.size(), 0);
    QCOMPARE(edit.keySequence(), keySequence);
    QTRY_COMPARE(spy.size(), 1);
}

void tst_QKeySequenceEdit::testLineEditContents()
{
    QKeySequenceEdit edit;
    QLineEdit *le = edit.findChild<QLineEdit*>();
    QVERIFY(le);

    QCOMPARE(le->text(), QString());

    edit.setKeySequence(QKeySequence::New);
    QCOMPARE(edit.keySequence(), QKeySequence(QKeySequence::New));

    edit.clear();
    QCOMPARE(le->text(), QString());

    edit.setKeySequence(QKeySequence::New);
    QVERIFY(le->text() != QString());

    edit.setKeySequence(QKeySequence());
    QCOMPARE(le->text(), QString());
}

void tst_QKeySequenceEdit::testFinishingKeyCombinations_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QKeySequence>("keySequence");

    QTest::newRow("1") << Qt::Key_Backtab << Qt::KeyboardModifiers(Qt::NoModifier) << QKeySequence("Backtab");
    QTest::newRow("2") << Qt::Key_Tab << Qt::KeyboardModifiers(Qt::NoModifier) << QKeySequence("Tab");
    QTest::newRow("3") << Qt::Key_Return << Qt::KeyboardModifiers(Qt::NoModifier) << QKeySequence("Return");
    QTest::newRow("4") << Qt::Key_Enter << Qt::KeyboardModifiers(Qt::NoModifier) << QKeySequence("Enter");
    QTest::newRow("5") << Qt::Key_Enter << Qt::KeyboardModifiers(Qt::ShiftModifier) << QKeySequence("Shift+Enter");
}

void tst_QKeySequenceEdit::testFinishingKeyCombinations()
{
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QKeySequence, keySequence);
    QKeySequenceEdit edit;

    QSignalSpy spy(&edit, SIGNAL(editingFinished()));
    QCOMPARE(spy.size(), 0);

    edit.setFinishingKeyCombinations({QKeyCombination(modifiers, key)});
    QTest::keyPress(&edit, key, modifiers);
    QTest::keyRelease(&edit, key, modifiers);

    QCOMPARE(edit.keySequence(), QKeySequence());
    QTRY_COMPARE(spy.size(), 1);

    edit.setFinishingKeyCombinations({});
    QTest::keyPress(&edit, key, modifiers);
    QTest::keyRelease(&edit, key, modifiers);

    QCOMPARE(edit.keySequence(), keySequence);
    QTRY_COMPARE(spy.size(), 2);
}

QTEST_MAIN(tst_QKeySequenceEdit)
#include "tst_qkeysequenceedit.moc"
