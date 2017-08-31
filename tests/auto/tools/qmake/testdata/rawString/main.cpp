/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// macro names that *aren't* string-literal-prefixes:
#define Ru8 "rue-it"
#define RL "real life"
#define Ru "are you ?"
#define RU "Are You ?"
#define LLR "double-hockey-sticks"
#define LUR "Tricky"
#define LuR "tricky"
#define Lu8R "l'uber"
#define UUR "Double-Yew"
#define ULR "Eweler"
#define UuR "You ... you-are"
#define Uu8R "You ... you *ate* our ..."
#define uuR "water"
#define uLR "eweler"
#define uUR "double-Your"
#define uu8R "totally uber"
#define u8u8R "rubber-you"
#define u8LR "Uber left-to-right"
#define u8UR "Uber Upper-Right"
#define u8uR "Uber upper-right"
#define Ru8R "bouncy"
#define RLR "Marching"
#define RuR "Rossum's general-purpose workers"
#define RUR "Rossum's Universal Robots"

static const char monstrosity[] =
    Ru8"Ru8("
    RL"RL("
    Ru"Ru("
    RU"RU("
    LLR"LLR("
    LUR"LUR("
    LuR"LuR("
    Lu8R"Lu8R("
    UUR"UUR("
    ULR"ULR("
    UuR"UuR("
    Uu8R"Uu8R("
    uuR"uuR("
    uLR"uLR("
    uUR"uUR("
    uu8R"uu8R("
    u8u8R"u8u8R("
    u8LR"u8LR("
    u8UR"u8UR("
    u8uR"u8uR("
    Ru8R"Ru8R("
    RLR"RLR("
    RuR"RuR("
    RUR"RUR("
    "Finally, some content";

#include <moc_object2.cpp>

static const char closure[] =
    ")RUR"
    ")RuR"
    ")RLR"
    ")Ru8R"
    ")u8uR"
    ")u8UR"
    ")u8LR"
    ")u8u8R"
    ")uu8R"
    ")uUR"
    ")uLR"
    ")uuR"
    ")Uu8R"
    ")UuR"
    ")ULR"
    ")UUR"
    ")Lu8R"
    ")LuR"
    ")LUR"
    ")LLR"
    ")RU"
    ")Ru"
    ")RL"
    ")Ru8";
// If moc got confused, the confusion should now be over

// Real raw strings, not actually leaving us inside any comments:
static const char raw[] = R"blah(lorem " ipsum /*)blah"\
;
static const wchar_t wider[] = LR"blah(lorem " ipsum /*)blah"\
;
static const char32_t UCS4[] = UR"blah(lorem " ipsum /*)blah"\
;
static const char16_t UCS2[] = uR"blah(lorem " ipsum /*)blah"\
;
static const char utf8[] = u8R"blah(lorem " ipsum /*)blah"\
;
#include <moc_object1.cpp>

/* Avoid unused variable warnings by silly uses of arrays: */
#define final(x) x[sizeof(x) - 1] // 0, of course
int main () {
    return final(raw)
    * (final(wider) - final(UCS4))
    * (final(UCS2) - final(utf8))
    * (final(monstrosity) - final(closure));
}
#undef final
