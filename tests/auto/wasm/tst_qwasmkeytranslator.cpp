// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../../../src/plugins/platforms/wasm/qwasmkeytranslator.h"

#include "../../../src/plugins/platforms/wasm/qwasmevent.h"

#include <QTest>

#include <emscripten/val.h>

namespace {
emscripten::val makeDeadKeyJsEvent(QString code, Qt::KeyboardModifiers modifiers)
{
    auto jsEvent = emscripten::val::object();
    jsEvent.set("code", emscripten::val(code.toStdString()));
    jsEvent.set("key", emscripten::val("Dead"));
    jsEvent.set("shiftKey", emscripten::val(modifiers.testFlag(Qt::ShiftModifier)));
    jsEvent.set("ctrlKey", emscripten::val(false));
    jsEvent.set("altKey", emscripten::val(false));
    jsEvent.set("metaKey", emscripten::val(false));

    return jsEvent;
}

emscripten::val makeKeyJsEvent(QString code, QString key, Qt::KeyboardModifiers modifiers)
{
    auto jsEvent = emscripten::val::object();
    jsEvent.set("code", emscripten::val(code.toStdString()));
    jsEvent.set("key", emscripten::val(key.toStdString()));
    jsEvent.set("shiftKey", emscripten::val(modifiers.testFlag(Qt::ShiftModifier)));
    jsEvent.set("ctrlKey", emscripten::val(modifiers.testFlag(Qt::ControlModifier)));
    jsEvent.set("altKey", emscripten::val(modifiers.testFlag(Qt::AltModifier)));
    jsEvent.set("metaKey", emscripten::val(modifiers.testFlag(Qt::MetaModifier)));

    return jsEvent;
}
} // unnamed namespace

class tst_QWasmKeyTranslator : public QObject
{
    Q_OBJECT

public:
    tst_QWasmKeyTranslator() = default;

private slots:
    void init();

    void modifyByDeadKey_data();
    void modifyByDeadKey();
    void deadKeyModifiesOnlyOneKeyPressAndUp();
    void deadKeyIgnoresKeyUpPrecedingKeyDown();
    void onlyKeysProducingTextAreModifiedByDeadKeys();
};

void tst_QWasmKeyTranslator::init() { }

