// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QFont>
#include <QFontMetrics>

namespace src_gui_text_qfontmetrics {

void wrapper0() {
//! [0]
QFont font("times", 24);
QFontMetrics fm(font);
int pixelsWide = fm.horizontalAdvance("What's the advance width of this text?");
int pixelsHigh = fm.height();
//! [0]

Q_UNUSED(pixelsWide);
Q_UNUSED(pixelsHigh);
} // wrapper0


void wrapper1() {
//! [1]
QFont font("times", 24);
QFontMetricsF fm(font);
qreal pixelsWide = fm.horizontalAdvance("What's the advance width of this text?");
qreal pixelsHigh = fm.height();
//! [1]

Q_UNUSED(pixelsWide);
Q_UNUSED(pixelsHigh);
} // wrapper1

} //src_gui_text_qfontmetrics
