// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <iostream>
#include <string>
#include <algorithm>

#include <QByteArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>

#include <QColor>

using namespace std;
// REUSE-IgnoreStart
static const char LICENSE_HEADER[] =
R"(
// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
)";
// REUSE-IgnoreEnd
class Printer {
    Q_DISABLE_COPY_MOVE(Printer)
public:
    Printer() = default;

    class Indenter
    {
        Q_DISABLE_COPY_MOVE(Indenter)
        Printer &p;
    public:
        Indenter(Printer &p) : p(p) { p.indent(); }
        ~Indenter() { p.deindent(); }
    };

    ~Printer()
    {
        cout << flush;
    }

    void printLine(const QString &str) const
    {
        printLine(qPrintable(str));
    }

    void printLine(const char *str = nullptr) const
    {
        if (str)
            cout << m_indentString << str << '\n';
        else
            cout << '\n';
    }

    void indent()
    {
        m_indent += 4;
        m_indentString = std::string(m_indent, ' ');
    }

    void deindent()
    {
        m_indent -= 4;
        m_indentString = std::string(m_indent, ' ');
    }

private:
    int m_indent = 0;
    std::string m_indentString;
};

// like QGradientStop, but with a plain int as second field
struct GradientStop
{
    double position;
    int color;

    static bool sortByPosition(GradientStop s1, GradientStop s2)
    {
        return s1.position < s2.position;
    }
};

static void printGradientStops(Printer &p, const QJsonArray &presets)
{
    const QString presetCaseString("case QGradient::%1:");
    const QString presetStopColorString("QColor(%1, %2, %3, %4)");
    const QString presetStopString("QGradientStop(%1, %2), ");

    const auto presetStopToGradientStop = [](const QJsonValue &presetStop)
    {
        const double position = presetStop[QLatin1String("position")].toDouble();
        const int color = presetStop[QLatin1String("color")].toInt();

        return GradientStop{position, color};
    };

    for (const QJsonValue &presetValue : presets) {
        if (!presetValue.isObject())
            continue;

        QJsonObject preset = presetValue.toObject();

        // print the case label
        const QString presetName = preset[QLatin1String("name")].toString();
        p.printLine(presetCaseString.arg(presetName));

        Printer::Indenter i(p);

        // convert the json array of stops to QGradientStop objects
        const QJsonArray stops = preset[QLatin1String("stops")].toArray();
        Q_ASSERT(!stops.isEmpty());

        QList<GradientStop> gradientStops;
        gradientStops.reserve(stops.size());
        std::transform(stops.cbegin(),
                       stops.cend(),
                       std::back_inserter(gradientStops),
                       presetStopToGradientStop);

        // stops should be sorted, but just in case...
        std::sort(gradientStops.begin(), gradientStops.end(),
                  &GradientStop::sortByPosition);

        Q_ASSERT(gradientStops.size() == stops.size());

        // convert to strings
        QString result;
        result.reserve(result.size() + gradientStops.size() * (presetStopString.size() + 20));
        result += "return Q_ARRAY_LITERAL(QGradientStop, ";

        for (const GradientStop &stop : std::as_const(gradientStops)) {
            // gradientgen.js does not output the alpha channel, so hardcode full alpha here
            Q_ASSERT(qAlpha(stop.color) == 0);

            const QString colorString = presetStopColorString
                    .arg(qRed(stop.color))
                    .arg(qGreen(stop.color))
                    .arg(qBlue(stop.color))
                    .arg(255);
            result += presetStopString.arg(stop.position).arg(colorString);
        }

        result.chop(2);
        result += ");";
        p.printLine(result);
    }

    // Add an entry for NumPresets, to silence warnings about switches over enumerations
    p.printLine(presetCaseString.arg("NumPresets"));
    {
        Printer::Indenter i(p);
        p.printLine("Q_UNREACHABLE();");
    }
}

static void printGradientData(Printer &p, const QJsonArray &presets)
{
    const QString formatString("{ { %1, %2, %3, %4 } },");

    for (const QJsonValue &presetValue : presets) {
        if (!presetValue.isObject()) {
            p.printLine("{ { 0, 0, 0, 0 } },");
        } else {
            QJsonObject preset = presetValue.toObject();
            const QJsonValue start = preset[QLatin1String("start")];
            const QJsonValue end = preset[QLatin1String("end")];

            p.printLine(formatString
                        .arg(start[QLatin1String("x")].toDouble())
                        .arg(start[QLatin1String("y")].toDouble())
                        .arg(end[QLatin1String("x")].toDouble())
                        .arg(end[QLatin1String("y")].toDouble()));
        }
    }
}

int main()
{
    QByteArray json;
    while (!cin.eof()) {
        char arr[1024];
        cin.read(arr, sizeof(arr));
        json.append(arr, cin.gcount());
    }

    QJsonParseError error;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(json, &error);
    if (jsonDocument.isNull())
        qFatal("Error: %s at offset %d", qPrintable(error.errorString()), error.offset);

    if (!jsonDocument.isArray())
        qFatal("Error: expected a document with a JSON array");

    QJsonArray presets = jsonDocument.array();

    Printer p;

    p.printLine(LICENSE_HEADER);
    p.printLine();
    p.printLine("// This file is auto-generated by gradientgen. DO NOT EDIT!");
    p.printLine();

    p.printLine("static QList<QGradientStop> qt_preset_gradient_stops(QGradient::Preset preset)");
    p.printLine("{");
    {
        Printer::Indenter i(p);
        p.printLine("Q_ASSERT(preset < QGradient::NumPresets);");
        p.printLine("switch (preset) {");
        printGradientStops(p, presets);
        p.printLine("}");
        p.printLine("Q_UNREACHABLE();");
        p.printLine("return {};");
    }
    p.printLine("}");
    p.printLine();

    p.printLine("static constexpr QGradient::QGradientData qt_preset_gradient_data[] = {");
    {
        Printer::Indenter i(p);
        printGradientData(p, presets);
    }
    p.printLine("};");
    p.printLine();

    p.printLine("static void *qt_preset_gradient_dummy()");
    p.printLine("{");
    {
        Printer::Indenter i(p);
        p.printLine("union {void *p; uint i;};");
        p.printLine("p = 0;");
        p.printLine("i |= uint(QGradient::ObjectMode);");
        p.printLine("return p;");
    }
    p.printLine("}");
}
