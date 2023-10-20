// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "language.h"

#include <QtCore/qtextstream.h>

namespace language {

using namespace Qt::StringLiterals;

static Encoding encoding = Encoding::Utf8;
static Language _language = Language::Cpp;

Language language() { return _language; }

void setLanguage(Language l)
{
    _language = l;
    switch (_language) {
    case Language::Cpp:
        derefPointer = u"->"_s;
        listStart = '{';
        listEnd = '}';
        nullPtr = u"nullptr"_s;
        operatorNew = u"new "_s;
        qtQualifier = u"Qt::"_s;
        qualifier = u"::"_s;
        self = u""_s;  // for testing: change to "this->";
        eol = u";\n"_s;
        emptyString = u"QString()"_s;
        encoding = Encoding::Utf8;
        break;
    case Language::Python:
        derefPointer = u"."_s;
        listStart = '[';
        listEnd = ']';
        nullPtr = u"None"_s;
        operatorNew = u""_s;
        qtQualifier = u"Qt."_s;
        qualifier = u"."_s;
        self = u"self."_s;
        eol = u"\n"_s;
        emptyString = u"\"\""_s;
        encoding = Encoding::Unicode;
        break;
    }
}

QString derefPointer;
char listStart;
char listEnd;
QString nullPtr;
QString operatorNew;
QString qtQualifier;
QString qualifier;
QString self;
QString eol;
QString emptyString;

QString cppQualifier = u"::"_s;
QString cppTrue = u"true"_s;
QString cppFalse = u"false"_s;

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
    QLatin1StringView valueString;
};

template <int N>
QLatin1StringView lookupEnum(const EnumLookup(&array)[N], int value, int defaultIndex = 0)
{
    for (int i = 0; i < N; ++i) {
        if (value == array[i].value)
            return array[i].valueString;
    }
    auto defaultValue = array[defaultIndex].valueString;
    qWarning("uic: Warning: Invalid enumeration value %d, defaulting to %s",
             value, defaultValue.data());
    return defaultValue;
}

QString fixClassName(QString className)
{
    if (language() == Language::Python)
        className.replace(cppQualifier, "_"_L1);
    return className;
}

QLatin1StringView toolbarArea(int v)
{
    static const EnumLookup toolBarAreas[] =
    {
        {0,   "NoToolBarArea"_L1},
        {0x1, "LeftToolBarArea"_L1},
        {0x2, "RightToolBarArea"_L1},
        {0x4, "TopToolBarArea"_L1},
        {0x8, "BottomToolBarArea"_L1},
        {0xf, "AllToolBarAreas"_L1}
    };
    return lookupEnum(toolBarAreas, v);
}

QLatin1StringView sizePolicy(int v)
{
    static const EnumLookup sizePolicies[] =
    {
        {0,   "Fixed"_L1},
        {0x1, "Minimum"_L1},
        {0x4, "Maximum"_L1},
        {0x5, "Preferred"_L1},
        {0x3, "MinimumExpanding"_L1},
        {0x7, "Expanding"_L1},
        {0xD, "Ignored"_L1}
    };
    return lookupEnum(sizePolicies, v, 3);
}

QLatin1StringView dockWidgetArea(int v)
{
    static const EnumLookup dockWidgetAreas[] =
    {
        {0,   "NoDockWidgetArea"_L1},
        {0x1, "LeftDockWidgetArea"_L1},
        {0x2, "RightDockWidgetArea"_L1},
        {0x4, "TopDockWidgetArea"_L1},
        {0x8, "BottomDockWidgetArea"_L1},
        {0xf, "AllDockWidgetAreas"_L1}
    };
    return lookupEnum(dockWidgetAreas, v);
}

QLatin1StringView paletteColorRole(int v)
{
    static const EnumLookup colorRoles[] =
    {
        {0, "WindowText"_L1},
        {1, "Button"_L1},
        {2, "Light"_L1},
        {3, "Midlight"_L1},
        {4, "Dark"_L1},
        {5, "Mid"_L1},
        {6, "Text"_L1},
        {7, "BrightText"_L1},
        {8, "ButtonText"_L1},
        {9, "Base"_L1},
        {10, "Window"_L1},
        {11, "Shadow"_L1},
        {12, "Highlight"_L1},
        {13, "HighlightedText"_L1},
        {14, "Link"_L1},
        {15, "LinkVisited"_L1},
        {16, "AlternateBase"_L1},
        {17, "NoRole"_L1},
        {18, "ToolTipBase"_L1},
        {19, "ToolTipText"_L1},
        {20, "PlaceholderText"_L1},
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
    str.setPadChar(u'0');
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

enum OverloadUse {
    UseOverload,
    UseOverloadWhenNoArguments, // Use overload only when the argument list is empty,
                                // in this case there is no chance of connecting
                                // mismatching T against const T &
    DontUseOverload
};

// Format a member function for a signal slot connection
static void formatMemberFnPtr(QTextStream &str, const SignalSlot &s,
                              OverloadUse useQOverload = DontUseOverload)
{
    const qsizetype parenPos = s.signature.indexOf(u'(');
    Q_ASSERT(parenPos >= 0);
    const auto functionName = QStringView{s.signature}.left(parenPos);

    const auto parameters = QStringView{s.signature}.mid(parenPos + 1,
                                               s.signature.size() - parenPos - 2);
    const bool withOverload = useQOverload == UseOverload ||
            (useQOverload == UseOverloadWhenNoArguments && parameters.isEmpty());

    if (withOverload)
        str << "qOverload<" << parameters << ">(";

    str << '&' << s.className << "::" << functionName;

    if (withOverload)
        str << ')';
}

static void formatMemberFnPtrConnection(QTextStream &str,
                                        const SignalSlot &sender,
                                        const SignalSlot &receiver)
{
    str << "QObject::connect(" << sender.name << ", ";
    formatMemberFnPtr(str, sender);
    str << ", " << receiver.name << ", ";
    formatMemberFnPtr(str, receiver, UseOverloadWhenNoArguments);
    str << ')';
}

static void formatStringBasedConnection(QTextStream &str,
                                        const SignalSlot &sender,
                                        const SignalSlot &receiver)
{
    str << "QObject::connect(" << sender.name << ", SIGNAL("<< sender.signature
        << "), " << receiver.name << ", SLOT(" << receiver.signature << "))";
}

void formatConnection(QTextStream &str, const SignalSlot &sender, const SignalSlot &receiver,
                      ConnectionSyntax connectionSyntax)
{
    switch (language()) {
    case Language::Cpp:
        switch (connectionSyntax) {
        case ConnectionSyntax::MemberFunctionPtr:
            formatMemberFnPtrConnection(str, sender, receiver);
            break;
        case ConnectionSyntax::StringBased:
            formatStringBasedConnection(str, sender, receiver);
            break;
        }
        break;
    case Language::Python: {
        const auto paren = sender.signature.indexOf(u'(');
        auto senderSignature = QStringView{sender.signature};
        str << sender.name << '.' << senderSignature.left(paren);
        // Signals like "QAbstractButton::clicked(checked=false)" require
        // the parameter if it is used.
        if (sender.options.testFlag(SignalSlotOption::Ambiguous)) {
            const QStringView parameters =
                senderSignature.mid(paren + 1, senderSignature.size() - paren - 2);
            if (!parameters.isEmpty() && !parameters.contains(u','))
                str << "[\"" << parameters << "\"]";
        }
        str << ".connect(" << receiver.name << '.'
            << QStringView{receiver.signature}.left(receiver.signature.indexOf(u'('))
            << ')';
    }
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
