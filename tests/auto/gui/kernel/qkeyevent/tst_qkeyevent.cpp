/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include <QtCore/qcoreapplication.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>

class Window : public QWindow
{
public:
    ~Window() { reset(); }

    void keyPressEvent(QKeyEvent *event) { recordEvent(event); }
    void keyReleaseEvent(QKeyEvent *event) { recordEvent(event); }

    void reset() {
        qDeleteAll(keyEvents.begin(), keyEvents.end());
        keyEvents.clear();
    }
private:
    void recordEvent(QKeyEvent *event) {
        keyEvents.append(new QKeyEvent(event->type(), event->key(), event->modifiers(), event->nativeScanCode(),
            event->nativeVirtualKey(), event->nativeModifiers(), event->text(),
            event->isAutoRepeat(), event->count()));
    }

public:
    QVector<QKeyEvent*> keyEvents;
};

class tst_QKeyEvent : public QObject
{
    Q_OBJECT
public:
    tst_QKeyEvent();
    ~tst_QKeyEvent();

private slots:
    void basicEventDelivery();
    void modifiers_data();
    void modifiers();
};

tst_QKeyEvent::tst_QKeyEvent()
{
}

tst_QKeyEvent::~tst_QKeyEvent()
{
}

void tst_QKeyEvent::basicEventDelivery()
{
    Window window;
    window.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const Qt::Key key = Qt::Key_A;
    const Qt::KeyboardModifier modifiers = Qt::NoModifier;

    QTest::keyClick(&window, key, modifiers);

    QCOMPARE(window.keyEvents.size(), 2);
    QCOMPARE(window.keyEvents.first()->type(), QKeyEvent::KeyPress);
    QCOMPARE(window.keyEvents.last()->type(), QKeyEvent::KeyRelease);
    foreach (const QKeyEvent *event, window.keyEvents) {
        QCOMPARE(Qt::Key(event->key()), key);
        QCOMPARE(Qt::KeyboardModifiers(event->modifiers()), modifiers);
    }
}

static bool orderByModifier(const QVector<int> &v1, const QVector<int> &v2)
{
    if (v1.size() != v2.size())
        return v1.size() < v2.size();

    for (int i = 0; i < qMin(v1.size(), v2.size()); ++i) {
        if (v1.at(i) == v2.at(i))
            continue;

        return v1.at(i) < v2.at(i);
    }

    return true;
}

static QByteArray modifiersTestRowName(const QString &keySequence)
{
    QByteArray result;
    QTextStream str(&result);
    for (int i = 0, size = keySequence.size(); i < size; ++i) {
        const QChar &c = keySequence.at(i);
        const ushort uc = c.unicode();
        if (uc > 32 && uc < 128)
            str << '"' << c << '"';
        else
            str << "U+" << hex << uc << dec;
        if (i < size - 1)
            str << ',';
    }
    return result;
}

void tst_QKeyEvent::modifiers_data()
{
    struct Modifier
    {
        Qt::Key key;
        Qt::KeyboardModifier modifier;
    };
    static const Modifier modifiers[] = {
        { Qt::Key_Shift, Qt::ShiftModifier },
        { Qt::Key_Control, Qt::ControlModifier },
        { Qt::Key_Alt, Qt::AltModifier },
        { Qt::Key_Meta, Qt::MetaModifier },
    };

    QVector<QVector<int>> modifierCombinations;

    // Generate powerset (minus the empty set) of possible modifier combinations
    static const int kNumModifiers = sizeof(modifiers) / sizeof(Modifier);
    for (quint64 bitmask = 1; bitmask < (1 << kNumModifiers) ; ++bitmask) {
        QVector<int> modifierCombination;
        for (quint64 modifier = 0; modifier < kNumModifiers; ++modifier) {
            if (bitmask & (quint64(1) << modifier))
                modifierCombination.append(modifier);
        }
        modifierCombinations.append(modifierCombination);
    }

    qSort(modifierCombinations.begin(), modifierCombinations.end(), orderByModifier);

    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    foreach (const QVector<int> combination, modifierCombinations) {
        int keys[4] = {};
        Qt::KeyboardModifiers mods;
        for (int i = 0; i < combination.size(); ++i) {
            Modifier modifier = modifiers[combination.at(i)];
            keys[i] = modifier.key;
            mods |= modifier.modifier;
        }
        QKeySequence keySequence(keys[0], keys[1], keys[2], keys[3]);
        QTest::newRow(modifiersTestRowName(keySequence.toString(QKeySequence::NativeText)).constData())
            << mods;
    }
}

void tst_QKeyEvent::modifiers()
{
    Window window;
    window.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const Qt::Key key = Qt::Key_A;
    QFETCH(Qt::KeyboardModifiers, modifiers);

    QTest::keyClick(&window, key, modifiers);

    int numKeys = qPopulationCount(quint64(modifiers)) + 1;
    QCOMPARE(window.keyEvents.size(), numKeys * 2);

    for (int i = 0; i < window.keyEvents.size(); ++i) {
        const QKeyEvent *event = window.keyEvents.at(i);
        QCOMPARE(event->type(), i < numKeys ? QKeyEvent::KeyPress : QKeyEvent::KeyRelease);
        if (i == numKeys - 1 || i == numKeys) {
            QCOMPARE(Qt::Key(event->key()), key);
            QCOMPARE(event->modifiers(), modifiers);
        } else {
            QVERIFY(Qt::Key(event->key()) != key);
        }
    }
}

QTEST_MAIN(tst_QKeyEvent)
#include "tst_qkeyevent.moc"
