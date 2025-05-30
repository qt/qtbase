// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmkeytranslator.h"
#include "qwasmevent.h"

#include <QtCore/private/qmakearray_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace {
struct WebKb2QtData
{
    static constexpr char StringTerminator = '\0';

    const char *web;
    unsigned int qt;

    constexpr bool operator<=(const WebKb2QtData &that) const noexcept
    {
        return !(strcmp(that) > 0);
    }

    bool operator<(const WebKb2QtData &that) const noexcept { return ::strcmp(web, that.web) < 0; }

    constexpr bool operator==(const WebKb2QtData &that) const noexcept { return strcmp(that) == 0; }

    constexpr int strcmp(const WebKb2QtData &that, const int i = 0) const
    {
        return web[i] == StringTerminator && that.web[i] == StringTerminator ? 0
                : web[i] == StringTerminator                                 ? -1
                : that.web[i] == StringTerminator                            ? 1
                : web[i] < that.web[i]                                       ? -1
                : web[i] > that.web[i]                                       ? 1
                                                                             : strcmp(that, i + 1);
    }
};

template<unsigned int Qt, char... WebChar>
struct Web2Qt
{
    static constexpr const char storage[sizeof...(WebChar) + 1] = { WebChar..., '\0' };
    using Type = WebKb2QtData;
    static constexpr Type data() noexcept { return Type{ storage, Qt }; }
};

template<unsigned int Qt, char... WebChar>
constexpr char Web2Qt<Qt, WebChar...>::storage[];

static constexpr const auto WebToQtKeyCodeMappings = qMakeArray(
        QSortedData<Web2Qt<Qt::Key_Alt, 'A', 'l', 't', 'L', 'e', 'f', 't'>,
                    Web2Qt<Qt::Key_Alt, 'A', 'l', 't'>,
                    Web2Qt<Qt::Key_AltGr, 'A', 'l', 't', 'R', 'i', 'g', 'h', 't'>,
                    Web2Qt<Qt::Key_Apostrophe, 'Q', 'u', 'o', 't', 'e'>,
                    Web2Qt<Qt::Key_Backspace, 'B', 'a', 'c', 'k', 's', 'p', 'a', 'c', 'e'>,
                    Web2Qt<Qt::Key_CapsLock, 'C', 'a', 'p', 's', 'L', 'o', 'c', 'k'>,
                    Web2Qt<Qt::Key_Control, 'C', 'o', 'n', 't', 'r', 'o', 'l'>,
                    Web2Qt<Qt::Key_Delete, 'D', 'e', 'l', 'e', 't', 'e'>,
                    Web2Qt<Qt::Key_Down, 'A', 'r', 'r', 'o', 'w', 'D', 'o', 'w', 'n'>,
                    Web2Qt<Qt::Key_Escape, 'E', 's', 'c', 'a', 'p', 'e'>,
                    Web2Qt<Qt::Key_F1, 'F', '1'>, Web2Qt<Qt::Key_F2, 'F', '2'>,
                    Web2Qt<Qt::Key_F11, 'F', '1', '1'>, Web2Qt<Qt::Key_F12, 'F', '1', '2'>,
                    Web2Qt<Qt::Key_F13, 'F', '1', '3'>, Web2Qt<Qt::Key_F14, 'F', '1', '4'>,
                    Web2Qt<Qt::Key_F15, 'F', '1', '5'>, Web2Qt<Qt::Key_F16, 'F', '1', '6'>,
                    Web2Qt<Qt::Key_F17, 'F', '1', '7'>, Web2Qt<Qt::Key_F18, 'F', '1', '8'>,
                    Web2Qt<Qt::Key_F19, 'F', '1', '9'>, Web2Qt<Qt::Key_F20, 'F', '2', '0'>,
                    Web2Qt<Qt::Key_F21, 'F', '2', '1'>, Web2Qt<Qt::Key_F22, 'F', '2', '2'>,
                    Web2Qt<Qt::Key_F23, 'F', '2', '3'>,
                    Web2Qt<Qt::Key_F3, 'F', '3'>, Web2Qt<Qt::Key_F4, 'F', '4'>,
                    Web2Qt<Qt::Key_F5, 'F', '5'>, Web2Qt<Qt::Key_F6, 'F', '6'>,
                    Web2Qt<Qt::Key_F7, 'F', '7'>, Web2Qt<Qt::Key_F8, 'F', '8'>,
                    Web2Qt<Qt::Key_F9, 'F', '9'>, Web2Qt<Qt::Key_F10, 'F', '1', '0'>,
                    Web2Qt<Qt::Key_Help, 'H', 'e', 'l', 'p'>,
                    Web2Qt<Qt::Key_Home, 'H', 'o', 'm', 'e'>, Web2Qt<Qt::Key_End, 'E', 'n', 'd'>,
                    Web2Qt<Qt::Key_Insert, 'I', 'n', 's', 'e', 'r', 't'>,
                    Web2Qt<Qt::Key_Left, 'A', 'r', 'r', 'o', 'w', 'L', 'e', 'f', 't'>,
                    Web2Qt<Qt::Key_Meta, 'M', 'e', 't', 'a'>, Web2Qt<Qt::Key_Meta, 'O', 'S'>,
                    Web2Qt<Qt::Key_Menu, 'C', 'o', 'n', 't', 'e', 'x', 't', 'M', 'e', 'n', 'u'>,
                    Web2Qt<Qt::Key_NumLock, 'N', 'u', 'm', 'L', 'o', 'c', 'k'>,
                    Web2Qt<Qt::Key_PageDown, 'P', 'a', 'g', 'e', 'D', 'o', 'w', 'n'>,
                    Web2Qt<Qt::Key_PageUp, 'P', 'a', 'g', 'e', 'U', 'p'>,
                    Web2Qt<Qt::Key_Paste, 'P', 'a', 's', 't', 'e'>,
                    Web2Qt<Qt::Key_Pause, 'C', 'l', 'e', 'a', 'r'>,
                    Web2Qt<Qt::Key_Pause, 'P', 'a', 'u', 's', 'e'>,
                    Web2Qt<Qt::Key_QuoteLeft, 'B', 'a', 'c', 'k', 'q', 'u', 'o', 't', 'e'>,
                    Web2Qt<Qt::Key_QuoteLeft, 'I', 'n', 't', 'l', 'B', 'a', 'c', 'k', 's', 'l', 'a', 's', 'h'>,
                    Web2Qt<Qt::Key_Return, 'E', 'n', 't', 'e', 'r'>,
                    Web2Qt<Qt::Key_Right, 'A', 'r', 'r', 'o', 'w', 'R', 'i', 'g', 'h', 't'>,
                    Web2Qt<Qt::Key_ScrollLock, 'S', 'c', 'r', 'o', 'l', 'l', 'L', 'o', 'c', 'k'>,
                    Web2Qt<Qt::Key_Shift, 'S', 'h', 'i', 'f', 't'>,
                    Web2Qt<Qt::Key_Tab, 'T', 'a', 'b'>,
                    Web2Qt<Qt::Key_Up, 'A', 'r', 'r', 'o', 'w', 'U', 'p'>,
                    Web2Qt<Qt::Key_yen, 'I', 'n', 't', 'l', 'Y', 'e', 'n'>>::Data{});

