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

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtCore/qstring.h>

QT_FORWARD_DECLARE_CLASS(QTextStream)

enum class Language { Cpp, Python };

namespace language {

Language language();
void setLanguage(Language);

extern QString derefPointer;
extern QString nullPtr;
extern QString operatorNew;
extern QString qtQualifier;
extern QString qualifier;
extern QString self;
extern QString eol;
extern QString emptyString;

extern QString cppQualifier;
extern QString cppTrue;
extern QString cppFalse;

// Base class for streamable objects with one QStringView parameter
class StringViewStreamable
{
public:
    StringViewStreamable(QStringView parameter) : m_parameter(parameter) {}

    QStringView parameter() const { return m_parameter; }

private:
    QStringView m_parameter;
};

class qtConfig : public StringViewStreamable
{
public:
    qtConfig(QStringView name) : StringViewStreamable(name) {}
};

QTextStream &operator<<(QTextStream &str, const qtConfig &c);

class openQtConfig : public StringViewStreamable
{
public:
    openQtConfig(QStringView name) : StringViewStreamable(name) {}
};

QTextStream &operator<<(QTextStream &str, const openQtConfig &c);

class closeQtConfig : public StringViewStreamable
{
public:
    closeQtConfig(QStringView name) : StringViewStreamable(name) {}
};

QTextStream &operator<<(QTextStream &, const closeQtConfig &c);

QString fixClassName(QString className);

const char *toolbarArea(int v);
const char *sizePolicy(int v);
const char *dockWidgetArea(int v);
const char *paletteColorRole(int v);

enum class Encoding { Utf8, Unicode };

void _formatString(QTextStream &str, const QString &value, const QString &indent,
                   bool qString);

template <bool AsQString>
class _string
{
public:
    explicit _string(const QString &value, const QString &indent = QString())
        : m_value(value), m_indent(indent) {}

    void format(QTextStream &str) const
    { _formatString(str, m_value, m_indent, AsQString); }

private:
    const QString &m_value;
    const QString &m_indent;
};

template <bool AsQString>
inline QTextStream &operator<<(QTextStream &str, const language::_string<AsQString> &s)
{
    s.format(str);
    return str;
}

using charliteral = _string<false>;
using qstring = _string<true>;

class repeat {
public:
    explicit repeat(int count, char c) : m_count(count), m_char(c) {}

    friend QTextStream &operator<<(QTextStream &str, const repeat &r);

private:
    const int m_count;
    const char m_char;
};

class startFunctionDefinition1 {
public:
    explicit startFunctionDefinition1(const char *name, const QString &parameterType,
                                      const QString &parameterName,
                                      const QString &indent,
                                      const char *returnType = nullptr);

    friend QTextStream &operator<<(QTextStream &str, const startFunctionDefinition1 &f);
private:
    const char *m_name;
    const QString &m_parameterType;
    const QString &m_parameterName;
    const QString &m_indent;
    const char *m_return;
};

class endFunctionDefinition {
public:
    explicit endFunctionDefinition(const char *name);

    friend QTextStream &operator<<(QTextStream &str, const endFunctionDefinition &f);
private:
    const char *m_name;
};

void _formatStackVariable(QTextStream &str, const char *className, QStringView varName, bool withInitParameters);

template <bool withInitParameters>
class _stackVariable {
public:
    explicit _stackVariable(const char *className, QStringView varName) :
        m_className(className), m_varName(varName) {}

    void format(QTextStream &str) const
    { _formatStackVariable(str, m_className, m_varName, withInitParameters); }

private:
    const char *m_className;
    QStringView m_varName;
    QStringView m_parameters;
};

template <bool withInitParameters>
inline QTextStream &operator<<(QTextStream &str, const _stackVariable<withInitParameters> &s)
{
    s.format(str);
    return str;
}

using stackVariable = _stackVariable<false>;
using stackVariableWithInitParameters = _stackVariable<true>;

struct SignalSlot
{
    QString name;
    QString signature;
    QString className;
};

void formatConnection(QTextStream &str, const SignalSlot &sender, const SignalSlot &receiver);

QString boolValue(bool v);

QString enumValue(const QString &value);

} // namespace language

#endif // LANGUAGE_H
