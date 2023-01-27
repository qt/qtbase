// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PARSER_H
#define PARSER_H

#include <qiodevice.h>
#include <qlist.h>
#include <qbytearray.h>

struct Function
{
    QString className;
    QString functionName;
    QString functionParameters;
};

struct Point
{
    QString name;
    QString parameters;
};

struct Replace
{
    QString in;
    QString out;
};

struct LineNumber
{
    qsizetype begin;
    qsizetype end;
    int line;
};

struct Parser
{
    Parser(const QString &provider)
        : m_provider(provider)
    {

    }

    void addIncludeDirs(const QStringList &list)
    {
        m_includeDirs.append(list);
    }
    QString resolveInclude(const QString &filename);
    void addIncludesRecursive(const QString &filename, QStringList &includes);
    QStringList findEnumValues(const QString &name, const QStringList &includes);

    void parseParamReplace(const QString &data, qsizetype offset, const QString &name);
    void parseInstrument(const QString &data, qsizetype offset);
    void parsePoint(const QString &data, qsizetype offset);
    void parsePrefix(const QString &data, qsizetype offset);
    void parseMetadata(const QString &data, qsizetype offset, const QStringList &includes);
    int lineNumber(qsizetype offset) const;

    void parse(QIODevice &input, const QString &name);
    void write(QIODevice &input) const;
    bool isEmpty() const
    {
        return m_functions.isEmpty() && m_points.isEmpty();
    }

    QList<Function> m_functions;
    QList<Point> m_points;
    QList<Replace> m_replaces;
    QList<QString> m_prefixes;
    QList<QString> m_metadata;
    QList<LineNumber> m_offsets;
    QList<QString> m_includeDirs;
    QString m_provider;
};

#endif // PARSER_H