void tst_QWasmKeyTranslator::modifyByDeadKey_data()
{
    QTest::addColumn<QString>("deadKeyCode");
    QTest::addColumn<Qt::KeyboardModifiers>("deadKeyModifiers");
    QTest::addColumn<QString>("targetKeyCode");
    QTest::addColumn<QString>("targetKey");
    QTest::addColumn<Qt::Key>("targetQtKey");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QString>("expectedModifiedKey");

    QTest::newRow("à (Backquote)") << "Backquote" << Qt::KeyboardModifiers() << "KeyA"
                                   << "a" << Qt::Key_Agrave << Qt::KeyboardModifiers() << "à";
    QTest::newRow("À (Backquote)")
            << "Backquote" << Qt::KeyboardModifiers() << "KeyA"
            << "A" << Qt::Key_Agrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "À";
    QTest::newRow("à (IntlBackslash)") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyA"
                                       << "a" << Qt::Key_Agrave << Qt::KeyboardModifiers() << "à";
    QTest::newRow("À (IntlBackslash)")
            << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyA"
            << "A" << Qt::Key_Agrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "À";
    QTest::newRow("á (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyA"
                               << "a" << Qt::Key_Aacute << Qt::KeyboardModifiers() << "á";
    QTest::newRow("Á (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyA"
                               << "A" << Qt::Key_Aacute << Qt::KeyboardModifiers(Qt::ShiftModifier)
                               << "Á";
    QTest::newRow("á") << "KeyE" << Qt::KeyboardModifiers() << "KeyA"
                       << "a" << Qt::Key_Aacute << Qt::KeyboardModifiers() << "á";
    QTest::newRow("Á") << "KeyE" << Qt::KeyboardModifiers() << "KeyA"
                       << "A" << Qt::Key_Aacute << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Á";
    QTest::newRow("ä (Mac Umlaut)") << "KeyU" << Qt::KeyboardModifiers() << "KeyA"
                                    << "a" << Qt::Key_Adiaeresis << Qt::KeyboardModifiers() << "ä";
    QTest::newRow("Ä (Mac Umlaut)")
            << "KeyU" << Qt::KeyboardModifiers() << "KeyA"
            << "A" << Qt::Key_Adiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ä";
    QTest::newRow("ä (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyA"
            << "a" << Qt::Key_Adiaeresis << Qt::KeyboardModifiers() << "ä";
    QTest::newRow("Ä (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyA"
            << "A" << Qt::Key_Adiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ä";
    QTest::newRow("â") << "KeyI" << Qt::KeyboardModifiers() << "KeyA"
                       << "a" << Qt::Key_Acircumflex << Qt::KeyboardModifiers() << "â";
    QTest::newRow("Â") << "KeyI" << Qt::KeyboardModifiers() << "KeyA"
                       << "A" << Qt::Key_Acircumflex << Qt::KeyboardModifiers(Qt::ShiftModifier)
                       << "Â";
    QTest::newRow("â (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyA"
                               << "a" << Qt::Key_Acircumflex << Qt::KeyboardModifiers() << "â";
    QTest::newRow("Â (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyA"
                               << "A" << Qt::Key_Acircumflex
                               << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Â";
    QTest::newRow("ã") << "KeyN" << Qt::KeyboardModifiers() << "KeyA"
                       << "a" << Qt::Key_Atilde << Qt::KeyboardModifiers() << "ã";
    QTest::newRow("Ã") << "KeyN" << Qt::KeyboardModifiers() << "KeyA"
                       << "A" << Qt::Key_Atilde << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ã";

    QTest::newRow("è (Backquote)") << "Backquote" << Qt::KeyboardModifiers() << "KeyE"
                                   << "e" << Qt::Key_Egrave << Qt::KeyboardModifiers() << "è";
    QTest::newRow("È (Backquote)")
            << "Backquote" << Qt::KeyboardModifiers() << "KeyE"
            << "E" << Qt::Key_Egrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "È";
    QTest::newRow("è") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyE"
                       << "e" << Qt::Key_Egrave << Qt::KeyboardModifiers() << "è";
    QTest::newRow("È") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyE"
                       << "E" << Qt::Key_Egrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "È";
    QTest::newRow("é") << "KeyE" << Qt::KeyboardModifiers() << "KeyE"
                       << "e" << Qt::Key_Eacute << Qt::KeyboardModifiers() << "é";
    QTest::newRow("É") << "KeyE" << Qt::KeyboardModifiers() << "KeyE"
                       << "E" << Qt::Key_Eacute << Qt::KeyboardModifiers(Qt::ShiftModifier) << "É";
    QTest::newRow("é (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyE"
                               << "e" << Qt::Key_Eacute << Qt::KeyboardModifiers() << "é";
    QTest::newRow("É (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyE"
                               << "E" << Qt::Key_Eacute << Qt::KeyboardModifiers(Qt::ShiftModifier)
                               << "É";
    QTest::newRow("ë (Mac Umlaut)") << "KeyU" << Qt::KeyboardModifiers() << "KeyE"
                                    << "e" << Qt::Key_Ediaeresis << Qt::KeyboardModifiers() << "ë";
    QTest::newRow("Ë (Mac Umlaut)")
            << "KeyU" << Qt::KeyboardModifiers() << "KeyE"
            << "E" << Qt::Key_Ediaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ë";
    QTest::newRow("ë (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyE"
            << "e" << Qt::Key_Ediaeresis << Qt::KeyboardModifiers() << "ë";
    QTest::newRow("Ë (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyE"
            << "E" << Qt::Key_Ediaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ë";
    QTest::newRow("ê") << "KeyI" << Qt::KeyboardModifiers() << "KeyE"
                       << "e" << Qt::Key_Ecircumflex << Qt::KeyboardModifiers() << "ê";
    QTest::newRow("Ê") << "KeyI" << Qt::KeyboardModifiers() << "KeyE"
                       << "E" << Qt::Key_Ecircumflex << Qt::KeyboardModifiers(Qt::ShiftModifier)
                       << "Ê";
    QTest::newRow("ê (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyE"
                               << "e" << Qt::Key_Ecircumflex << Qt::KeyboardModifiers() << "ê";
    QTest::newRow("Ê (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyE"
                               << "E" << Qt::Key_Ecircumflex
                               << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ê";

    QTest::newRow("ì (Backquote)") << "Backquote" << Qt::KeyboardModifiers() << "KeyI"
                                   << "i" << Qt::Key_Igrave << Qt::KeyboardModifiers() << "ì";
    QTest::newRow("Ì (Backquote)")
            << "Backquote" << Qt::KeyboardModifiers() << "KeyI"
            << "I" << Qt::Key_Igrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ì";
    QTest::newRow("ì") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyI"
                       << "i" << Qt::Key_Igrave << Qt::KeyboardModifiers() << "ì";
    QTest::newRow("Ì") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyI"
                       << "I" << Qt::Key_Igrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ì";
    QTest::newRow("í") << "KeyE" << Qt::KeyboardModifiers() << "KeyI"
                       << "i" << Qt::Key_Iacute << Qt::KeyboardModifiers() << "í";
    QTest::newRow("Í") << "KeyE" << Qt::KeyboardModifiers() << "KeyI"
                       << "I" << Qt::Key_Iacute << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Í";
    QTest::newRow("í (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyI"
                               << "i" << Qt::Key_Iacute << Qt::KeyboardModifiers() << "í";
    QTest::newRow("Í (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyI"
                               << "I" << Qt::Key_Iacute << Qt::KeyboardModifiers(Qt::ShiftModifier)
                               << "Í";
    QTest::newRow("ï (Mac Umlaut)") << "KeyU" << Qt::KeyboardModifiers() << "KeyI"
                                    << "i" << Qt::Key_Idiaeresis << Qt::KeyboardModifiers() << "ï";
    QTest::newRow("Ï (Mac Umlaut)")
            << "KeyU" << Qt::KeyboardModifiers() << "KeyI"
            << "I" << Qt::Key_Idiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ï";
    QTest::newRow("ï (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyI"
            << "i" << Qt::Key_Idiaeresis << Qt::KeyboardModifiers() << "ï";
    QTest::newRow("Ï (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyI"
            << "I" << Qt::Key_Idiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ï";
    QTest::newRow("î") << "KeyI" << Qt::KeyboardModifiers() << "KeyI"
                       << "i" << Qt::Key_Icircumflex << Qt::KeyboardModifiers() << "î";
    QTest::newRow("Î") << "KeyI" << Qt::KeyboardModifiers() << "KeyI"
                       << "I" << Qt::Key_Icircumflex << Qt::KeyboardModifiers(Qt::ShiftModifier)
                       << "Î";
    QTest::newRow("î (^ key)") << "Digit6" << Qt::KeyboardModifiers() << "KeyI"
                               << "i" << Qt::Key_Icircumflex << Qt::KeyboardModifiers() << "î";
    QTest::newRow("Î (^ key)") << "Digit6" << Qt::KeyboardModifiers() << "KeyI"
                               << "I" << Qt::Key_Icircumflex
                               << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Î";

    QTest::newRow("ñ") << "KeyN" << Qt::KeyboardModifiers() << "KeyN"
                       << "n" << Qt::Key_Ntilde << Qt::KeyboardModifiers() << "ñ";
    QTest::newRow("Ñ") << "KeyN" << Qt::KeyboardModifiers() << "KeyN"
                       << "N" << Qt::Key_Ntilde << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ñ";

    QTest::newRow("ò (Backquote)") << "Backquote" << Qt::KeyboardModifiers() << "KeyO"
                                   << "o" << Qt::Key_Ograve << Qt::KeyboardModifiers() << "ò";
    QTest::newRow("Ò (Backquote)")
            << "Backquote" << Qt::KeyboardModifiers() << "KeyO"
            << "O" << Qt::Key_Ograve << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ò";
    QTest::newRow("ò") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyO"
                       << "o" << Qt::Key_Ograve << Qt::KeyboardModifiers() << "ò";
    QTest::newRow("Ò") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyO"
                       << "O" << Qt::Key_Ograve << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ò";
    QTest::newRow("ó") << "KeyE" << Qt::KeyboardModifiers() << "KeyO"
                       << "o" << Qt::Key_Oacute << Qt::KeyboardModifiers() << "ó";
    QTest::newRow("Ó") << "KeyE" << Qt::KeyboardModifiers() << "KeyO"
                       << "O" << Qt::Key_Oacute << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ó";
    QTest::newRow("ó (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyO"
                               << "o" << Qt::Key_Oacute << Qt::KeyboardModifiers() << "ó";
    QTest::newRow("Ó  (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyO"
                                << "O" << Qt::Key_Oacute << Qt::KeyboardModifiers(Qt::ShiftModifier)
                                << "Ó";
    QTest::newRow("ö (Mac Umlaut)") << "KeyU" << Qt::KeyboardModifiers() << "KeyO"
                                    << "o" << Qt::Key_Odiaeresis << Qt::KeyboardModifiers() << "ö";
    QTest::newRow("Ö (Mac Umlaut)")
            << "KeyU" << Qt::KeyboardModifiers() << "KeyO"
            << "O" << Qt::Key_Odiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ö";
    QTest::newRow("ö (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyO"
            << "o" << Qt::Key_Odiaeresis << Qt::KeyboardModifiers() << "ö";
    QTest::newRow("Ö (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyO"
            << "O" << Qt::Key_Odiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ö";
    QTest::newRow("ô") << "KeyI" << Qt::KeyboardModifiers() << "KeyO"
                       << "o" << Qt::Key_Ocircumflex << Qt::KeyboardModifiers() << "ô";
    QTest::newRow("Ô") << "KeyI" << Qt::KeyboardModifiers() << "KeyO"
                       << "O" << Qt::Key_Ocircumflex << Qt::KeyboardModifiers(Qt::ShiftModifier)
                       << "Ô";
    QTest::newRow("ô (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyO"
                               << "o" << Qt::Key_Ocircumflex << Qt::KeyboardModifiers() << "ô";
    QTest::newRow("Ô (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyO"
                               << "O" << Qt::Key_Ocircumflex
                               << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ô";
    QTest::newRow("õ") << "KeyN" << Qt::KeyboardModifiers() << "KeyO"
                       << "o" << Qt::Key_Otilde << Qt::KeyboardModifiers() << "õ";
    QTest::newRow("Õ") << "KeyN" << Qt::KeyboardModifiers() << "KeyO"
                       << "O" << Qt::Key_Otilde << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Õ";

    QTest::newRow("ù (Backquote)") << "Backquote" << Qt::KeyboardModifiers() << "KeyU"
                                   << "u" << Qt::Key_Ugrave << Qt::KeyboardModifiers() << "ù";
    QTest::newRow("Ù (Backquote)")
            << "Backquote" << Qt::KeyboardModifiers() << "KeyU"
            << "U" << Qt::Key_Ugrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ù";
    QTest::newRow("ù") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyU"
                       << "u" << Qt::Key_Ugrave << Qt::KeyboardModifiers() << "ù";
    QTest::newRow("Ù") << "IntlBackslash" << Qt::KeyboardModifiers() << "KeyU"
                       << "U" << Qt::Key_Ugrave << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ù";
    QTest::newRow("ú") << "KeyE" << Qt::KeyboardModifiers() << "KeyU"
                       << "u" << Qt::Key_Uacute << Qt::KeyboardModifiers() << "ú";
    QTest::newRow("Ú") << "KeyE" << Qt::KeyboardModifiers() << "KeyU"
                       << "U" << Qt::Key_Uacute << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ú";
    QTest::newRow("ú (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyU"
                               << "u" << Qt::Key_Uacute << Qt::KeyboardModifiers() << "ú";
    QTest::newRow("Ú (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyU"
                               << "U" << Qt::Key_Uacute << Qt::KeyboardModifiers(Qt::ShiftModifier)
                               << "Ú";
    QTest::newRow("ü (Mac Umlaut)") << "KeyU" << Qt::KeyboardModifiers() << "KeyU"
                                    << "u" << Qt::Key_Udiaeresis << Qt::KeyboardModifiers() << "ü";
    QTest::newRow("Ü (Mac Umlaut)")
            << "KeyU" << Qt::KeyboardModifiers() << "KeyU"
            << "U" << Qt::Key_Udiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ü";
    QTest::newRow("ü (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyU"
            << "u" << Qt::Key_Udiaeresis << Qt::KeyboardModifiers() << "ü";
    QTest::newRow("Ü (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyU"
            << "U" << Qt::Key_Udiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ü";
    QTest::newRow("û") << "KeyI" << Qt::KeyboardModifiers() << "KeyU"
                       << "û" << Qt::Key_Ucircumflex << Qt::KeyboardModifiers() << "û";
    QTest::newRow("Û") << "KeyI" << Qt::KeyboardModifiers() << "KeyU"
                       << "U" << Qt::Key_Ucircumflex << Qt::KeyboardModifiers(Qt::ShiftModifier)
                       << "Û";
    QTest::newRow("û (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyU"
                               << "û" << Qt::Key_Ucircumflex << Qt::KeyboardModifiers() << "û";
    QTest::newRow("Û (^ key)") << "Digit6" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyU"
                               << "U" << Qt::Key_Ucircumflex
                               << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Û";

    QTest::newRow("ý") << "KeyE" << Qt::KeyboardModifiers() << "KeyY"
                       << "y" << Qt::Key_Yacute << Qt::KeyboardModifiers() << "ý";
    QTest::newRow("Ý") << "KeyE" << Qt::KeyboardModifiers() << "KeyY"
                       << "Y" << Qt::Key_Yacute << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ý";
    QTest::newRow("ý (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyY"
                               << "y" << Qt::Key_Yacute << Qt::KeyboardModifiers() << "ý";
    QTest::newRow("Ý (Quote)") << "Quote" << Qt::KeyboardModifiers() << "KeyY"
                               << "Y" << Qt::Key_Yacute << Qt::KeyboardModifiers(Qt::ShiftModifier)
                               << "Ý";
    QTest::newRow("ÿ (Mac Umlaut)") << "KeyU" << Qt::KeyboardModifiers() << "KeyY"
                                    << "y" << Qt::Key_ydiaeresis << Qt::KeyboardModifiers() << "ÿ";
    QTest::newRow("Ÿ (Mac Umlaut)")
            << "KeyU" << Qt::KeyboardModifiers() << "KeyY"
            << "Y" << Qt::Key_ydiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ÿ";
    QTest::newRow("ÿ (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyY"
            << "y" << Qt::Key_ydiaeresis << Qt::KeyboardModifiers() << "ÿ";
    QTest::newRow("Ÿ (Shift+Quote)")
            << "Quote" << Qt::KeyboardModifiers(Qt::ShiftModifier) << "KeyY"
            << "Y" << Qt::Key_ydiaeresis << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Ÿ";
}

void tst_QWasmKeyTranslator::modifyByDeadKey()
{
    QFETCH(QString, deadKeyCode);
    QFETCH(Qt::KeyboardModifiers, deadKeyModifiers);
    QFETCH(QString, targetKeyCode);
    QFETCH(QString, targetKey);
    QFETCH(Qt::Key, targetQtKey);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QString, expectedModifiedKey);

    QWasmDeadKeySupport deadKeySupport;

    KeyEvent event(EventType::KeyDown, makeDeadKeyJsEvent(deadKeyCode, deadKeyModifiers));
    QCOMPARE(event.deadKey, true);

    deadKeySupport.applyDeadKeyTranslations(&event);

    KeyEvent eDown(EventType::KeyDown, makeKeyJsEvent(targetKeyCode, targetKey, modifiers));
    QCOMPARE(eDown.deadKey, false);
    deadKeySupport.applyDeadKeyTranslations(&eDown);
    QCOMPARE(eDown.deadKey, false);
    QCOMPARE(eDown.text, expectedModifiedKey);
    QCOMPARE(eDown.key, targetQtKey);

    KeyEvent eUp(EventType::KeyUp, makeKeyJsEvent(targetKeyCode, targetKey, modifiers));
    QCOMPARE(eUp.deadKey, false);
    deadKeySupport.applyDeadKeyTranslations(&eUp);
    QCOMPARE(eUp.text, expectedModifiedKey);
    QCOMPARE(eUp.key, targetQtKey);
}

void tst_QWasmKeyTranslator::deadKeyModifiesOnlyOneKeyPressAndUp()
{
    QWasmDeadKeySupport deadKeySupport;
    KeyEvent event(EventType::KeyDown, makeDeadKeyJsEvent("KeyU", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&event);

    KeyEvent eDown(EventType::KeyDown, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eDown);
    QCOMPARE(eDown.text, "ü");
    QCOMPARE(eDown.key, Qt::Key_Udiaeresis);

    KeyEvent eUp(EventType::KeyUp, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eUp);
    QCOMPARE(eUp.text, "ü");
    QCOMPARE(eUp.key, Qt::Key_Udiaeresis);

    KeyEvent eDown2(EventType::KeyDown, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eDown2);
    QCOMPARE(eDown2.text, "u");
    QCOMPARE(eDown2.key, Qt::Key_U);

    KeyEvent eUp2(EventType::KeyUp, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eUp2);
    QCOMPARE(eUp2.text, "u");
    QCOMPARE(eUp2.key, Qt::Key_U);
}

void tst_QWasmKeyTranslator::deadKeyIgnoresKeyUpPrecedingKeyDown()
{
    QWasmDeadKeySupport deadKeySupport;

    KeyEvent deadKeyDownEvent(EventType::KeyDown,
                              makeDeadKeyJsEvent("KeyU", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&deadKeyDownEvent);

    KeyEvent deadKeyUpEvent(EventType::KeyUp, makeDeadKeyJsEvent("KeyU", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&deadKeyUpEvent);

    KeyEvent otherKeyUpEvent(EventType::KeyUp,
                             makeKeyJsEvent("AltLeft", "Alt", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&otherKeyUpEvent);

    KeyEvent eDown(EventType::KeyDown, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eDown);
    QCOMPARE(eDown.text, "ü");
    QCOMPARE(eDown.key, Qt::Key_Udiaeresis);

    KeyEvent yetAnotherKeyUpEvent(
            EventType::KeyUp, makeKeyJsEvent("ControlLeft", "Control", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&yetAnotherKeyUpEvent);

    KeyEvent eUp(EventType::KeyUp, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eUp);
    QCOMPARE(eUp.text, "ü");
    QCOMPARE(eUp.key, Qt::Key_Udiaeresis);
}

void tst_QWasmKeyTranslator::onlyKeysProducingTextAreModifiedByDeadKeys()
{
    QWasmDeadKeySupport deadKeySupport;

    KeyEvent deadKeyDownEvent(EventType::KeyDown,
                              makeDeadKeyJsEvent("KeyU", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&deadKeyDownEvent);

    KeyEvent noTextKeyDown(EventType::KeyDown,
                           makeKeyJsEvent("AltLeft", "Alt", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&noTextKeyDown);
    QCOMPARE(noTextKeyDown.text, "");
    QCOMPARE(noTextKeyDown.key, Qt::Key_Alt);

    KeyEvent noTextKeyUp(EventType::KeyUp,
                         makeKeyJsEvent("AltLeft", "Alt", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&noTextKeyUp);
    QCOMPARE(noTextKeyDown.text, "");
    QCOMPARE(noTextKeyDown.key, Qt::Key_Alt);

    KeyEvent eDown(EventType::KeyDown, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eDown);
    QCOMPARE(eDown.text, "ü");
    QCOMPARE(eDown.key, Qt::Key_Udiaeresis);

    KeyEvent eUp(EventType::KeyUp, makeKeyJsEvent("KeyU", "u", Qt::KeyboardModifiers()));
    deadKeySupport.applyDeadKeyTranslations(&eUp);
    QCOMPARE(eUp.text, "ü");
    QCOMPARE(eUp.key, Qt::Key_Udiaeresis);
}

QTEST_MAIN(tst_QWasmKeyTranslator)
#include "tst_qwasmkeytranslator.moc"
