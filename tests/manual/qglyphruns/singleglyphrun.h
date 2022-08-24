// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SINGLEGLYPHRUN_H
#define SINGLEGLYPHRUN_H

#include <QWidget>
#include <QGlyphRun>

namespace Ui {
class SingleGlyphRun;
}

class SingleGlyphRun : public QWidget
{
    Q_OBJECT

public:
    explicit SingleGlyphRun(QWidget *parent = nullptr);
    ~SingleGlyphRun();

    void updateGlyphRun(const QGlyphRun &glyphRun);
    QRegion bounds() const { return m_bounds; }

private:
    Ui::SingleGlyphRun *ui;
    QRegion m_bounds;
};

#endif // SINGLEGLYPHRUN_H