static constexpr const auto DiacriticalCharsKeyToTextLowercase = qMakeArray(
        QSortedData<
                Web2Qt<Qt::Key_Aacute, '\xc3', '\xa1'>,
                Web2Qt<Qt::Key_Acircumflex, '\xc3', '\xa2'>,
                Web2Qt<Qt::Key_Adiaeresis, '\xc3', '\xa4'>,
                Web2Qt<Qt::Key_AE, '\xc3', '\xa6'>,
                Web2Qt<Qt::Key_Agrave, '\xc3', '\xa0'>,
                Web2Qt<Qt::Key_Aring, '\xc3', '\xa5'>,
                Web2Qt<Qt::Key_Atilde, '\xc3', '\xa3'>,
                Web2Qt<Qt::Key_Ccedilla, '\xc3', '\xa7'>,
                Web2Qt<Qt::Key_Eacute, '\xc3', '\xa9'>,
                Web2Qt<Qt::Key_Ecircumflex, '\xc3', '\xaa'>,
                Web2Qt<Qt::Key_Ediaeresis, '\xc3', '\xab'>,
                Web2Qt<Qt::Key_Egrave, '\xc3', '\xa8'>,
                Web2Qt<Qt::Key_Iacute, '\xc3', '\xad'>,
                Web2Qt<Qt::Key_Icircumflex, '\xc3', '\xae'>,
                Web2Qt<Qt::Key_Idiaeresis, '\xc3', '\xaf'>,
                Web2Qt<Qt::Key_Igrave, '\xc3', '\xac'>,
                Web2Qt<Qt::Key_Ntilde, '\xc3', '\xb1'>,
                Web2Qt<Qt::Key_Oacute, '\xc3', '\xb3'>,
                Web2Qt<Qt::Key_Ocircumflex, '\xc3', '\xb4'>,
                Web2Qt<Qt::Key_Odiaeresis, '\xc3', '\xb6'>,
                Web2Qt<Qt::Key_Ograve, '\xc3', '\xb2'>,
                Web2Qt<Qt::Key_Ooblique, '\xc3', '\xb8'>,
                Web2Qt<Qt::Key_Otilde, '\xc3', '\xb5'>,
                Web2Qt<Qt::Key_Uacute, '\xc3', '\xba'>,
                Web2Qt<Qt::Key_Ucircumflex, '\xc3', '\xbb'>,
                Web2Qt<Qt::Key_Udiaeresis, '\xc3', '\xbc'>,
                Web2Qt<Qt::Key_Ugrave, '\xc3', '\xb9'>,
                Web2Qt<Qt::Key_Yacute, '\xc3', '\xbd'>,
                Web2Qt<Qt::Key_ydiaeresis, '\xc3', '\xbf'>>::Data{});

