// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef GENERATOR_H
#define GENERATOR_H

#include <QTextStream>
#include <QStringList>

#include "nfa.h"

class LineStream
{
private:
    struct SharedStream
    {
        int ref;
        QTextStream *stream;
    };

public:
    LineStream(QTextStream *textStream)
    {
        shared = new SharedStream;
        shared->ref = 1;
        shared->stream = textStream;
    }
    LineStream(const LineStream &other)
    {
        shared = other.shared;
        shared->ref++;
    }
    LineStream &operator=(const LineStream &other)
    {
        if (this == &other)
            return *this;
        LineStream copy(other); // keep refcount up
        qSwap(*shared, *other.shared);
        return *this;
    }
    ~LineStream()
    {
        if (!--shared->ref) {
            (*shared->stream) << Qt::endl;
            delete shared;
        }
    }

    template <typename T>
    LineStream &operator<<(const T &value)
    { (*shared->stream) << value; return *this; }

    SharedStream *shared;
};

class CodeBlock
{
public:
    inline CodeBlock() { stream.setString(&output, QIODevice::WriteOnly); }

    inline void indent() { indentStr += QLatin1String("    "); }
    inline void outdent() { indentStr.remove(0, 4); }

    template <typename T>
    LineStream operator<<(const T &value)
    { stream << indentStr; stream << value; return LineStream(&stream); }

    inline void addNewLine() { stream << Qt::endl; }

    inline QString toString() const { stream.flush(); return output; }

private:
    QString output;
    mutable QTextStream stream;
    QString indentStr;
};

class Function
{
public:
    inline Function(const QString &returnType, const QString &name)
        : rtype(returnType), fname(name), iline(false), cnst(false) {}
    inline Function() : iline(false), cnst(false) {}

    inline void setName(const QString &name) { fname = name; }
    inline QString name() const { return fname; }

    inline void setInline(bool i) { iline = i; }
    inline bool isInline() const { return iline; }

    inline void setReturnType(const QString &type) { rtype = type; }
    inline QString returnType() const { return rtype; }

    inline void addBody(const QString &_body) { body += _body; }
    inline void addBody(const CodeBlock &block) { body += block.toString(); }
    inline bool hasBody() const { return !body.isEmpty(); }

    inline void setConst(bool konst) { cnst = konst; }
    inline bool isConst() const { return cnst; }

    void printDeclaration(CodeBlock &block, const QString &funcNamePrefix = QString()) const;
    QString definition() const;

private:
    QString signature(const QString &funcNamePrefix = QString()) const;

    QString rtype;
    QString fname;
    QString body;
    bool iline;
    bool cnst;
};

class Class
{
public:
    enum Access { PublicMember, ProtectedMember, PrivateMember };

    inline Class(const QString &name) : cname(name) {}

    inline void setName(const QString &name) { cname = name; }
    inline QString name() const { return cname; }

    inline void addMember(Access access, const QString &name)
    { sections[access].variables.append(name); }
    inline void addMember(Access access, const Function &func)
    { sections[access].functions.append(func); }

    void addConstructor(Access access, const QString &body, const QString &args = QString());
    inline void addConstructor(Access access, const CodeBlock &body, const QString &args = QString())
    { addConstructor(access, body.toString(), args); }

    QString declaration() const;
    QString definition() const;

private:
    QString cname;
    struct Section
    {
        QList<Function> functions;
        QStringList variables;
        QList<Function> constructors;

        inline bool isEmpty() const
        { return functions.isEmpty() && variables.isEmpty() && constructors.isEmpty(); }

        void printDeclaration(const Class *klass, CodeBlock &block) const;
        QString definition(const Class *klass) const;
    };

    Section sections[3];
};

class Generator
{
public:
    Generator(const DFA &dfa, const Config &config);

    QString generate();

private:
    void generateTransitions(CodeBlock &body, const TransitionMap &transitions);
    bool isSingleReferencedFinalState(int i) const;

    DFA dfa;
    Config cfg;
    InputType minInput;
    InputType maxInput;
    QHash<int, int> backReferenceMap;
    QString headerFileName;
public:
    struct TransitionSequence
    {
        inline TransitionSequence() : first(-1), last(-1), transition(-1) {}
        InputType first;
        InputType last;
        int transition;
        QString testFunction;
    };
private:
    QList<TransitionSequence> charFunctionRanges;
};

#endif
