// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define Q_ASSERT(cond) ((cond) ? static_cast<void>(0) : qCritical(#cond))

#include <QTest>

#include <QtGui/qpixmap.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterstateguard.h>

class tst_QPainterStateGuard : public QObject
{
    Q_OBJECT

private slots:
    void basics_data();
    void basics();
};

void tst_QPainterStateGuard::basics_data()
{
    QTest::addColumn<QPainterStateGuard::InitialState>("initialState");
    QTest::addColumn<bool>("expectWarning");

    QTest::addRow("Save") << QPainterStateGuard::InitialState::Save << false;
    QTest::addRow("NoSave") << QPainterStateGuard::InitialState::NoSave << true;
}

void tst_QPainterStateGuard::basics()
{
    QFETCH(const QPainterStateGuard::InitialState, initialState);
    QFETCH(const bool, expectWarning);

    if (expectWarning) {
        QTest::ignoreMessage(QtCriticalMsg, "m_level > 0");
        QTest::ignoreMessage(QtWarningMsg, "QPainter::restore: Unbalanced save/restore");
    } else {
        QTest::failOnWarning("QPainter::restore: Unbalanced save/restore");
    }
    QPixmap pixmap(100, 100);
    QPainter painter(&pixmap);
    const QPen defaultPen = painter.pen();

    {
        auto makeGuard = [initialState](QPainter *p) -> QPainterStateGuard {
            return QPainterStateGuard(p, initialState);
        };
        QPainterStateGuard guard = makeGuard(&painter);

        painter.setPen(Qt::red);
        QCOMPARE(painter.pen().color(), Qt::red);

        QPainterStateGuard guard2 = std::move(guard);

        QCOMPARE(painter.pen().color(), Qt::red);

        guard2.restore();
    }

    if (expectWarning)
        QCOMPARE_NE(painter.pen(), defaultPen);
    else
        QCOMPARE(painter.pen(), defaultPen);
}

QTEST_MAIN(tst_QPainterStateGuard)

#include "tst_qpainterstateguard.moc"
