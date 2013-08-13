/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtablegenerator.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QString>

#include <xkbcommon/xkbcommon.h>

#ifdef XKBCOMMON_0_2_0
#include <xkbcommon_workaround.h>
#endif

//#define DEBUG_GENERATOR

TableGenerator::TableGenerator() : m_state(NoErrors),
    m_systemComposeDir(QString())
{
    initPossibleLocations();
    findComposeFile();
    orderComposeTable();
#ifdef DEBUG_GENERATOR
    printComposeTable();
#endif
}

void TableGenerator::initPossibleLocations()
{
    // AFAICT there is no way to know the exact location
    // of the compose files. It depends on how Xlib was configured
    // on a specific platform. During the "./configure" process
    // xlib generates a config.h file which contains a bunch of defines,
    // including XLOCALEDIR which points to the location of the compose file dir.
    // To add an extra system path use the QTCOMPOSE environment variable
    if (qEnvironmentVariableIsSet("QTCOMPOSE")) {
        m_possibleLocations.append(QString(qgetenv("QTCOMPOSE")));
    }
    m_possibleLocations.append(QStringLiteral("/usr/share/X11/locale"));
    m_possibleLocations.append(QStringLiteral("/usr/lib/X11/locale"));
}

void TableGenerator::findComposeFile()
{
    bool found = false;
    // check if XCOMPOSEFILE points to a Compose file
    if (qEnvironmentVariableIsSet("XCOMPOSEFILE")) {
        QString composeFile(qgetenv("XCOMPOSEFILE"));
        if (composeFile.endsWith(QLatin1String("Compose")))
            found = processFile(composeFile);
        else
            qWarning("Qt Warning: XCOMPOSEFILE doesn't point to a valid Compose file");
#ifdef DEBUG_GENERATOR
        if (found)
            qDebug() << "Using Compose file from: " << composeFile;
#endif
    }

    // check if user’s home directory has a file named .XCompose
    if (!found && cleanState()) {
        QString composeFile = qgetenv("HOME") + QStringLiteral("/.XCompose");
        if (QFile(composeFile).exists())
            found = processFile(composeFile);
#ifdef DEBUG_GENERATOR
        if (found)
            qDebug() << "Using Compose file from: " << composeFile;
#endif
    }

    // check for the system provided compose files
    if (!found && cleanState()) {
        readLocaleMappings();

        if (cleanState()) {

            QString table = m_localeToTable.value(locale().toUpper());
            if (table.isEmpty())
                // no table mappings for the system's locale in the compose.dir
                m_state = UnsupportedLocale;
            else
                found = processFile(systemComposeDir() + QLatin1String("/") + table);
#ifdef DEBUG_GENERATOR
            if (found)
                qDebug() << "Using Compose file from: " <<
                            systemComposeDir() + QLatin1String("/") + table;
#endif
        }
    }

    if (found && m_composeTable.isEmpty())
        m_state = EmptyTable;

    if (!found)
        m_state = MissingComposeFile;
}

bool TableGenerator::findSystemComposeDir()
{
    bool found = false;
    for (int i = 0; i < m_possibleLocations.size(); ++i) {
        QString path = m_possibleLocations.at(i);
        if (QFile(path + QLatin1String("/compose.dir")).exists()) {
            m_systemComposeDir = path;
            found = true;
            break;
        }
    }

    if (!found) {
        // should we ask to report this in the qt bug tracker?
        m_state = UnknownSystemComposeDir;
        qWarning("Qt Warning: Could not find a location of the system's Compose files. "
             "Consider setting the QTCOMPOSE environment variable.");
    }

    return found;
}

QString TableGenerator::systemComposeDir()
{
    if (m_systemComposeDir.isNull()
            && !findSystemComposeDir()) {
        return QLatin1String("$QTCOMPOSE");
    }

    return m_systemComposeDir;
}

QString TableGenerator::locale() const
{
    char *name = setlocale(LC_CTYPE, (char *)0);
    return QLatin1String(name);
}

