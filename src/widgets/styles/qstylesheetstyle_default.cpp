// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/* This is the default Qt style sheet.

   IMPORTANT: This style sheet is primarily meant for defining feature
   capabilities of styles. Do NOT add default styling rules here. When in
   doubt ask the stylesheet maintainer.

   The stylesheet in here used to be in a CSS file, but was moved here to
   avoid parsing overhead.
*/

#include "qstylesheetstyle_p.h"
#if QT_CONFIG(cssparser)
#include "private/qcssparser_p.h"
#endif

#ifndef QT_NO_STYLE_STYLESHEET

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QCss;

// This is the class name of the selector.
// Use an empty string where you would use '*' in CSS.
// Ex. QHeaderView

#define SET_ELEMENT_NAME(x) \
    bSelector.elementName = (x)

// This acts as both pseudo state and sub control. The first parameter is the
// string name, and the second is the PseudoClass_* constant.
// The sub control specifier is always the first, and has the type
// PseudoClass_Unknown.
// If there is no PseudoClass_Unknown as the first pseudo, it is assumed to be
// a pseudo state.
// Ex. QComboBox::drop-down:enabled
//                   ^         ^

#define ADD_PSEUDO(x, y) \
    pseudo.type = (y); \
    pseudo.name = (x); \
    bSelector.pseudos << pseudo

// This is attributes. The third parameter is AttributeSelector::*
// Ex. QComboBox[style="QWindowsVistaStyle"]
//                 ^           ^

#define ADD_ATTRIBUTE_SELECTOR(x, y, z) \
    attr.name = (x); \
    attr.value = (y); \
    attr.valueMatchCriterium = (z); \
    bSelector.attributeSelectors << attr

// Adds the current basic selector to the rule.
// Several basic selectors behave as AND (space in CSS).

#define ADD_BASIC_SELECTOR \
    selector.basicSelectors << bSelector; \
    bSelector.ids.clear(); \
    bSelector.pseudos.clear(); \
    bSelector.attributeSelectors.clear()

// Adds the current selector to the rule.
// Several selectors behave as OR (comma in CSS).

#define ADD_SELECTOR \
    styleRule.selectors << selector; \
    selector.basicSelectors.clear()

// Sets the name of a property.
// Ex. background: red;
//         ^

#define SET_PROPERTY(x, y) \
    decl.d->property = (x); \
    decl.d->propertyId = (y)

// Adds a value to the current property.
// The first parameter should be Value::KnownIdentifier if the value can be
// found among the Value_* constants, in which case the second should be that
// constant. Otherwise the first parameter is Value::Identifier and the second
// is a string.
// Adding more values is the same as separating by spaces in CSS.
// Ex. border: 2px solid black;
//              ^    ^     ^

#define ADD_VALUE(x, y) \
    value.type = (x); \
    value.variant = (y); \
    decl.d->values << value

// Adds the current declaration to the rule.
// Ex. border: 2px solid black;
//     \----------------------/

#define ADD_DECLARATION \
    styleRule.declarations << decl; \
    decl.d.detach(); \
    decl.d->values.clear()

// Adds the rule to the stylesheet.
// Use at the end of every CSS block.

#define ADD_STYLE_RULE \
    sheet.styleRules << styleRule; \
    styleRule.selectors.clear(); \
    styleRule.declarations.clear()

