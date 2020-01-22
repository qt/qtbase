/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "language.h"

#include <QtCore/qtextstream.h>

namespace language {

static Encoding encoding = Encoding::Utf8;
static Language _language = Language::Cpp;

Language language() { return _language; }

void setLanguage(Language l)
{
    _language = l;
    switch (_language) {
    case Language::Cpp:
        derefPointer = QLatin1String("->");
        nullPtr = QLatin1String("nullptr");
        operatorNew = QLatin1String("new ");
        qtQualifier = QLatin1String("Qt::");
        qualifier = QLatin1String("::");
        self = QLatin1String("");  // for testing: change to "this->";
        eol = QLatin1String(";\n");
        emptyString = QLatin1String("QString()");
        encoding = Encoding::Utf8;
        break;
    case Language::Python:
        derefPointer = QLatin1String(".");
        nullPtr = QLatin1String("None");
        operatorNew = QLatin1String("");
        qtQualifier = QLatin1String("Qt.");
        qualifier = QLatin1String(".");
        self = QLatin1String("self.");
        eol = QLatin1String("\n");
        emptyString = QLatin1String("\"\"");
        encoding = Encoding::Unicode;
        break;
    }
}

QString derefPointer;
QString nullPtr;
QString operatorNew;
QString qtQualifier;
QString qualifier;
QString self;
QString eol;
QString emptyString;

QString cppQualifier = QLatin1String("::");
QString cppTrue = QLatin1String("true");
QString cppFalse = QLatin1String("false");

QTextStream &operator<<(QTextStream &str, const qtConfig &c)
{
    str << "QT_CONFIG(" << c.parameter() << ')';
    return str;
}

QTextStream &operator<<(QTextStream &str, const openQtConfig &c)
{
    str << "#if " << qtConfig(c.parameter())  << '\n';
    return str;
}

QTextStream &operator<<(QTextStream &str, const closeQtConfig &c)
{
    str << "#endif // " << qtConfig(c.parameter()) << '\n';
    return str;
}

struct EnumLookup
{
    int value;
    const char *valueString;
};

template <int N>
const char *lookupEnum(const EnumLookup(&array)[N], int value, int defaultIndex = 0)
{
    for (int i = 0; i < N; ++i) {
        if (value == array[i].value)
            return array[i].valueString;
    }
    const char *defaultValue = array[defaultIndex].valueString;
    qWarning("uic: Warning: Invalid enumeration value %d, defaulting to %s",
             value, defaultValue);
    return defaultValue;
}

QString fixClassName(QString className)
{
    if (language() == Language::Python)
        className.replace(cppQualifier, QLatin1String("_"));
    return className;
}

const char *toolbarArea(int v)
{
    static const EnumLookup toolBarAreas[] =
    {
        {0,   "NoToolBarArea"},
        {0x1, "LeftToolBarArea"},
        {0x2, "RightToolBarArea"},
        {0x4, "TopToolBarArea"},
        {0x8, "BottomToolBarArea"},
        {0xf, "AllToolBarAreas"}
    };
    return lookupEnum(toolBarAreas, v);
}

const char *sizePolicy(int v)
{
    static const EnumLookup sizePolicies[] =
    {
        {0,   "Fixed"},
        {0x1, "Minimum"},
        {0x4, "Maximum"},
        {0x5, "Preferred"},
        {0x3, "MinimumExpanding"},
        {0x7, "Expanding"},
        {0xD, "Ignored"}
    };
    return lookupEnum(sizePolicies, v, 3);
}

const char *dockWidgetArea(int v)
{
    static const EnumLookup dockWidgetAreas[] =
    {
        {0,   "NoDockWidgetArea"},
        {0x1, "LeftDockWidgetArea"},
        {0x2, "RightDockWidgetArea"},
        {0x4, "TopDockWidgetArea"},
        {0x8, "BottomDockWidgetArea"},
        {0xf, "AllDockWidgetAreas"}
    };
    return lookupEnum(dockWidgetAreas, v);
}

const char *paletteColorRole(int v)
{
    static const EnumLookup colorRoles[] =
    {
        {0, "WindowText"},
        {1, "Button"},
        {2, "Light"},
        {3, "Midlight"},
        {4, "Dark"},
        {5, "Mid"},
        {6, "Text"},
        {7, "BrightText"},
        {8, "ButtonText"},
        {9, "Base"},
        {10, "Window"},
        {11, "Shadow"},
        {12, "Highlight"},
        {13, "HighlightedText"},
        {14, "Link"},
        {15, "LinkVisited"},
        {16, "AlternateBase"},
        {17, "NoRole"},
        {18, "ToolTipBase"},
        {19, "ToolTipText"},
        {20, "PlaceholderText"},
    };
    return lookupEnum(colorRoles, v);
}

// Helpers for formatting a character sequences

// Format a special character like '\x0a'
static int formatEscapedNumber(QTextStream &str, ushort value, int base, int width,
                               char prefix = 0)
{
    int length = 1 + width;
    str << '\\';
    if (prefix) {
        str << prefix;
        ++length;
    }
    const auto oldPadChar = str.padChar();
    const auto oldFieldWidth = str.fieldWidth();
    const auto oldFieldAlignment = str.fieldAlignment();
    const auto oldIntegerBase = str.integerBase();
    str.setPadChar(QLatin1Char('0'));
    str.setFieldWidth(width);
    str.setFieldAlignment(QTextStream::AlignRight);
    str.setIntegerBase(base);
    str << value;
    str.setIntegerBase(oldIntegerBase);
    str.setFieldAlignment(oldFieldAlignment);
    str.setFieldWidth(oldFieldWidth);
    str.setPadChar(oldPadChar);
    return length;
}

static int formatSpecialCharacter(QTextStream &str, ushort value)
{
    int length = 0;
    switch (value) {
    case '\\':
        str << "\\\\";
        length += 2;
        break;
    case '\"':
        str << "\\\"";
        length += 2;
        break;
    case '\n':
        str << "\\n\"\n\"";
        length += 5;
        break;
    default:
        break;
    }
    return length;
}

// Format a sequence of characters for C++ with special characters numerically
// escaped (non-raw string literals), wrappped at maxSegmentSize. FormattingTraits
// are used to transform characters into (unsigned) codes, which can be used
// for either normal escapes or Unicode code points as used in Unicode literals.

enum : int { maxSegmentSize = 1024 };

template <Encoding e>
struct FormattingTraits
{
};

template <>
struct FormattingTraits<Encoding::Utf8>
{
    static ushort code(char c) { return uchar(c); }
};

template <>
struct FormattingTraits<Encoding::Unicode>
{
    static ushort code(QChar c) { return c.unicode(); }
};

template <Encoding e, class Iterator>
static void formatStringSequence(QTextStream &str, Iterator it, Iterator end,
                                 const QString &indent,
                                 int escapeIntegerBase, int escapeWidth,
                                 char escapePrefix = 0)
{
    str << '"';
    int length = 0;
    while (it != end) {
        const auto code = FormattingTraits<e>::code(*it);
        if (code >= 0x80) {
            length += formatEscapedNumber(str, code, escapeIntegerBase, escapeWidth, escapePrefix);
        } else if (const int l = formatSpecialCharacter(str, code)) {
            length += l;
        } else if (code != '\r') {
            str << *it;
            ++length;
        }
        ++it;
        if (it != end && length > maxSegmentSize) {
            str << "\"\n" << indent << indent << '"';
            length = 0;
        }
    }
    str << '"';
}

void _formatString(QTextStream &str, const QString &value, const QString &indent,
                   bool qString)
{
    switch (encoding) {
    // Special characters as 3 digit octal escapes (u8"\303\234mlaut")
    case Encoding::Utf8: {
        if (qString && _language == Language::Cpp)
            str << "QString::fromUtf8(";
        const QByteArray utf8 = value.toUtf8();
        formatStringSequence<Encoding::Utf8>(str, utf8.cbegin(), utf8.cend(), indent,
                                             8, 3);
        if (qString && _language == Language::Cpp)
            str << ')';
    }
        break;
    // Special characters as 4 digit hex Unicode points (u8"\u00dcmlaut")
    case Encoding::Unicode:
        str << 'u'; // Python Unicode literal (would be UTF-16 in C++)
        formatStringSequence<Encoding::Unicode>(str, value.cbegin(), value.cend(), indent,
                                                16, 4, 'u');
        break;
    }
}

QTextStream &operator<<(QTextStream &str, const repeat &r)
{
    for (int i = 0; i < r.m_count; ++i)
        str << r.m_char;
    return str;
}

startFunctionDefinition1::startFunctionDefinition1(const char *name, const QString &parameterType,
                                                   const QString &parameterName,
                                                   const QString &indent,
                                                   const char *returnType) :
    m_name(name), m_parameterType(parameterType), m_parameterName(parameterName),
    m_indent(indent), m_return(returnType)
{
}

QTextStream &operator<<(QTextStream &str, const startFunctionDefinition1 &f)
{
    switch (language()) {
    case Language::Cpp:
        str << (f.m_return ? f.m_return : "void") << ' ' << f.m_name << '('
            << f.m_parameterType;
        if (f.m_parameterType.cend()->isLetter())
            str << ' ';
        str << f.m_parameterName << ')' << '\n' << f.m_indent << "{\n";
        break;
    case Language::Python:
        str << "def " << f.m_name << "(self, " << f.m_parameterName << "):\n";
        break;
    }
    return str;
}

endFunctionDefinition::endFunctionDefinition(const char *name) : m_name(name)
{
}

QTextStream &operator<<(QTextStream &str, const endFunctionDefinition &f)
{
    switch (language()) {
    case Language::Cpp:
        str << "} // " << f.m_name << "\n\n";
        break;
    case Language::Python:
        str << "# " << f.m_name << "\n\n";
        break;
    }
    return str;
}

void _formatStackVariable(QTextStream &str, const char *className, QStringView varName,
                          bool withInitParameters)
{
    switch (language()) {
    case Language::Cpp:
        str << className << ' ' << varName;
        if (withInitParameters)
            str << '(';
        break;
    case Language::Python:
        str << varName << " = " << className << '(';
        if (!withInitParameters)
            str << ')';
        break;
    }
}

void formatConnection(QTextStream &str, const SignalSlot &sender, const SignalSlot &receiver)
{
    switch (language()) {
    case Language::Cpp:
        str << "QObject::connect(" << sender.name << ", SIGNAL("<< sender.signature
            << "), " << receiver.name << ", SLOT("<< receiver.signature << "))";
        break;
    case Language::Python:
        str << sender.name << '.'
            << sender.signature.leftRef(sender.signature.indexOf(QLatin1Char('(')))
            << ".connect(" << receiver.name << '.'
            << receiver.signature.leftRef(receiver.signature.indexOf(QLatin1Char('(')))
            << ')';
        break;
    }
}

QString boolValue(bool v)
{
    switch (language()) {
    case Language::Cpp:
        return v ? cppTrue : cppFalse;
    case Language::Python:
        return v ? QStringLiteral("True") : QStringLiteral("False");
    }
    Q_UNREACHABLE();
}

static inline QString dot() { return QStringLiteral("."); }

QString enumValue(const QString &value)
{
    if (language() == Language::Cpp || !value.contains(cppQualifier))
        return value;
    QString fixed = value;
    fixed.replace(cppQualifier, dot());
    return fixed;
}

} // namespace language