void TableGenerator::readLocaleMappings()
{
    QFile mappings(systemComposeDir() + QLatin1String("/compose.dir"));
    if (mappings.exists()) {
        mappings.open(QIODevice::ReadOnly);
        QTextStream in(&mappings);
        // formating of compose.dir has some inconsistencies
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.startsWith("#") && line.size() != 0 &&
                    line.at(0).isLower()) {

                QStringList pair = line.split(QRegExp(QLatin1String("\\s+")));
                QString table = pair.at(0);
                if (table.endsWith(QLatin1String(":")))
                    table.remove(table.size() - 1, 1);

                m_localeToTable.insert(pair.at(1).toUpper(), table);
            }
        }
        mappings.close();
    }
}

bool TableGenerator::processFile(QString composeFileName)
{
    QFile composeFile(composeFileName);
    if (composeFile.exists()) {
        composeFile.open(QIODevice::ReadOnly);
        parseComposeFile(&composeFile);
        return true;
    }
    qWarning() << QString(QLatin1String("Qt Warning: Compose file: \"%1\" can't be found"))
                  .arg(composeFile.fileName());
    return false;
}

TableGenerator::~TableGenerator()
{
}

QList<QComposeTableElement> TableGenerator::composeTable() const
{
    return m_composeTable;
}

void TableGenerator::parseComposeFile(QFile *composeFile)
{
#ifdef DEBUG_GENERATOR
    qDebug() << "TableGenerator::parseComposeFile: " << composeFile->fileName();
#endif
    QTextStream in(composeFile);

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith(QLatin1String("<"))) {
            parseKeySequence(line);
        } else if (line.startsWith(QLatin1String("include"))) {
            parseIncludeInstruction(line);
        }
    }

    composeFile->close();
}

void TableGenerator::parseIncludeInstruction(QString line)
{
    // Parse something that looks like:
    // include "/usr/share/X11/locale/en_US.UTF-8/Compose"
    QString quote = QStringLiteral("\"");
    line.remove(0, line.indexOf(quote) + 1);
    line.chop(line.length() - line.indexOf(quote));

    // expand substitutions if present
    line.replace(QLatin1String("%H"), QString(qgetenv("HOME")));
    line.replace(QLatin1String("%L"), locale());
    line.replace(QLatin1String("%S"), systemComposeDir());

    processFile(line);
}

ushort TableGenerator::keysymToUtf8(quint32 sym)
{
    QByteArray chars;
    int bytes;
    chars.resize(8);

#ifdef XKBCOMMON_0_2_0
    if (needWorkaround(sym)) {
        quint32 codepoint;
        if (sym == XKB_KEY_KP_Space)
            codepoint = XKB_KEY_space & 0x7f;
        else
            codepoint = sym & 0x7f;

        bytes = utf32_to_utf8(codepoint, chars.data());
    } else {
        bytes = xkb_keysym_to_utf8(sym, chars.data(), chars.size());
    }
#else
    bytes = xkb_keysym_to_utf8(sym, chars.data(), chars.size());
#endif

    if (bytes == -1)
        qWarning("TableGenerator::keysymToUtf8 - buffer too small");

    chars.resize(bytes-1);

#ifdef DEBUG_GENERATOR
    QTextCodec *codec = QTextCodec::codecForLocale();
    qDebug() << QString("keysym - 0x%1 : utf8 - %2").arg(QString::number(sym, 16))
                                                    .arg(codec->toUnicode(chars));
#endif
    return QString::fromUtf8(chars).at(0).unicode();
}

quint32 TableGenerator::stringToKeysym(QString keysymName)
{
    quint32 keysym;
    QByteArray keysymArray = keysymName.toLatin1();
    const char *name = keysymArray.constData();

    if ((keysym = xkb_keysym_from_name(name, (xkb_keysym_flags)0)) == XKB_KEY_NoSymbol)
        qWarning() << QString("Qt Warning - invalid keysym: %1").arg(keysymName);

    return keysym;
}

