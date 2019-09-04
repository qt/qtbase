/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "encodingdialog.h"

#if QT_CONFIG(action)
#  include <QAction>
#endif
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#if QT_CONFIG(clipboard)
#  include <QGuiApplication>
#  include <QClipboard>
#endif

#include <QTextStream>

// Helpers for formatting character sequences

// Format a special character like '\x0a'
template <class Int>
static void formatEscapedNumber(QTextStream &str, Int value, int base,
                                int width = 0,char prefix = 0)
{
    str << '\\';
    if (prefix)
        str << prefix;
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
}

template <class Int>
static bool formatSpecialCharacter(QTextStream &str, Int value)
{
    bool result = true;
    switch (value) {
    case '\\':
        str << "\\\\";
        break;
    case '\"':
        str << "\\\"";
        break;
    case '\n':
        str << "\\n";
        break;
    default:
        result = false;
        break;
    }
    return result;
}

// Format a sequence of characters (QChar, ushort (UTF-16), uint (UTF-32)
// or just char (Latin1, Utf-8)) with the help of traits specifying
// how to obtain the code for checking the printable-ness and how to
// stream out the plain ASCII values.

template <EncodingDialog::Encoding>
struct FormattingTraits
{
};

template <>
struct FormattingTraits<EncodingDialog::Unicode>
{
    static ushort code(QChar c) { return c.unicode(); }
    static char toAscii(QChar c) { return c.toLatin1(); }
};

template <>
struct FormattingTraits<EncodingDialog::Utf8>
{
    static ushort code(char c) { return uchar(c); }
    static char toAscii(char c) { return c; }
};

template <>
struct FormattingTraits<EncodingDialog::Utf16>
{
    static ushort code(ushort c) { return c; }
    static char toAscii(ushort c) { return char(c); }
};

template <>
struct FormattingTraits<EncodingDialog::Utf32>
{
    static uint code(uint c) { return c; }
    static char toAscii(uint c) { return char(c); }
};

template <>
struct FormattingTraits<EncodingDialog::Latin1>
{
    static uchar code(char c) { return uchar(c); }
    static char toAscii(char  c) { return c; }
};

static bool isHexDigit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

template <EncodingDialog::Encoding encoding, class Iterator>
static void formatStringSequence(QTextStream &str, Iterator i1, Iterator i2,
                                 int escapeIntegerBase, int escapeWidth,
                                 char escapePrefix = 0)
{
    str << '"';
    bool separateHexEscape = false;
    for (; i1 != i2; ++i1) {
        const auto code = FormattingTraits<encoding>::code(*i1);
        if (code >= 0x80) {
            formatEscapedNumber(str, code, escapeIntegerBase, escapeWidth, escapePrefix);
            separateHexEscape = escapeIntegerBase == 16 && escapeWidth == 0;
        } else {
            if (!formatSpecialCharacter(str, code)) {
                const char c = FormattingTraits<encoding>::toAscii(*i1);
                // For variable width/hex: Terminate the literal to stop digit parsing
                // ("\x12" "34...").
                if (separateHexEscape && isHexDigit(c))
                    str << "\" \"";
                str << c;
            }
            separateHexEscape = false;
        }
    }
    str << '"';
}

static QString encodedString(const QString &value, EncodingDialog::Encoding e)
{
    QString result;
    QTextStream str(&result);
    switch (e) {
    case EncodingDialog::Unicode:
        formatStringSequence<EncodingDialog::Unicode>(str, value.cbegin(), value.cend(),
                                                      16, 4, 'u');
        break;
    case EncodingDialog::Utf8: {
        const QByteArray utf8 = value.toUtf8();
        str << "u8";
        formatStringSequence<EncodingDialog::Utf8>(str, utf8.cbegin(), utf8.cend(),
                                                   8, 3);
    }
        break;
    case EncodingDialog::Utf16: {
        auto utf16 = value.utf16();
        auto utf16End = utf16 + value.size();
        str << 'u';
        formatStringSequence<EncodingDialog::Utf16>(str, utf16, utf16End,
                                                    16, 0, 'x');
    }
        break;
    case EncodingDialog::Utf32: {
        auto utf32 = value.toUcs4();
        str << 'U';
        formatStringSequence<EncodingDialog::Utf32>(str, utf32.cbegin(), utf32.cend(),
                                                    16, 0, 'x');
    }
        break;
    case EncodingDialog::Latin1: {
        const QByteArray latin1 = value.toLatin1();
        formatStringSequence<EncodingDialog::Latin1>(str, latin1.cbegin(), latin1.cend(),
                                                     16, 0, 'x');
    }
        break;
    case EncodingDialog::EncodingCount:
        break;
    }
    return result;
}

