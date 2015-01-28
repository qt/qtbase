/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: http://www.qt.io/licensing/
**
** This file is part of the utilities of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "codegenerator.h"
#include "legacyspecparser.h"
#include "xmlspecparser.h"

#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cmdParser;

    // flag whether to use legacy or not
    QCommandLineOption legacyOption(QStringList() << "l" << "legacy", "Use legacy parser.");
    cmdParser.addOption(legacyOption);
    cmdParser.process(app);

    SpecParser *parser;

    if (cmdParser.isSet(legacyOption)) {
        parser = new LegacySpecParser();
        parser->setTypeMapFileName(QStringLiteral("gl.tm"));
        parser->setSpecFileName(QStringLiteral("gl.spec"));
    } else {
        parser = new XmlSpecParser();
        parser->setSpecFileName(QStringLiteral("gl.xml"));
    }

    parser->parse();

    CodeGenerator generator;
    generator.setParser(parser);
    generator.generateCoreClasses(QStringLiteral("qopenglversionfunctions"));
    generator.generateExtensionClasses(QStringLiteral("qopenglextensions"));

    delete parser;
    return 0;
}
