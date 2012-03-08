/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QByteArray>
#include <QString>
#include <QVarLengthArray>
#include <QFile>
#include <QProcess>
#include <QMetaObject>
#include <QList>
#include <QRegExp>
#include <QCoreApplication>
#include <QLibraryInfo>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "qdbusconnection.h"    // for the Export* flags

// copied from dbus-protocol.h:
static const char docTypeHeader[] =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n";

// in qdbusxmlgenerator.cpp
QT_BEGIN_NAMESPACE
extern Q_DBUS_EXPORT QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                                       const QMetaObject *base, int flags);
QT_END_NAMESPACE

#define PROGRAMNAME     "qdbuscpp2xml"
#define PROGRAMVERSION  "0.1"
#define PROGRAMCOPYRIGHT "Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)."

static QString outputFile;
static int flags;

static const char help[] =
    "Usage: " PROGRAMNAME " [options...] [files...]\n"
    "Parses the C++ source or header file containing a QObject-derived class and\n"
    "produces the D-Bus Introspection XML."
    "\n"
    "Options:\n"
    "  -p|-s|-m       Only parse scriptable Properties, Signals and Methods (slots)\n"
    "  -P|-S|-M       Parse all Properties, Signals and Methods (slots)\n"
    "  -a             Output all scriptable contents (equivalent to -psm)\n"
    "  -A             Output all contents (equivalent to -PSM)\n"
    "  -o <filename>  Write the output to file <filename>\n"
    "  -h             Show this information\n"
    "  -V             Show the program version and quit.\n"
    "\n";

class MocParser
{
    void parseError();
    QByteArray readLine();
    void loadIntData(uint *&data);
    void loadStringData(char *&stringdata);

    QIODevice *input;
    const char *filename;
    int lineNumber;
public:
    ~MocParser();
    void parse(const char *filename, QIODevice *input, int lineNumber = 0);

    QList<QMetaObject> objects;
};

void MocParser::parseError()
{
    fprintf(stderr, PROGRAMNAME ": error parsing input file '%s' line %d \n", filename, lineNumber);
    exit(1);
}

QByteArray MocParser::readLine()
{
    ++lineNumber;
    return input->readLine();
}

void MocParser::loadIntData(uint *&data)
{
    data = 0;                   // initialise
    QVarLengthArray<uint> array;
    QRegExp rx(QLatin1String("(\\d+|0x[0-9abcdef]+)"), Qt::CaseInsensitive);

    while (!input->atEnd()) {
        QString line = QLatin1String(readLine());
        int pos = line.indexOf(QLatin1String("//"));
        if (pos != -1)
            line.truncate(pos); // drop comments

        if (line == QLatin1String("};\n")) {
            // end of data
            data = new uint[array.count()];
            memcpy(data, array.data(), array.count() * sizeof(*data));
            return;
        }

        pos = 0;
        while ((pos = rx.indexIn(line, pos)) != -1) {
            QString num = rx.cap(1);
            if (num.startsWith(QLatin1String("0x")))
                array.append(num.mid(2).toUInt(0, 16));
            else
                array.append(num.toUInt());
            pos += rx.matchedLength();
        }
    }

    parseError();
}

void MocParser::loadStringData(char *&stringdata)
{
    stringdata = 0;
    QVarLengthArray<char, 1024> array;

    while (!input->atEnd()) {
        QByteArray line = readLine();
        if (line == "};\n") {
            // end of data
            stringdata = new char[array.count()];
            memcpy(stringdata, array.data(), array.count() * sizeof(*stringdata));
            return;
        }

        int start = line.indexOf('"');
        if (start == -1)
            parseError();

        int len = line.length() - 1;
        line.truncate(len);     // drop ending \n
        if (line.at(len - 1) != '"')
            parseError();

        --len;
        ++start;
        for ( ; start < len; ++start)
            if (line.at(start) == '\\') {
                // parse escaped sequence
                ++start;
                if (start == len)
                    parseError();

                QChar c(QLatin1Char(line.at(start)));
                if (!c.isDigit()) {
                    switch (c.toLatin1()) {
                    case 'a':
                        array.append('\a');
                        break;
                    case 'b':
                        array.append('\b');
                        break;
                    case 'f':
                        array.append('\f');
                        break;
                    case 'n':
                        array.append('\n');
                        break;
                    case 'r':
                        array.append('\r');
                        break;
                    case 't':
                        array.append('\t');
                        break;
                    case 'v':
                        array.append('\v');
                        break;
                    case '\\':
                    case '?':
                    case '\'':
                    case '"':
                        array.append(c.toLatin1());
                        break;

                    case 'x':
                        if (start + 2 <= len)
                            parseError();
                        array.append(char(line.mid(start + 1, 2).toInt(0, 16)));
                        break;

                    default:
                        array.append(c.toLatin1());
                        fprintf(stderr, PROGRAMNAME ": warning: invalid escape sequence '\\%c' found in input",
                                c.toLatin1());
                    }
                } else {
                    // octal
                    QRegExp octal(QLatin1String("([0-7]+)"));
                    if (octal.indexIn(QLatin1String(line), start) == -1)
                        parseError();
                    array.append(char(octal.cap(1).toInt(0, 8)));
                }
            } else {
                array.append(line.at(start));
            }
    }

    parseError();
}

