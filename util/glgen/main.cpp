// Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