static constexpr const auto DiacriticalCharsKeyToTextUppercase = qMakeArray(
        QSortedData<
                Web2Qt<Qt::Key_Aacute, '\xc3', '\x81'>,
                Web2Qt<Qt::Key_Acircumflex, '\xc3', '\x82'>,
                Web2Qt<Qt::Key_Adiaeresis, '\xc3', '\x84'>,
                Web2Qt<Qt::Key_AE, '\xc3', '\x86'>,
                Web2Qt<Qt::Key_Agrave, '\xc3', '\x80'>,
                Web2Qt<Qt::Key_Aring, '\xc3', '\x85'>,
                Web2Qt<Qt::Key_Atilde, '\xc3', '\x83'>,
                Web2Qt<Qt::Key_Ccedilla, '\xc3', '\x87'>,
                Web2Qt<Qt::Key_Eacute, '\xc3', '\x89'>,
                Web2Qt<Qt::Key_Ecircumflex, '\xc3', '\x8a'>,
                Web2Qt<Qt::Key_Ediaeresis, '\xc3', '\x8b'>,
                Web2Qt<Qt::Key_Egrave, '\xc3', '\x88'>,
                Web2Qt<Qt::Key_Iacute, '\xc3', '\x8d'>,
                Web2Qt<Qt::Key_Icircumflex, '\xc3', '\x8e'>,
                Web2Qt<Qt::Key_Idiaeresis, '\xc3', '\x8f'>,
                Web2Qt<Qt::Key_Igrave, '\xc3', '\x8c'>,
                Web2Qt<Qt::Key_Ntilde, '\xc3', '\x91'>,
                Web2Qt<Qt::Key_Oacute, '\xc3', '\x93'>,
                Web2Qt<Qt::Key_Ocircumflex, '\xc3', '\x94'>,
                Web2Qt<Qt::Key_Odiaeresis, '\xc3', '\x96'>,
                Web2Qt<Qt::Key_Ograve, '\xc3', '\x92'>,
                Web2Qt<Qt::Key_Ooblique, '\xc3', '\x98'>,
                Web2Qt<Qt::Key_Otilde, '\xc3', '\x95'>,
                Web2Qt<Qt::Key_Uacute, '\xc3', '\x9a'>,
                Web2Qt<Qt::Key_Ucircumflex, '\xc3', '\x9b'>,
                Web2Qt<Qt::Key_Udiaeresis, '\xc3', '\x9c'>,
                Web2Qt<Qt::Key_Ugrave, '\xc3', '\x99'>,
                Web2Qt<Qt::Key_Yacute, '\xc3', '\x9d'>,
                Web2Qt<Qt::Key_ydiaeresis, '\xc5', '\xb8'>>::Data{});

static_assert(DiacriticalCharsKeyToTextLowercase.size()
                      == DiacriticalCharsKeyToTextUppercase.size(),
              "Add the new key to both arrays");

struct KeyMapping
{
    Qt::Key from, to;
};

