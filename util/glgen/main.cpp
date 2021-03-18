/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utilities of the Qt Toolkit.
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
