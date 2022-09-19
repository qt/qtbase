// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tracepointgen.h"
#include "parser.h"
#include <qtextstream.h>
#include <qregularexpression.h>


static void removeOffsetRange(qsizetype begin, qsizetype end, QList<LineNumber> &offsets)
{
    qsizetype count = end - begin;
    qsizetype i = 0;
    DEBUGPRINTF2(printf("tracepointgen: removeOffsetRange: %d %d\n", begin, end));
    while (i < offsets.size()) {
        LineNumber &cur = offsets[i];
        if (begin > cur.end) {
            i++;
        } else if (begin >= cur.begin && begin <= cur.end) {
            cur.end = begin;
            i++;
        } else if (begin < cur.begin && end > cur.end) {
            offsets.remove(i);
            DEBUGPRINTF2(printf("tracepointgen: removeOffsetRange: %d, %d, %d\n", cur.begin, cur.end, cur.line));
        } else if (end >= cur.begin && end <= cur.end) {
            cur.begin = begin;
            cur.end -= count;
            i++;
        } else if (end < cur.begin) {
            cur.begin -= count;
            cur.end -= count;
            i++;
        }
    }
}

static bool findSpaceRange(const QString &data, qsizetype &offset, qsizetype &end) {
    qsizetype cur = data.indexOf(QLatin1Char(' '), offset);
    if (cur >= 0) {
        qsizetype i = cur + 1;
        while (data.constData()[i] == QLatin1Char(' ')) i++;
        if (i - cur > 1) {
            offset = cur;
            end = i - 1;
            return true;
        }
        cur = data.indexOf(QLatin1Char(' '), cur + 1);
    }
    return false;
}

static void simplifyData(QString &data, QList<LineNumber> &offsets)
{
    qsizetype offset = data.indexOf(QStringLiteral("//"));
    while (offset >= 0) {
        qsizetype endOfLine = data.indexOf(QLatin1Char('\n'), offset);
        if (endOfLine == -1)
            endOfLine = data.length();
        removeOffsetRange(offset, endOfLine, offsets);
        data.remove(offset, endOfLine - offset);
        offset = data.indexOf(QStringLiteral("//"), offset);
    }
    offset = data.indexOf(QStringLiteral("/*"));
    while (offset >= 0) {
        qsizetype endOfComment = data.indexOf(QStringLiteral("*/"), offset);
        if (endOfComment == -1)
            break;
        removeOffsetRange(offset, endOfComment + 2, offsets);
        data.remove(offset, endOfComment - offset + 2);
        offset = data.indexOf(QStringLiteral("/*"), offset);
    }
    offset = 0;
    qsizetype end = 0;
    data.replace(QLatin1Char('\n'), QLatin1Char(' '));
    while (findSpaceRange(data, offset, end)) {
        removeOffsetRange(offset, end, offsets);
        data.remove(offset, end - offset);
    }
}

static QString preprocessPrefix(const QString &in)
{
    DEBUGPRINTF(printf("prefix: %s\n", qPrintable(in)));
    QList<QString> lines = in.split(QLatin1Char('\\'));
    QString out;
    for (int i = 0; i < lines.size(); i++) {
        QString l = lines.at(i).simplified();
        DEBUGPRINTF(printf("prefix line: %s\n", qPrintable(l)));
        if (l.length() < 2)
            continue;
        if (l.startsWith(QStringLiteral("\"")))
            l = l.right(l.length() - 1);
        if (l.endsWith(QStringLiteral("\"")))
            l = l.left(l.length() - 1);
        l = l.simplified();

        if (l.length() > 1) {
            if (out.size() > 0)
                out.append(QLatin1Char('\n'));
            out.append(l);
        }
    }
    DEBUGPRINTF(printf("prefix out: %s\n", qPrintable(out)));
    return out;
}

int Parser::lineNumber(qsizetype offset) const
{
    DEBUGPRINTF(printf("tracepointgen: lineNumber: offset %u, line count: %u\n", offset , m_offsets.size()));
    for (auto line : m_offsets) {
        DEBUGPRINTF(printf("tracepointgen: lineNumber: %d %d %d\n", line.begin, line.end, line.line));
        if (offset >= line.begin && offset <= line.end)
            return line.line;
    }
    return 0;
}

void Parser::parseParamReplace(const QString &data, qsizetype offset, const QString &name)
{
    Replace rep;
    qsizetype beginBrace = data.indexOf(QLatin1Char('('), offset);
    qsizetype endBrace = data.indexOf(QLatin1Char(')'), beginBrace);
    QString params = data.mid(beginBrace + 1, endBrace - beginBrace -1);
    int punc = params.indexOf(QLatin1Char(','));
    if (punc < 0)
        panic("Syntax error in Q_TRACE_PARAM_REPLACE at file %s, line %d", qPrintable(name), lineNumber(offset));
    rep.in = params.left(punc).simplified();
    rep.out = params.right(params.length() - punc - 1).simplified();
    if (rep.in.endsWith(QLatin1Char('*')) || rep.out.endsWith(QLatin1Char(']')))
        rep.out.append(QLatin1Char(' '));
    DEBUGPRINTF(printf("tracepointgen: replace: %s with %s\n", qPrintable(rep.in), qPrintable(rep.out)));
    m_replaces.push_back(rep);
}

