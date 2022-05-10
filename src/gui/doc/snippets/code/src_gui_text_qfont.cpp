// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QFont>
#include <QFontInfo>
#include <QFontMetrics>

namespace src_gui_text_qfont {

void wrapper0() {
//! [0]
QFont serifFont("Times", 10, QFont::Bold);
QFont sansFont("Helvetica [Cronyx]", 12);
//! [0]


//! [1]
QFont f("Helvetica");
//! [1]

} // wrapper0


void wrapper1() {
QFont f1;
//! [2]
QFont f("Helvetica [Cronyx]");
//! [2]


//! [3]
QFontInfo info(f1);
QString family = info.family();
//! [3]


//! [4]
QFontMetrics fm(f1);
int textWidthInPixels = fm.horizontalAdvance("How many pixels wide is this text?");
int textHeightInPixels = fm.height();
//! [4]

Q_UNUSED(textWidthInPixels);
Q_UNUSED(textHeightInPixels);
} // wrapper
} // src_gui_text_qfont