StyleSheet QStyleSheetStyle::getDefaultStyleSheet() const
{
    StyleSheet sheet;
    StyleRule styleRule;
    BasicSelector bSelector;
    Selector selector;
    Declaration decl;
    QCss::Value value;
    Pseudo pseudo;
    AttributeSelector attr;

    // pixmap based style doesn't support any features
    bool styleIsPixmapBased = baseStyle()->inherits("QMacStyle")
            || (baseStyle()->inherits("QWindowsVistaStyle")
                && !baseStyle()->inherits("QWindows11Style"));


    /*QLineEdit {
        -qt-background-role: base;
        border: native;
        -qt-style-features: background-color;
    }*/
    {
        SET_ELEMENT_NAME("QLineEdit"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        SET_PROPERTY("-qt-style-features"_L1, QtStyleFeatures);
        ADD_VALUE(QCss::Value::Identifier, QString::fromLatin1("background-color"));
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QLineEdit:no-frame {
        border: none;
    }*/
    {
        SET_ELEMENT_NAME("QLineEdit"_L1);
        ADD_PSEUDO("no-frame"_L1, PseudoClass_Frameless);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_None);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QFrame {
        border: native;
    }*/
    {
        SET_ELEMENT_NAME("QFrame"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QLabel, QToolBox {
        background: none;
        border-image: none;
    }*/
    {
        SET_ELEMENT_NAME("QLabel"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_ELEMENT_NAME("QToolBox"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("background"_L1, Background);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_None);
        ADD_DECLARATION;

        SET_PROPERTY("border-image"_L1, BorderImage);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_None);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QGroupBox {
        border: native;
    }*/
    {
        SET_ELEMENT_NAME("QGroupBox"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }


    /*QToolTip {
        -qt-background-role: window;
        border: native;
    }*/
    {
        SET_ELEMENT_NAME("QToolTip"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QPushButton, QToolButton {
        border-style: native;
        -qt-style-features: background-color;  //only for not pixmap based styles
    }*/
    {
        SET_ELEMENT_NAME("QPushButton"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_ELEMENT_NAME("QToolButton"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("border-style"_L1, BorderStyles);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        if (!styleIsPixmapBased) {
            SET_PROPERTY("-qt-style-features"_L1, QtStyleFeatures);
            ADD_VALUE(QCss::Value::Identifier, QString::fromLatin1("background-color"));
            ADD_DECLARATION;
        }


        ADD_STYLE_RULE;
    }


    /*QComboBox {
        border: native;
        -qt-style-features: background-color background-gradient;   //only for not pixmap based styles
        -qt-background-role: base;
    }*/

    {
        SET_ELEMENT_NAME("QComboBox"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        if (!styleIsPixmapBased) {
            SET_PROPERTY("-qt-style-features"_L1, QtStyleFeatures);
            ADD_VALUE(QCss::Value::Identifier, QString::fromLatin1("background-color"));
            ADD_VALUE(QCss::Value::Identifier, QString::fromLatin1("background-gradient"));
            ADD_DECLARATION;
        }

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QComboBox[style="QPlastiqueStyle"][readOnly="true"],
    QComboBox[style="QFusionStyle"][readOnly="true"],
    QComboBox[style="QCleanlooksStyle"][readOnly="true"]
    {
        -qt-background-role: button;
    }*/
    if (baseStyle()->inherits("QPlastiqueStyle")  || baseStyle()->inherits("QCleanlooksStyle") || baseStyle()->inherits("QFusionStyle"))
    {
        SET_ELEMENT_NAME("QComboBox"_L1);
        ADD_ATTRIBUTE_SELECTOR("readOnly"_L1, "true"_L1, AttributeSelector::MatchEqual);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Button);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QAbstractSpinBox {
        border: native;
        -qt-style-features: background-color;
        -qt-background-role: base;
    }*/
    {
        SET_ELEMENT_NAME("QAbstractSpinBox"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        SET_PROPERTY("-qt-style-features"_L1, QtStyleFeatures);
        ADD_VALUE(QCss::Value::Identifier, QString::fromLatin1("background-color"));
        ADD_DECLARATION;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QMenu {
        -qt-background-role: window;
    }*/
    {
        SET_ELEMENT_NAME("QMenu"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }
    /*QMenu::item {
        -qt-style-features: background-color;
    }*/
    if (!styleIsPixmapBased) {
        SET_ELEMENT_NAME("QMenu"_L1);
        ADD_PSEUDO("item"_L1, PseudoClass_Unknown);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-style-features"_L1, QtStyleFeatures);
        ADD_VALUE(QCss::Value::Identifier, QString::fromLatin1("background-color"));
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QHeaderView {
        -qt-background-role: window;
    }*/
    {
        SET_ELEMENT_NAME("QHeaderView"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QTableCornerButton::section, QHeaderView::section {
        -qt-background-role: button;
        -qt-style-features: background-color; //if style is not pixmap based
        border: native;
    }*/
    {
        SET_ELEMENT_NAME("QTableCornerButton"_L1);
        ADD_PSEUDO("section"_L1, PseudoClass_Unknown);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_ELEMENT_NAME("QHeaderView"_L1);
        ADD_PSEUDO("section"_L1, PseudoClass_Unknown);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Button);
        ADD_DECLARATION;

        if (!styleIsPixmapBased) {
            SET_PROPERTY("-qt-style-features"_L1, QtStyleFeatures);
            ADD_VALUE(QCss::Value::Identifier, QString::fromLatin1("background-color"));
            ADD_DECLARATION;
        }

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QProgressBar {
        -qt-background-role: base;
    }*/
    {
        SET_ELEMENT_NAME("QProgressBar"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QScrollBar {
        -qt-background-role: window;
    }*/
    {
        SET_ELEMENT_NAME("QScrollBar"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("-qt-background-role"_L1, QtBackgroundRole);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QDockWidget {
        border: native;
    }*/
    {
        SET_ELEMENT_NAME("QDockWidget"_L1);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY("border"_L1, Border);
        ADD_VALUE(QCss::Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    sheet.origin = StyleSheetOrigin_UserAgent;
    sheet.buildIndexes();
    return sheet;
}

#endif // #ifndef QT_NO_STYLE_STYLESHEET

QT_END_NAMESPACE