void MocParser::parse(const char *fname, QIODevice *io, int lineNum)
{
    filename = fname;
    input = io;
    lineNumber = lineNum;

    while (!input->atEnd()) {
        QByteArray line = readLine();
        if (line.startsWith("static const uint qt_meta_data_")) {
            // start of new class data
            uint *data;
            loadIntData(data);

            // find the start of the string data
            do {
                line = readLine();
                if (input->atEnd())
                    parseError();
            } while (!line.startsWith("static const char qt_meta_stringdata_"));

            char *stringdata;
            loadStringData(stringdata);

            QMetaObject mo;
            mo.d.superdata = &QObject::staticMetaObject;
            mo.d.stringdata = stringdata;
            mo.d.data = data;
            mo.d.extradata = 0;
            objects.append(mo);
        }
    }

    fname = 0;
    input = 0;
}

MocParser::~MocParser()
{
    foreach (QMetaObject mo, objects) {
        delete const_cast<char *>(mo.d.stringdata);
        delete const_cast<uint *>(mo.d.data);
    }
}

static void showHelp()
{
    printf("%s", help);
    exit(0);
}

static void showVersion()
{
    printf("%s version %s\n", PROGRAMNAME, PROGRAMVERSION);
    printf("D-Bus QObject-to-XML converter\n");
    exit(0);
}

static void parseCmdLine(QStringList &arguments)
{
    for (int i = 1; i < arguments.count(); ++i) {
        const QString arg = arguments.at(i);

        if (arg == QLatin1String("--help"))
            showHelp();

        if (!arg.startsWith(QLatin1Char('-')))
            continue;

        char c = arg.count() == 2 ? arg.at(1).toLatin1() : char(0);
        switch (c) {
        case 'P':
            flags |= QDBusConnection::ExportNonScriptableProperties;
            // fall through
        case 'p':
            flags |= QDBusConnection::ExportScriptableProperties;
            break;

        case 'S':
            flags |= QDBusConnection::ExportNonScriptableSignals;
            // fall through
        case 's':
            flags |= QDBusConnection::ExportScriptableSignals;
            break;

        case 'M':
            flags |= QDBusConnection::ExportNonScriptableSlots;
            // fall through
        case 'm':
            flags |= QDBusConnection::ExportScriptableSlots;
            break;

        case 'A':
            flags |= QDBusConnection::ExportNonScriptableContents;
            // fall through
        case 'a':
            flags |= QDBusConnection::ExportScriptableContents;
            break;

        case 'o':
            if (arguments.count() < i + 2 || arguments.at(i + 1).startsWith(QLatin1Char('-'))) {
                printf("-o expects a filename\n");
                exit(1);
            }
            outputFile = arguments.takeAt(i + 1);
            break;

        case 'h':
        case '?':
            showHelp();
            break;

        case 'V':
            showVersion();
            break;

        default:
            printf("unknown option: \"%s\"\n", qPrintable(arg));
            exit(1);
        }
    }

    if (flags == 0)
        flags = QDBusConnection::ExportScriptableContents
                | QDBusConnection::ExportNonScriptableContents;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    MocParser parser;
    parseCmdLine(args);

    for (int i = 1; i < args.count(); ++i) {
        const QString arg = args.at(i);
        if (arg.startsWith(QLatin1Char('-')))
            continue;

        QFile f(arg);
        if (!f.open(QIODevice::ReadOnly|QIODevice::Text)) {
            fprintf(stderr, PROGRAMNAME ": could not open '%s': %s\n",
                    qPrintable(arg), qPrintable(f.errorString()));
            return 1;
        }

        f.readLine();

        QByteArray line = f.readLine();
        if (line.contains("Meta object code from reading C++ file"))
            // this is a moc-generated file
            parser.parse(argv[i], &f, 3);
        else {
            // run moc on this file
            QProcess proc;
            proc.start(QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/moc"), QStringList() << QFile::decodeName(argv[i]), QIODevice::ReadOnly | QIODevice::Text);

            if (!proc.waitForStarted()) {
                fprintf(stderr, PROGRAMNAME ": could not execute moc! Aborting.\n");
                return 1;
            }

            proc.closeWriteChannel();

            if (!proc.waitForFinished() || proc.exitStatus() != QProcess::NormalExit ||
                proc.exitCode() != 0) {
                // output the moc errors:
                fprintf(stderr, "%s", proc.readAllStandardError().constData());
                fprintf(stderr, PROGRAMNAME ": exit code %d from moc. Aborting\n", proc.exitCode());
                return 1;
            }
            fprintf(stderr, "%s", proc.readAllStandardError().constData());

            parser.parse(argv[i], &proc, 1);
        }

        f.close();
    }

    QFile output;
    if (outputFile.isEmpty()) {
        output.open(stdout, QIODevice::WriteOnly);
    } else {
        output.setFileName(outputFile);
        if (!output.open(QIODevice::WriteOnly)) {
            fprintf(stderr, PROGRAMNAME ": could not open output file '%s': %s",
                    qPrintable(outputFile), qPrintable(output.errorString()));
            return 1;
        }
    }

    output.write(docTypeHeader);
    output.write("<node>\n");
    foreach (QMetaObject mo, parser.objects) {
        QString xml = qDBusGenerateMetaObjectXml(QString(), &mo, &QObject::staticMetaObject,
                                                 flags);
        output.write(xml.toLocal8Bit());
    }
    output.write("</node>\n");

    return 0;
}