constexpr KeyMapping tildeKeyTable[] = {
    // ~
    { Qt::Key_A, Qt::Key_Atilde },
    { Qt::Key_N, Qt::Key_Ntilde },
    { Qt::Key_O, Qt::Key_Otilde },
};
constexpr KeyMapping graveKeyTable[] = {
    // `
    { Qt::Key_A, Qt::Key_Agrave }, { Qt::Key_E, Qt::Key_Egrave }, { Qt::Key_I, Qt::Key_Igrave },
    { Qt::Key_O, Qt::Key_Ograve }, { Qt::Key_U, Qt::Key_Ugrave },
};
constexpr KeyMapping acuteKeyTable[] = {
    // '
    { Qt::Key_A, Qt::Key_Aacute }, { Qt::Key_E, Qt::Key_Eacute }, { Qt::Key_I, Qt::Key_Iacute },
    { Qt::Key_O, Qt::Key_Oacute }, { Qt::Key_U, Qt::Key_Uacute }, { Qt::Key_Y, Qt::Key_Yacute },
};
constexpr KeyMapping diaeresisKeyTable[] = {
    // umlaut ¨
    { Qt::Key_A, Qt::Key_Adiaeresis }, { Qt::Key_E, Qt::Key_Ediaeresis },
    { Qt::Key_I, Qt::Key_Idiaeresis }, { Qt::Key_O, Qt::Key_Odiaeresis },
    { Qt::Key_U, Qt::Key_Udiaeresis }, { Qt::Key_Y, Qt::Key_ydiaeresis },
};
constexpr KeyMapping circumflexKeyTable[] = {
    // ^
    { Qt::Key_A, Qt::Key_Acircumflex }, { Qt::Key_E, Qt::Key_Ecircumflex },
    { Qt::Key_I, Qt::Key_Icircumflex }, { Qt::Key_O, Qt::Key_Ocircumflex },
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

template<size_t N>
static Qt::Key find(const KeyMapping (&map)[N], Qt::Key key) noexcept
{
    return find_impl(map, map + N, key);
}

Qt::Key translateBaseKeyUsingDeadKey(Qt::Key accentBaseKey, Qt::Key deadKey)
{
    switch (deadKey) {
    case Qt::Key_Dead_Grave:
        return find(graveKeyTable, accentBaseKey);
    case Qt::Key_Dead_Acute:
        return find(acuteKeyTable, accentBaseKey);
    case Qt::Key_Dead_Tilde:
        return find(tildeKeyTable, accentBaseKey);
    case Qt::Key_Dead_Diaeresis:
        return find(diaeresisKeyTable, accentBaseKey);
    case Qt::Key_Dead_Circumflex:
        return find(circumflexKeyTable, accentBaseKey);
    default:
        return Qt::Key_unknown;
    };
}

template<class T>
std::optional<QString> findKeyTextByKeyId(const T &mappingArray, Qt::Key qtKey)
{
    const auto it = std::find_if(mappingArray.cbegin(), mappingArray.cend(),
                                 [qtKey](const WebKb2QtData &data) { return data.qt == qtKey; });
    return it != mappingArray.cend() ? it->web : std::optional<QString>();
}
} // namespace

std::optional<Qt::Key> QWasmKeyTranslator::mapWebKeyTextToQtKey(const char *toFind)
{
    const WebKb2QtData searchKey{ toFind, 0 };
    const auto it = std::lower_bound(WebToQtKeyCodeMappings.cbegin(), WebToQtKeyCodeMappings.cend(),
                                     searchKey);
    return it != WebToQtKeyCodeMappings.cend() && searchKey == *it ? static_cast<Qt::Key>(it->qt)
                                                                   : std::optional<Qt::Key>();
}

QWasmDeadKeySupport::QWasmDeadKeySupport() = default;

QWasmDeadKeySupport::~QWasmDeadKeySupport() = default;

void QWasmDeadKeySupport::applyDeadKeyTranslations(KeyEvent *event)
{
    if (event->deadKey) {
        m_activeDeadKey = event->key;
    } else if (m_activeDeadKey != Qt::Key_unknown
               && (((m_keyModifiedByDeadKeyOnPress == Qt::Key_unknown
                     && event->type == EventType::KeyDown))
                   || (m_keyModifiedByDeadKeyOnPress == event->key
                       && event->type == EventType::KeyUp))) {
        const Qt::Key baseKey = event->key;
        const Qt::Key translatedKey = translateBaseKeyUsingDeadKey(baseKey, m_activeDeadKey);
        if (translatedKey != Qt::Key_unknown) {
            event->key = translatedKey;

            auto foundText = event->modifiers.testFlag(Qt::ShiftModifier)
                    ? findKeyTextByKeyId(DiacriticalCharsKeyToTextUppercase, event->key)
                    : findKeyTextByKeyId(DiacriticalCharsKeyToTextLowercase, event->key);
            Q_ASSERT(foundText.has_value());
            event->text = foundText->size() == 1 ? *foundText : QString();
        }

        if (!event->text.isEmpty()) {
            if (event->type == EventType::KeyDown) {
                // Assume the first keypress with an active dead key is treated as modified,
                // regardless of whether it has actually been modified or not. Take into account
                // only events that produce actual key text.
                if (!event->text.isEmpty())
                    m_keyModifiedByDeadKeyOnPress = baseKey;
            } else {
                Q_ASSERT(event->type == EventType::KeyUp);
                Q_ASSERT(m_keyModifiedByDeadKeyOnPress == baseKey);
                m_keyModifiedByDeadKeyOnPress = Qt::Key_unknown;
                m_activeDeadKey = Qt::Key_unknown;
            }
        }
    }
}

QT_END_NAMESPACE
