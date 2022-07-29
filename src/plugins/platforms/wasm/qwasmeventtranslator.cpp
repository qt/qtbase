// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmeventtranslator.h"
#include "qwasmeventdispatcher.h"
#include "qwasmcompositor.h"
#include "qwasmintegration.h"
#include "qwasmclipboard.h"
#include "qwasmstring.h"
#include "qwasmcursor.h"
#include <QtGui/qevent.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>

#include <QtCore/qdeadlinetimer.h>
#include <private/qmakearray_p.h>
#include <QtCore/qnamespace.h>
#include <QCursor>
#include <QtCore/private/qstringiterator_p.h>

#include <emscripten/bind.h>

#include <iostream>

QT_BEGIN_NAMESPACE

using namespace emscripten;

namespace {
constexpr std::string_view WebDeadKeyValue = "Dead";

struct Emkb2QtData
{
    static constexpr char StringTerminator = '\0';

    const char *em;
    unsigned int qt;

    constexpr bool operator<=(const Emkb2QtData &that) const noexcept
    {
        return !(strcmp(that) > 0);
    }

    bool operator<(const Emkb2QtData &that) const noexcept { return ::strcmp(em, that.em) < 0; }

    constexpr bool operator==(const Emkb2QtData &that) const noexcept { return strcmp(that) == 0; }