void TableGenerator::parseKeySequence(QString line)
{
    // we are interested in the lines with the following format:
    // <Multi_key> <numbersign> <S> : "♬"   U266c # BEAMED SIXTEENTH NOTE
    int keysEnd = line.indexOf(QLatin1String(":"));
    QString keys = line.left(keysEnd).trimmed();

    // find the key sequence
    QString regexp = QStringLiteral("<[^>]+>");
    QRegularExpression reg(regexp);
    QRegularExpressionMatchIterator i = reg.globalMatch(keys);
    QStringList keyList;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(0);
        keyList << word;
    }

    QComposeTableElement elem;
    QString quote = QStringLiteral("\"");
    // find the composed value - strings may be direct text encoded in the locale
    // for which the compose file is to be used, or an escaped octal or hexadecimal
    // character code. Octal codes are specified as "\123" and hexadecimal codes as "\0x123a".
    int composeValueIndex = line.indexOf(quote, keysEnd) + 1;
    const QChar valueType(line.at(composeValueIndex));

    if (valueType == '\\' && line.at(composeValueIndex + 1).isDigit()) {
        // handle octal and hex code values
        QChar detectBase(line.at(composeValueIndex + 2));
        QString codeValue = line.mid(composeValueIndex + 1, line.lastIndexOf(quote) - composeValueIndex - 1);
        if (detectBase == 'x') {
            // hexadecimal character code
            elem.value = keysymToUtf8(codeValue.toUInt(0, 16));
        } else {
            // octal character code
            QString hexStr = QString::number(codeValue.toUInt(0, 8), 16);
            elem.value = keysymToUtf8(hexStr.toUInt(0, 16));
        }
    } else {
        // handle direct text encoded in the locale
        const QChar localeValueType = (valueType == '\\') ? line.at(composeValueIndex + 1) : valueType;
        elem.value = localeValueType.unicode();
    }

    // find the comment
    int commnetIndex = line.lastIndexOf(quote) + 1;
    elem.comment = line.mid(commnetIndex).trimmed();

    // Convert to X11 keysym
    int count = keyList.length();
    for (int i = 0; i < QT_KEYSEQUENCE_MAX_LEN; i++) {
        if (i < count) {
            QString keysym = keyList.at(i);
            keysym.remove(keysym.length() - 1, 1);
            keysym.remove(0, 1);

            if (keysym == QLatin1String("dead_inverted_breve"))
                keysym = QStringLiteral("dead_invertedbreve");
            else if (keysym == QLatin1String("dead_double_grave"))
                keysym = QStringLiteral("dead_doublegrave");

            elem.keys[i] = stringToKeysym(keysym);
        } else {
            elem.keys[i] = 0;
        }
    }
    m_composeTable.append(elem);
}

void TableGenerator::printComposeTable() const
{
    if (composeTable().isEmpty())
        return;

    QString output;
    QComposeTableElement elem;
    QString comma = QStringLiteral(",");
    int tableSize = m_composeTable.size();
    for (int i = 0; i < tableSize; ++i) {
        elem = m_composeTable.at(i);
        output.append(QLatin1String("{ {"));
        for (int j = 0; j < QT_KEYSEQUENCE_MAX_LEN; j++) {
            output.append(QString(QLatin1String("0x%1, ")).arg(QString::number(elem.keys[j],16)));
        }
        // take care of the trailing comma
        if (i == tableSize - 1)
            comma = QStringLiteral("");
        output.append(QString(QLatin1String("}, 0x%1, \"\" }%2 // %3 \n"))
                      .arg(QString::number(elem.value,16))
                      .arg(comma)
                      .arg(elem.comment));
    }

    qDebug() << "output: \n" << output;
}

void TableGenerator::orderComposeTable()
{
    // Stable-sorting to ensure that the item that appeared before the other in the
    // original container will still appear first after the sort. This property is
    // needed to handle the cases when user re-defines already defined key sequence
    qStableSort(m_composeTable.begin(), m_composeTable.end(), Compare());
}