void Parser::parseInstrument(const QString &data, qsizetype offset)
{
    qsizetype beginOfProvider = data.indexOf(QLatin1Char('('), offset);
    qsizetype endOfProvider = data.indexOf(QLatin1Char(')'), beginOfProvider);
    Function func;
    QString provider = data.mid(beginOfProvider + 1, endOfProvider - beginOfProvider - 1).simplified();
    if (provider != m_provider)
        return;

    qsizetype classMarker = data.indexOf(QStringLiteral("::"), endOfProvider);
    qsizetype beginOfFunctionMarker = data.indexOf(QLatin1Char('{'), classMarker);
    QString begin = data.mid(endOfProvider + 1, classMarker - endOfProvider - 1);
    QString end = data.mid(classMarker + 2, beginOfFunctionMarker - classMarker - 2);
    int spaceIndex = begin.lastIndexOf(QLatin1Char(' '));
    if (spaceIndex == -1)
        func.className = begin;
    else
        func.className = begin.mid(spaceIndex + 1, begin.length() - spaceIndex - 1);
    qsizetype braceIndex = end.indexOf(QLatin1Char('('));
    spaceIndex = end.indexOf(QLatin1Char(' '));
    if (spaceIndex < braceIndex)
        func.functionName = end.left(spaceIndex).simplified();
    else
        func.functionName = end.left(braceIndex).simplified();

    qsizetype lastBraceIndex = end.lastIndexOf(QLatin1Char(')'));
    func.functionParameters = end.mid(braceIndex + 1, lastBraceIndex - braceIndex - 1).simplified();

    DEBUGPRINTF(printf("tracepointgen: %s(%s)\n", qPrintable(func.functionName), qPrintable(func.functionParameters)));

    m_functions.push_back(func);
}

void Parser::parsePoint(const QString &data, qsizetype offset)
{
    qsizetype beginOfProvider = data.indexOf(QLatin1Char('('), offset);
    qsizetype endOfProvider = data.indexOf(QLatin1Char(','), beginOfProvider);
    Point point;
    QString provider = data.mid(beginOfProvider + 1, endOfProvider - beginOfProvider - 1).simplified();
    if (provider != m_provider)
        return;

    qsizetype endOfPoint = data.indexOf(QLatin1Char(','), endOfProvider + 1);
    qsizetype endOfPoint2 = data.indexOf(QLatin1Char(')'), endOfProvider + 1);
    bool params = true;
    if (endOfPoint == -1 || endOfPoint2 < endOfPoint) {
        endOfPoint = endOfPoint2;
        params = false;
    }
    point.name = data.mid(endOfProvider + 1, endOfPoint - endOfProvider - 1).simplified();
    if (params) {
        int endOfParams = data.indexOf(QLatin1Char(')'), endOfPoint);
        point.parameters = data.mid(endOfPoint + 1, endOfParams - endOfPoint - 1).simplified();
    }

    DEBUGPRINTF(printf("tracepointgen: %s(%s)\n", qPrintable(point.name), qPrintable(point.parameters)));

    m_points.push_back(point);
}

void Parser::parsePrefix(const QString &data, qsizetype offset)
{
    qsizetype beginOfProvider = data.indexOf(QLatin1Char('('), offset);
    qsizetype endOfProvider = data.indexOf(QLatin1Char(','), beginOfProvider);
    QString prefix;
    QString provider = data.mid(beginOfProvider + 1, endOfProvider - beginOfProvider - 1).simplified();
    if (provider != m_provider)
        return;

    qsizetype endOfPoint = data.indexOf(QLatin1Char(')'), endOfProvider + 1);
    prefix = data.mid(endOfProvider + 1, endOfPoint - endOfProvider - 1).simplified();

    DEBUGPRINTF(printf("tracepointgen: prefix: %s\n", qPrintable(prefix)));

    if (!m_prefixes.contains(prefix))
        m_prefixes.push_back(preprocessPrefix(prefix));
}

void Parser::parse(QIODevice &input, const QString &name)
{
    QString data;
    QTextStream stream(&input);
    int lineNumber = 1;
    qsizetype prev = 0;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        m_offsets.push_back({prev, prev + line.length(), lineNumber++});
        prev += line.length() + 1;
        data += line + QLatin1Char(QLatin1Char('\n'));
    }

    simplifyData(data, m_offsets);

    QRegularExpression traceMacro(QStringLiteral("Q_TRACE_([A-Z_]*)"));
    QRegularExpressionMatchIterator i = traceMacro.globalMatch(data);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        QString macroType = match.captured(1);
        if (macroType == QStringLiteral("PARAM_REPLACE"))
            parseParamReplace(data, match.capturedEnd(), name);
        else if (macroType == QStringLiteral("INSTRUMENT"))
            parseInstrument(data, match.capturedEnd());
        else if (macroType == QStringLiteral("POINT"))
            parsePoint(data, match.capturedEnd());
        else if (macroType == QStringLiteral("PREFIX"))
            parsePrefix(data, match.capturedEnd());
    }

    for (auto &func : m_functions) {
        for (auto &rep : m_replaces)
            func.functionParameters.replace(rep.in, rep.out);
    }
}

void Parser::write(QIODevice &input) const
{
    QTextStream out(&input);
    if (m_prefixes.size() > 0) {
        out << QStringLiteral("{\n");
        for (auto prefix : m_prefixes)
            out << prefix << "\n";
        out << QStringLiteral("}\n");
    }
    for (auto func : m_functions) {
        out << func.className << "_" << func.functionName << "_entry(" << func.functionParameters << ")\n";
        out << func.className << "_" << func.functionName << "_exit()\n";
    }
    for (auto point : m_points)
        out << point.name << "(" << point.parameters << ")\n";
}