    constexpr int strcmp(const Emkb2QtData &that, const int i = 0) const
    {
        return em[i] == StringTerminator && that.em[i] == StringTerminator ? 0
                : em[i] == StringTerminator                                ? -1
                : that.em[i] == StringTerminator                           ? 1
                : em[i] < that.em[i]                                       ? -1
                : em[i] > that.em[i]                                       ? 1
                                                                           : strcmp(that, i + 1);
    }
};

template<unsigned int Qt, char ... EmChar>
struct Emkb2Qt
{
    static constexpr const char storage[sizeof ... (EmChar) + 1] = {EmChar..., '\0'};
    using Type = Emkb2QtData;
    static constexpr Type data() noexcept { return Type{storage, Qt}; }
};

template<unsigned int Qt, char ... EmChar> constexpr char Emkb2Qt<Qt, EmChar...>::storage[];

static constexpr const auto WebToQtKeyCodeMappings = qMakeArray(
    QSortedData<
        Emkb2Qt< Qt::Key_Escape,        'E','s','c','a','p','e' >,
        Emkb2Qt< Qt::Key_Tab,           'T','a','b' >,
        Emkb2Qt< Qt::Key_Backspace,     'B','a','c','k','s','p','a','c','e' >,
        Emkb2Qt< Qt::Key_Return,        'E','n','t','e','r' >,
        Emkb2Qt< Qt::Key_Insert,        'I','n','s','e','r','t' >,
        Emkb2Qt< Qt::Key_Delete,        'D','e','l','e','t','e' >,
        Emkb2Qt< Qt::Key_Pause,         'P','a','u','s','e' >,
        Emkb2Qt< Qt::Key_Pause,         'C','l','e','a','r' >,
        Emkb2Qt< Qt::Key_Home,          'H','o','m','e' >,
        Emkb2Qt< Qt::Key_End,           'E','n','d' >,
        Emkb2Qt< Qt::Key_Left,          'A','r','r','o','w','L','e','f','t' >,
        Emkb2Qt< Qt::Key_Up,            'A','r','r','o','w','U','p' >,
        Emkb2Qt< Qt::Key_Right,         'A','r','r','o','w','R','i','g','h','t' >,
        Emkb2Qt< Qt::Key_Down,          'A','r','r','o','w','D','o','w','n' >,
        Emkb2Qt< Qt::Key_PageUp,        'P','a','g','e','U','p' >,
        Emkb2Qt< Qt::Key_PageDown,      'P','a','g','e','D','o','w','n' >,
        Emkb2Qt< Qt::Key_Shift,         'S','h','i','f','t' >,
        Emkb2Qt< Qt::Key_Control,       'C','o','n','t','r','o','l' >,
        Emkb2Qt< Qt::Key_Meta,          'M','e','t','a'>,
        Emkb2Qt< Qt::Key_Meta,          'O','S'>,
        Emkb2Qt< Qt::Key_Alt,           'A','l','t','L','e','f','t' >,
        Emkb2Qt< Qt::Key_Alt,           'A','l','t' >,
        Emkb2Qt< Qt::Key_CapsLock,      'C','a','p','s','L','o','c','k' >,
        Emkb2Qt< Qt::Key_NumLock,       'N','u','m','L','o','c','k' >,
        Emkb2Qt< Qt::Key_ScrollLock,    'S','c','r','o','l','l','L','o','c','k' >,
        Emkb2Qt< Qt::Key_F1,            'F','1' >,
        Emkb2Qt< Qt::Key_F2,            'F','2' >,
        Emkb2Qt< Qt::Key_F3,            'F','3' >,
        Emkb2Qt< Qt::Key_F4,            'F','4' >,
        Emkb2Qt< Qt::Key_F5,            'F','5' >,
        Emkb2Qt< Qt::Key_F6,            'F','6' >,
        Emkb2Qt< Qt::Key_F7,            'F','7' >,
        Emkb2Qt< Qt::Key_F8,            'F','8' >,
        Emkb2Qt< Qt::Key_F9,            'F','9' >,
        Emkb2Qt< Qt::Key_F10,           'F','1','0' >,
        Emkb2Qt< Qt::Key_F11,           'F','1','1' >,
        Emkb2Qt< Qt::Key_F12,           'F','1','2' >,
        Emkb2Qt< Qt::Key_F13,           'F','1','3' >,
        Emkb2Qt< Qt::Key_F14,           'F','1','4' >,
        Emkb2Qt< Qt::Key_F15,           'F','1','5' >,
        Emkb2Qt< Qt::Key_F16,           'F','1','6' >,
        Emkb2Qt< Qt::Key_F17,           'F','1','7' >,
        Emkb2Qt< Qt::Key_F18,           'F','1','8' >,
        Emkb2Qt< Qt::Key_F19,           'F','1','9' >,
        Emkb2Qt< Qt::Key_F20,           'F','2','0' >,
        Emkb2Qt< Qt::Key_F21,           'F','2','1' >,
        Emkb2Qt< Qt::Key_F22,           'F','2','2' >,
        Emkb2Qt< Qt::Key_F23,           'F','2','3' >,
        Emkb2Qt< Qt::Key_Paste,         'P','a','s','t','e' >,
        Emkb2Qt< Qt::Key_AltGr,         'A','l','t','R','i','g','h','t' >,
        Emkb2Qt< Qt::Key_Help,          'H','e','l','p' >,
        Emkb2Qt< Qt::Key_yen,           'I','n','t','l','Y','e','n' >,
        Emkb2Qt< Qt::Key_Menu,          'C','o','n','t','e','x','t','M','e','n','u' >
            >::Data{}
        );

static constexpr const auto WebToQtKeyCodeMappingsWithShift = qMakeArray(
    QSortedData<
       // shifted
        Emkb2Qt< Qt::Key_Agrave,        '\xc3','\x80' >,
        Emkb2Qt< Qt::Key_Aacute,        '\xc3','\x81' >,
        Emkb2Qt< Qt::Key_Acircumflex,   '\xc3','\x82' >,
        Emkb2Qt< Qt::Key_Adiaeresis,    '\xc3','\x84' >,
        Emkb2Qt< Qt::Key_AE,            '\xc3','\x86' >,
        Emkb2Qt< Qt::Key_Atilde,        '\xc3','\x83' >,
        Emkb2Qt< Qt::Key_Aring,         '\xc3','\x85' >,
        Emkb2Qt< Qt::Key_Egrave,        '\xc3','\x88' >,
        Emkb2Qt< Qt::Key_Eacute,        '\xc3','\x89' >,
        Emkb2Qt< Qt::Key_Ecircumflex,   '\xc3','\x8a' >,
        Emkb2Qt< Qt::Key_Ediaeresis,    '\xc3','\x8b' >,
        Emkb2Qt< Qt::Key_Icircumflex,   '\xc3','\x8e' >,
        Emkb2Qt< Qt::Key_Idiaeresis,    '\xc3','\x8f' >,
        Emkb2Qt< Qt::Key_Ocircumflex,   '\xc3','\x94' >,
        Emkb2Qt< Qt::Key_Odiaeresis,    '\xc3','\x96' >,
        Emkb2Qt< Qt::Key_Ograve,        '\xc3','\x92' >,
        Emkb2Qt< Qt::Key_Oacute,        '\xc3','\x93' >,
        Emkb2Qt< Qt::Key_Ooblique,      '\xc3','\x98' >,
        Emkb2Qt< Qt::Key_Otilde,        '\xc3','\x95' >,
        Emkb2Qt< Qt::Key_Ucircumflex,   '\xc3','\x9b' >,
        Emkb2Qt< Qt::Key_Udiaeresis,    '\xc3','\x9c' >,
        Emkb2Qt< Qt::Key_Ugrave,        '\xc3','\x99' >,
        Emkb2Qt< Qt::Key_Uacute,        '\xc3','\x9a' >,
        Emkb2Qt< Qt::Key_Ntilde,        '\xc3','\x91' >,
        Emkb2Qt< Qt::Key_Ccedilla,      '\xc3','\x87' >,
        Emkb2Qt< Qt::Key_ydiaeresis,    '\xc3','\x8f' >
    >::Data{}
);

std::optional<Qt::Key> findMappingByBisection(const char *toFind)
{
    const Emkb2QtData searchKey{ toFind, 0 };
    const auto it = std::lower_bound(WebToQtKeyCodeMappings.cbegin(), WebToQtKeyCodeMappings.cend(),
                                     searchKey);
    return it != WebToQtKeyCodeMappings.cend() && searchKey == *it ? static_cast<Qt::Key>(it->qt)
                                                                   : std::optional<Qt::Key>();
}

bool isDeadKeyEvent(const EmscriptenKeyboardEvent *emKeyEvent)
{
    return qstrncmp(emKeyEvent->key, WebDeadKeyValue.data(), WebDeadKeyValue.size()) == 0;
}

Qt::Key translateEmscriptKey(const EmscriptenKeyboardEvent *emscriptKey)
{
    if (isDeadKeyEvent(emscriptKey)) {
        if (auto mapping = findMappingByBisection(emscriptKey->code))
            return *mapping;
    }
    if (auto mapping = findMappingByBisection(emscriptKey->key))
        return *mapping;

    // cast to unicode key
    QString str = QString::fromUtf8(emscriptKey->key).toUpper();
    QStringIterator i(str);
    return static_cast<Qt::Key>(i.next(0));
}

struct KeyMapping { Qt::Key from, to; };

constexpr KeyMapping tildeKeyTable[] = { // ~
    { Qt::Key_A, Qt::Key_Atilde },
    { Qt::Key_N, Qt::Key_Ntilde },
    { Qt::Key_O, Qt::Key_Otilde },
};
constexpr KeyMapping graveKeyTable[] = { // `
    { Qt::Key_A, Qt::Key_Agrave },
    { Qt::Key_E, Qt::Key_Egrave },
    { Qt::Key_I, Qt::Key_Igrave },
    { Qt::Key_O, Qt::Key_Ograve },
    { Qt::Key_U, Qt::Key_Ugrave },
};
constexpr KeyMapping acuteKeyTable[] = { // '
    { Qt::Key_A, Qt::Key_Aacute },
    { Qt::Key_E, Qt::Key_Eacute },
    { Qt::Key_I, Qt::Key_Iacute },
    { Qt::Key_O, Qt::Key_Oacute },
    { Qt::Key_U, Qt::Key_Uacute },
    { Qt::Key_Y, Qt::Key_Yacute },
};
constexpr KeyMapping diaeresisKeyTable[] = { // umlaut ¨
    { Qt::Key_A, Qt::Key_Adiaeresis },
    { Qt::Key_E, Qt::Key_Ediaeresis },
    { Qt::Key_I, Qt::Key_Idiaeresis },
    { Qt::Key_O, Qt::Key_Odiaeresis },
    { Qt::Key_U, Qt::Key_Udiaeresis },
    { Qt::Key_Y, Qt::Key_ydiaeresis },
};
constexpr KeyMapping circumflexKeyTable[] = { // ^
    { Qt::Key_A, Qt::Key_Acircumflex },
    { Qt::Key_E, Qt::Key_Ecircumflex },
    { Qt::Key_I, Qt::Key_Icircumflex },
    { Qt::Key_O, Qt::Key_Ocircumflex },
    { Qt::Key_U, Qt::Key_Ucircumflex },
};

static Qt::Key find_impl(const KeyMapping *first, const KeyMapping *last, Qt::Key key) noexcept
{
    while (first != last) {
        if (first->from == key)
            return first->to;
        ++first;
    }
    return Qt::Key_unknown;
}

template <size_t N>
static Qt::Key find(const KeyMapping (&map)[N], Qt::Key key) noexcept
{
    return find_impl(map, map + N, key);
}

Qt::Key translateBaseKeyUsingDeadKey(Qt::Key accentBaseKey, Qt::Key deadKey)
{
    switch (deadKey) {
    case Qt::Key_QuoteLeft: {
        // ` macOS: Key_Dead_Grave
        return platform() == Platform::MacOS ? find(graveKeyTable, accentBaseKey)
                                             : find(diaeresisKeyTable, accentBaseKey);
    }
    case Qt::Key_O: // ´ Key_Dead_Grave
        return find(graveKeyTable, accentBaseKey);
    case Qt::Key_E: // ´ Key_Dead_Acute
        return find(acuteKeyTable, accentBaseKey);
    case Qt::Key_AsciiTilde:
    case Qt::Key_N: // Key_Dead_Tilde
        return find(tildeKeyTable, accentBaseKey);
    case Qt::Key_U: // ¨ Key_Dead_Diaeresis
        return find(diaeresisKeyTable, accentBaseKey);
    case Qt::Key_I: // macOS Key_Dead_Circumflex
    case Qt::Key_6: // linux
    case Qt::Key_Apostrophe: // linux
        return find(circumflexKeyTable, accentBaseKey);
    default:
        return Qt::Key_unknown;
    };
}

template<class T>
std::optional<QString> findKeyTextByKeyId(const T &mappingArray, Qt::Key qtKey)
{
    const auto it = std::find_if(mappingArray.cbegin(), mappingArray.cend(),
                                 [qtKey](const Emkb2QtData &data) { return data.qt == qtKey; });
    return it != mappingArray.cend() ? it->em : std::optional<QString>();
}
} // namespace