// Dialog helpers

static const char *encodingLabels[]
{
    QT_TRANSLATE_NOOP("EncodingDialog", "Unicode:"),
    QT_TRANSLATE_NOOP("EncodingDialog", "UTF-8:"),
    QT_TRANSLATE_NOOP("EncodingDialog", "UTF-16:"),
    QT_TRANSLATE_NOOP("EncodingDialog", "UTF-32:"),
    QT_TRANSLATE_NOOP("EncodingDialog", "Latin1:")
};

static const char *encodingToolTips[]
{
    QT_TRANSLATE_NOOP("EncodingDialog", "Unicode points for use with any encoding (C++, Python)"),
    QT_TRANSLATE_NOOP("EncodingDialog", "QString::fromUtf8()"),
    QT_TRANSLATE_NOOP("EncodingDialog", "wchar_t on Windows, char16_t everywhere"),
    QT_TRANSLATE_NOOP("EncodingDialog", "wchar_t on Unix (Ucs4)"),
    QT_TRANSLATE_NOOP("EncodingDialog", "QLatin1String")
};

// A read-only line edit with a tool button to copy the contents
class DisplayLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit DisplayLineEdit(const QIcon &icon, QWidget *parent = nullptr);

public slots:
    void copyAll();
};

DisplayLineEdit::DisplayLineEdit(const QIcon &icon, QWidget *parent) :
    QLineEdit(parent)
{
    setReadOnly(true);
#if QT_CONFIG(clipboard) && QT_CONFIG(action)
    auto copyAction = addAction(icon, QLineEdit::TrailingPosition);
    connect(copyAction, &QAction::triggered, this, &DisplayLineEdit::copyAll);
#endif
}

void DisplayLineEdit::copyAll()
{
#if QT_CONFIG(clipboard)
    QGuiApplication::clipboard()->setText(text());
#endif
}

static void addFormLayoutRow(QFormLayout *formLayout, const QString &text,
                             QWidget *w, const QString &toolTip)
{
    auto label = new QLabel(text);
    label->setToolTip(toolTip);
    w->setToolTip(toolTip);
    label->setBuddy(w);
    formLayout->addRow(label, w);
}

EncodingDialog::EncodingDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Encodings"));

    auto formLayout = new QFormLayout;
    auto sourceLineEdit = new QLineEdit(this);
    sourceLineEdit->setClearButtonEnabled(true);
    connect(sourceLineEdit, &QLineEdit::textChanged, this, &EncodingDialog::textChanged);

    addFormLayoutRow(formLayout, tr("&Source:"), sourceLineEdit, tr("Enter text"));

    const auto copyIcon = QIcon::fromTheme(QLatin1String("edit-copy"),
                                           QIcon(QLatin1String(":/images/editcopy")));
    for (int i = 0; i < EncodingCount; ++i) {
        m_lineEdits[i] = new DisplayLineEdit(copyIcon, this);
        addFormLayoutRow(formLayout, tr(encodingLabels[i]),
                         m_lineEdits[i], tr(encodingToolTips[i]));
    }

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void EncodingDialog::textChanged(const QString &t)
{
    if (t.isEmpty()) {
        for (auto lineEdit : m_lineEdits)
            lineEdit->clear();
    } else {
         for (int i = 0; i < EncodingCount; ++i)
             m_lineEdits[i]->setText(encodedString(t, static_cast<Encoding>(i)));
    }
}

#include "encodingdialog.moc"
