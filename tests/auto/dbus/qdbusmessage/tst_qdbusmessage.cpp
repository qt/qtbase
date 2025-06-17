// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QDBusMessage>

using namespace Qt::StringLiterals;

// incomplete, but enough for the test
#define COMPARE_MESSAGES(lhs, rhs) \
    do { \
        QCOMPARE_EQ(lhs.service(), rhs.service()); \
        QCOMPARE_EQ(lhs.path(), rhs.path()); \
        QCOMPARE_EQ(lhs.interface(), rhs.interface()); \
        QCOMPARE_EQ(lhs.member(), rhs.member()); \
        QCOMPARE_EQ(lhs.type(), rhs.type()); \
        QCOMPARE_EQ(lhs.signature(), rhs.signature()); \
        QCOMPARE_EQ(lhs.arguments(), rhs.arguments()); \
    } while (false)

class tst_QDBusMessage : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void moveAssign();
};

void tst_QDBusMessage::moveAssign()
{
    QDBusMessage referenceMessage =
            QDBusMessage::createMethodCall("org.kde.selftest"_L1, "/org/kde/selftest"_L1,
                                           "org.kde.selftest"_L1, "Ping"_L1);
    referenceMessage << "ping"_L1;

    {
        QDBusMessage copy = referenceMessage;

        // move-assign into another message
        QDBusMessage other;
        other = std::move(copy);
        COMPARE_MESSAGES(other, referenceMessage);

        // modify
        other << "other_arg"_L1; // also affects referenceMessage

        // and move-assign back
        copy = std::move(other);
        COMPARE_MESSAGES(copy, referenceMessage);
    }
}

#undef COMPARE_MESSAGES

QTEST_MAIN(tst_QDBusMessage)

#include "tst_qdbusmessage.moc"