QWasmEventTranslator::QWasmEventTranslator() = default;

QWasmEventTranslator::~QWasmEventTranslator() = default;

QCursor QWasmEventTranslator::cursorForMode(QWasmCompositor::ResizeMode m)
{
    switch (m) {
    case QWasmCompositor::ResizeTopLeft:
    case QWasmCompositor::ResizeBottomRight:
        return Qt::SizeFDiagCursor;
    case QWasmCompositor::ResizeBottomLeft:
    case QWasmCompositor::ResizeTopRight:
        return Qt::SizeBDiagCursor;
    case QWasmCompositor::ResizeTop:
    case QWasmCompositor::ResizeBottom:
        return Qt::SizeVerCursor;
    case QWasmCompositor::ResizeLeft:
    case QWasmCompositor::ResizeRight:
        return Qt::SizeHorCursor;
    case QWasmCompositor::ResizeNone:
        return Qt::ArrowCursor;
    }
    return Qt::ArrowCursor;
}

QWasmEventTranslator::TranslatedEvent
QWasmEventTranslator::translateKeyEvent(int emEventType, const EmscriptenKeyboardEvent *keyEvent)
{
    TranslatedEvent ret;
    switch (emEventType) {
    case EMSCRIPTEN_EVENT_KEYDOWN:
        ret.type = QEvent::KeyPress;
        break;
    case EMSCRIPTEN_EVENT_KEYUP:
        ret.type = QEvent::KeyRelease;
        break;
    default:
        // Should not be reached - do not call with this event type.
        Q_ASSERT(false);
        break;
    };

    ret.key = translateEmscriptKey(keyEvent);

    if (isDeadKeyEvent(keyEvent) || ret.key == Qt::Key_AltGr) {
        if (keyEvent->shiftKey && ret.key == Qt::Key_QuoteLeft)
            ret.key = Qt::Key_AsciiTilde;
        m_emDeadKey = ret.key;
    }

    if (m_emDeadKey != Qt::Key_unknown
        && (m_keyModifiedByDeadKeyOnPress == Qt::Key_unknown
            || ret.key == m_keyModifiedByDeadKeyOnPress)) {
        const Qt::Key baseKey = ret.key;
        const Qt::Key translatedKey = translateBaseKeyUsingDeadKey(baseKey, m_emDeadKey);
        if (translatedKey != Qt::Key_unknown)
            ret.key = translatedKey;

        if (auto text = keyEvent->shiftKey
                    ? findKeyTextByKeyId(WebToQtKeyCodeMappingsWithShift, ret.key)
                    : findKeyTextByKeyId(WebToQtKeyCodeMappings, ret.key)) {
            if (ret.type == QEvent::KeyPress) {
                Q_ASSERT(m_keyModifiedByDeadKeyOnPress == Qt::Key_unknown);
                m_keyModifiedByDeadKeyOnPress = baseKey;
            } else {
                Q_ASSERT(ret.type == QEvent::KeyRelease);
                Q_ASSERT(m_keyModifiedByDeadKeyOnPress == baseKey);
                m_keyModifiedByDeadKeyOnPress = Qt::Key_unknown;
                m_emDeadKey = Qt::Key_unknown;
            }
            ret.text = *text;
            return ret;
        }
    }
    ret.text = QString::fromUtf8(keyEvent->key);
    return ret;
}

QT_END_NAMESPACE
