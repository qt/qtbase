// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#include <QColor>

#include <vector>

static std::vector<QColor> all_rgb_colors()
{
    std::vector<QColor> colors;
    colors.reserve(256 * 256 * 256);
    for (int r = 0; r < 256; ++r) {
        for (int g = 0; g < 256; ++g) {
            for (int b = 0; b < 256; ++b)
                colors.emplace_back(r, g, b);
        }
    }
    return colors;
}


class tst_QColor : public QObject
{
    Q_OBJECT

private slots:
    void nameRgb();
    void nameArgb();
    void toHsl();
    void toHsv();

private:
    const std::vector<QColor> m_all_rgb = all_rgb_colors();
};

void tst_QColor::nameRgb()
{
    QColor color(128, 255, 10);
    QCOMPARE(color.name(), QStringLiteral("#80ff0a"));
    QBENCHMARK {
        color.name();
    }
}

void tst_QColor::nameArgb()
{
    QColor color(128, 255, 0, 102);
    QCOMPARE(color.name(QColor::HexArgb), QStringLiteral("#6680ff00"));
    QBENCHMARK {
        color.name(QColor::HexArgb);
    }
}

void tst_QColor::toHsl()
{
    QBENCHMARK {
        for (const QColor &c : m_all_rgb)
            [[maybe_unused]] const auto r = c.toHsl();
    }
}

void tst_QColor::toHsv()
{
    QBENCHMARK {
        for (const QColor &c : m_all_rgb)
            [[maybe_unused]] const auto r = c.toHsv();
    }
}

QTEST_MAIN(tst_QColor)

#include "tst_qcolor.moc"
