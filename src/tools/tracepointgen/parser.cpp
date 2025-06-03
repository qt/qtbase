// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tracepointgen.h"
#include "parser.h"
#include <qtextstream.h>
#include <qregularexpression.h>
#include <qfileinfo.h>

static void removeOffsetRange(qsizetype begin, qsizetype end, QList<LineNumber> &offsets)
{
    qsizetype count = end - begin;
    qsizetype i = 0;
    DEBUGPRINTF2(printf("tracepointgen: removeOffsetRange: %llu %llu\n", begin, end));
    while (i < offsets.size()) {
        LineNumber &cur = offsets[i];
        if (begin > cur.end) {
            i++;
        } else if (begin >= cur.begin && begin <= cur.end) {
            cur.end = begin;
            i++;
        } else if (begin < cur.begin && end > cur.end) {
            offsets.remove(i);
            DEBUGPRINTF2(printf("tracepointgen: removeOffsetRange: %llu, %llu, %d\n", cur.begin, cur.end, cur.line));
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

static void simplifyData(QString &data)
{
    qsizetype offset = data.indexOf(QStringLiteral("//"));
    while (offset >= 0) {
        qsizetype endOfLine = data.indexOf(QLatin1Char('\n'), offset);
        if (endOfLine == -1)
            endOfLine = data.length();
        data.remove(offset, endOfLine - offset);
        offset = data.indexOf(QStringLiteral("//"), offset);
    }
    offset = data.indexOf(QStringLiteral("/*"));
    while (offset >= 0) {
        qsizetype endOfComment = data.indexOf(QStringLiteral("*/"), offset);
        if (endOfComment == -1)
            break;
        data.remove(offset, endOfComment - offset + 2);
        offset = data.indexOf(QStringLiteral("/*"), offset);
    }
    offset = 0;
    qsizetype end = 0;
    while (findSpaceRange(data, offset, end))
        data.remove(offset, end - offset);
}

static QString preprocessMetadata(const QString &in)
{
    DEBUGPRINTF(printf("in: %s\n", qPrintable(in)));
    QList<QString> lines = in.split(QLatin1Char('\\'));
    QString out;
    for (int i = 0; i < lines.size(); i++) {
        QString l = lines.at(i).simplified();
        DEBUGPRINTF(printf("line: %s\n", qPrintable(l)));
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
    DEBUGPRINTF(printf("out: %s\n", qPrintable(out)));
    return out;
}

int Parser::lineNumber(qsizetype offset) const
{
    DEBUGPRINTF(printf("tracepointgen: lineNumber: offset %llu, line count: %llu\n", offset , m_offsets.size()));
    for (const auto line : m_offsets) {
        DEBUGPRINTF(printf("tracepointgen: lineNumber: %llu %llu %d\n", line.begin, line.end, line.line));
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
        panic("Syntax error in Q_TRACE_PARAM_REPLACE at file %s, line %llu", qPrintable(name), lineNumber(offset));
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
        m_prefixes.push_back(preprocessMetadata(prefix));
}

QStringList Parser::findEnumValues(const QString &name, const QStringList &includes)
{
    QStringList split = name.split(QStringLiteral("::"));
    QString enumName = split.last();
    DEBUGPRINTF(printf("searching for %s\n", qPrintable(name)));
    QStringList ret;
    for (const QString &filename : includes) {
        QFile input(filename);
        if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
            DEBUGPRINTF(printf("Cannot open '%s' for reading: %s\n",
                                qPrintable(filename), qPrintable(input.errorString())));
            return ret;
        }
        QString data;
        QTextStream stream(&input);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            data += line + QLatin1Char('\n');
        }
        simplifyData(data);

        int pos = 0;
        bool valid = true;
        for (int i = 0; i < split.size() - 1; i++) {
            QRegularExpression macro(QStringLiteral("(struct|class|namespace) +([A-Za-z0-9_]*)? +([A-Za-z0-9]*;?)"));
            QRegularExpressionMatchIterator m = macro.globalMatch(data);
            bool found = false;
            while (m.hasNext() && !found) {
                QRegularExpressionMatch match = m.next();
                QString n = match.captured(2);
                if (!n.endsWith(QLatin1Char(';')) && n == split[i] && match.capturedStart(2) > pos) {
                    pos = match.capturedStart(2);
                    found = true;
                    break;
                }
                if (match.hasCaptured(3)) {
                    n = match.captured(3);
                    if (!n.endsWith(QLatin1Char(';')) && n == split[i] && match.capturedStart(3) > pos) {
                        pos = match.capturedStart(3);
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                valid = false;
                break;
            }
        }

        if (valid) {
            QRegularExpression macro(QStringLiteral("enum +([A-Za-z0-9_]*)"));
            QRegularExpressionMatchIterator m = macro.globalMatch(data);
            while (m.hasNext()) {
                QRegularExpressionMatch match = m.next();

                if (match.capturedStart() < pos)
                    continue;

                QString n = match.captured(1);

                if (n == enumName) {
                    DEBUGPRINTF(printf("Found enum: %s\n", qPrintable(n)));
                    int begin = data.indexOf(QLatin1Char('{'), match.capturedEnd());
                    int end = data.indexOf(QLatin1Char('}'), begin);
                    QString block = data.mid(begin + 1, end - begin - 1);
                    const QStringList enums = block.split(QLatin1Char('\n'));
                    for (const auto &e : enums) {
                        const auto trimmed = e.trimmed();
                        if (!trimmed.isEmpty() && !trimmed.startsWith(QLatin1Char('#')))
                            ret << trimmed;
                    }

                    return ret;
                }
            }
        }
    }
    return ret;
}

struct EnumNameValue
{
    QString name;
    QString valueStr;
    int value;
};

static QList<EnumNameValue> enumsToValues(const QStringList &values)
{
    int cur = 0;
    QList<EnumNameValue> ret;
    for (const QString &value : values) {
        EnumNameValue r;
        if (value.contains(QLatin1Char('='))) {
            size_t offset = value.indexOf(QLatin1Char('='));
            r.name = value.left(offset).trimmed();
            QString val = value.right(value.length() - offset - 1).trimmed();
            if (val.endsWith(QLatin1Char(',')))
                val = val.left(val.length() - 1);
            bool valid = false;
            int integer = val.toInt(&valid);
            if (!valid)
                integer = val.toInt(&valid, 16);
            if (valid) {
                cur = r.value = integer;
                ret << r;
            } else {
                auto iter = std::find_if(ret.begin(), ret.end(), [&val](const EnumNameValue &elem){
                    return elem.name == val;
                });
                if (iter != ret.end()) {
                    cur = r.value = iter->value;
                    ret << r;
                } else {
                    DEBUGPRINTF(printf("Invalid value: %s %s\n", qPrintable(r.name), qPrintable(value)));
                }
            }
        } else {
            if (value.endsWith(QLatin1Char(',')))
                r.name = value.left(value.length() - 1);
            else
                r.name = value;
            r.value = ++cur;
            ret << r;
        }
    }
    return ret;
}

void Parser::parseMetadata(const QString &data, qsizetype offset, const QStringList &includes)
{
    qsizetype beginOfProvider = data.indexOf(QLatin1Char('('), offset);
    qsizetype endOfProvider = data.indexOf(QLatin1Char(','), beginOfProvider);
    QString metadata;
    QString provider = data.mid(beginOfProvider + 1, endOfProvider - beginOfProvider - 1).simplified();
    if (provider != m_provider)
        return;

    qsizetype endOfPoint = data.indexOf(QLatin1Char(')'), endOfProvider + 1);
    metadata = data.mid(endOfProvider + 1, endOfPoint - endOfProvider - 1).simplified();

    DEBUGPRINTF(printf("tracepointgen: metadata: %s", qPrintable(metadata)));

    QString preprocessed = preprocessMetadata(metadata);

    DEBUGPRINTF2(printf("preprocessed %s\n", qPrintable(preprocessed)));

    QRegularExpression macro(QStringLiteral("([A-Z]*) ?{ ?([A-Za-z0-9=_,. ]*) ?} ?([A-Za-z0-9_:]*) ?;"));
    QRegularExpressionMatchIterator i = macro.globalMatch(preprocessed);
    qsizetype prev = 0;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString values = match.captured(2).trimmed();
        int cur = match.capturedStart();
        if (cur > prev)
            m_metadata.append(preprocessed.mid(prev, cur - prev));

        prev = match.capturedEnd() + 1;
        DEBUGPRINTF2(printf("values: %s\n", qPrintable(values)));
        if (values.isEmpty() || values.startsWith(QStringLiteral("AUTO"))) {
            values.replace(QLatin1Char('\n'), QLatin1Char(' '));
            QStringList ranges;
            if (values.contains(QStringLiteral("RANGE"))) {
                QRegularExpression rangeMacro(QStringLiteral("RANGE +([A-Za-z0-9_]*) +... +([A-Za-z0-9_]*)"));
                QRegularExpressionMatchIterator r = rangeMacro.globalMatch(values);
                while (r.hasNext()) {
                    QRegularExpressionMatch rm = r.next();
                    ranges << rm.captured(1);
                    ranges << rm.captured(2);
                    DEBUGPRINTF2(printf("range: %s ... %s\n", qPrintable(rm.captured(1)), qPrintable(rm.captured(2))));
                }
            }

            const auto enumOrFlag = match.captured(1);
            const auto name = match.captured(3);
            const bool flags = enumOrFlag == QStringLiteral("FLAGS");

            QStringList values = findEnumValues(name, includes);
            if (values.isEmpty()) {
                if (flags && name.endsWith(QLatin1Char('s')))
                    values = findEnumValues(name.left(name.length() - 1), includes);
                if (values.isEmpty()) {
                    DEBUGPRINTF(printf("Unable to find values for %s\n", qPrintable(name)));
                }
            }
            if (!values.isEmpty()) {
                auto moreValues = enumsToValues(values);
                if (ranges.size()) {
                    for (int i = 0; i < ranges.size() / 2; i++) {
                        bool rangeFound = false;
                        for (auto &v : moreValues) {
                            if (v.name == ranges[2 * i]) {
                                rangeFound = true;
                                QString rangeEnd = ranges[2 * i + 1];
                                auto iter = std::find_if(moreValues.begin(), moreValues.end(), [&rangeEnd](const EnumNameValue &elem){
                                    return elem.name == rangeEnd;
                                });
                                if (iter != moreValues.end())
                                    v.valueStr = QStringLiteral("RANGE(%1, %2 ... %3)").arg(v.name).arg(v.value).arg(iter->value);
                                else
                                    panic("Unable to find range end: %s\n", qPrintable(rangeEnd));
                                break;
                            }
                        }
                        if (rangeFound == false)
                            panic("Unable to find range begin: %s\n", qPrintable(ranges[2 * i]));
                    }
                }
                std::sort(moreValues.begin(), moreValues.end(), [](const EnumNameValue &a, const EnumNameValue &b) {
                    return a.value < b.value;
                });
                values.clear();
                int prevValue = std::as_const(moreValues).front().value;
                for (const auto &v : std::as_const(moreValues)) {
                    QString a;
                    if (v.valueStr.isNull()) {
                        if (v.value == prevValue + 1 && !flags)
                            a = v.name;
                        else
                            a = QStringLiteral("%1 = %2").arg(v.name).arg(v.value);
                        prevValue = v.value;
                    } else {
                        a = v.valueStr;
                    }
                    values << a;
                }

                metadata = QStringLiteral("%1 {\n %2 \n} %3;").arg(enumOrFlag).arg(values.join(QStringLiteral(",\n"))).arg(name);
                if (!m_metadata.contains(metadata))
                    m_metadata.append(metadata);
            }
        } else {
            if (!m_metadata.contains(match.captured()))
                m_metadata.append(match.captured());
        }
    }
    if (prev < preprocessed.length())
        m_metadata.append(preprocessed.mid(prev, preprocessed.length() - prev));
}

QString Parser::resolveInclude(const QString &filename)
{
    QFileInfo info(filename);
    if (info.exists())
        return info.absoluteFilePath();
    for (const QString &sp : std::as_const(m_includeDirs)) {
        info = QFileInfo(sp + QLatin1Char('/') + filename);
        if (info.exists())
            return info.absoluteFilePath();
    }
    return {};
}

void Parser::addIncludesRecursive(const QString &filename, QList<QString> &includes)
{
    QFileInfo info(filename);
    DEBUGPRINTF(printf("check include: %s\n", qPrintable(filename)));
    QFile input(filename);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        DEBUGPRINTF(printf("Cannot open '%s' for reading: %s\n",
                            qPrintable(filename), qPrintable(input.errorString())));
        return;
    }
    QString data;
    QTextStream stream(&input);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        data += line + QLatin1Char(QLatin1Char('\n'));
    }

    QRegularExpression includeMacro(QStringLiteral("#include [\"<]([A-Za-z0-9_./]*.h)[\">]"));
    QRegularExpressionMatchIterator i = includeMacro.globalMatch(data);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString filename = match.captured(1);

        QString rinc = filename;
        if (filename.startsWith(QStringLiteral("../"))) {
            QFileInfo info2(info.absolutePath() + QLatin1Char('/') + filename);
            if (!info2.exists()) {
                DEBUGPRINTF(printf("unable to find %s\n", qPrintable(filename)));
                continue;
            }
            rinc = info2.absoluteFilePath();
            filename = info2.fileName();
        }

        // only search possible qt headers
        if (filename.startsWith(QLatin1Char('q'), Qt::CaseInsensitive)) {
            QString resolved = resolveInclude(rinc);
            if (!resolved.isEmpty() && !includes.contains(resolved)) {
                includes.push_back(resolved);
                addIncludesRecursive(resolved, includes);
            }
        }
    }
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

    QStringList includes;

    QRegularExpression includeMacro(QStringLiteral("#include [\"<]([A-Za-z0-9_./]*.h)[\">]"));
    QRegularExpressionMatchIterator i = includeMacro.globalMatch(data);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        const QString filename = match.captured(1);
        // only search possible qt headers
        if (filename.startsWith(QLatin1Char('q'), Qt::CaseInsensitive)) {
            const QString resolved = resolveInclude(filename);
            if (!resolved.isEmpty() && !includes.contains(resolved)) {
                includes.push_back(resolved);
                addIncludesRecursive(resolved, includes);
            }
        }
    }

    QRegularExpression traceMacro(QStringLiteral("Q_TRACE_([A-Z_]*)"));
    i = traceMacro.globalMatch(data);
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
        else if (macroType == QStringLiteral("METADATA"))
            parseMetadata(data, match.capturedEnd(), includes);
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
        for (const auto &prefix : m_prefixes)
            out << prefix << "\n";
        out << QStringLiteral("}\n");
    }
    for (const auto &m : m_metadata)
        out << m << "\n";
    for (const auto &func : m_functions) {
        out << func.className << "_" << func.functionName << "_entry(" << func.functionParameters << ")\n";
        out << func.className << "_" << func.functionName << "_exit()\n";
    }
    for (const auto &point : m_points)
        out << point.name << "(" << point.parameters << ")\n";
}
