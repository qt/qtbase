// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


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

    QCOMPARE(spy.count(), 2);
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

void tst_QKeySequenceEdit::testKeys()
{
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QKeySequence, keySequence);
    QKeySequenceEdit edit;

    QSignalSpy spy(&edit, SIGNAL(editingFinished()));
    QTest::keyPress(&edit, key, modifiers);
    QTest::keyRelease(&edit, key, modifiers);

    QCOMPARE(spy.count(), 0);
    QCOMPARE(edit.keySequence(), keySequence);
    QTRY_COMPARE(spy.count(), 1);
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

QTEST_MAIN(tst_QKeySequenceEdit)
#include "tst_qkeysequenceedit.moc"
