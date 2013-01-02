/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <qfile.h>
#include <qstring.h>
#include <qtextstream.h>

#include "parser.h"

static QTextStream qout(stdout, QIODevice::WriteOnly);
static QTextStream qerr(stderr, QIODevice::WriteOnly);

static void usage()
{
    qerr << "Usage: parse [-report-whitespace-only-chardata] [-report-start-end-entity] <in-file> [<out-file>]\n";
    exit(1);
}

int main(int argc, const char *argv[])
{
    QString file_name;
    QString out_file_name;
    bool report_start_end_entity = false;
    bool report_whitespace_only_chardata = false;

    for (int i = 1 ; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QLatin1String("-report-whitespace-only-chardata"))
            report_whitespace_only_chardata = true;
        else if (arg == QLatin1String("-report-start-end-entity"))
            report_start_end_entity = true;
        else if (file_name.isEmpty())
            file_name = arg;
        else if (out_file_name.isEmpty())
            out_file_name = arg;
        else
            usage();
    }

    if (file_name.isEmpty())
        usage();

    QFile in_file(file_name);
    if (!in_file.open(QIODevice::ReadOnly)) {
        qerr << "Could not open " << file_name << ": " << strerror(errno) << endl;
        return 1;
    }

    if (out_file_name.isEmpty())
        out_file_name = file_name + ".ref";

    QFile _out_file;
    QTextStream _out_stream;
    QTextStream *out_stream;
    if (out_file_name == "-") {
        out_stream = &qout;
    } else {
        _out_file.setFileName(out_file_name);
        if (!_out_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qerr << "Could not open " << out_file_name << ": " << strerror(errno) << endl;
            return 1;
        }
        _out_stream.setDevice(&_out_file);
        out_stream = &_out_stream;
    }

    Parser parser;
    if (report_start_end_entity)
        parser.setFeature("http://trolltech.com/xml/features/report-start-end-entity", true);
    if (report_whitespace_only_chardata)
        parser.setFeature("http://trolltech.com/xml/features/report-whitespace-only-CharData", true);

    parser.parseFile(&in_file);

    out_stream->setCodec("utf8");

    *out_stream << parser.result();

    return 0;
}
