/****************************************************************************
**
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCOMMANDLINEPARSER_H
#define QCOMMANDLINEPARSER_H

#include <QtCore/qstringlist.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcommandlineoption.h>

QT_REQUIRE_CONFIG(commandlineparser);

QT_BEGIN_NAMESPACE

class QCommandLineParserPrivate;
class QCoreApplication;

class Q_CORE_EXPORT QCommandLineParser
{
    Q_DECLARE_TR_FUNCTIONS(QCommandLineParser)
public:
    QCommandLineParser();
    ~QCommandLineParser();

    enum SingleDashWordOptionMode {
        ParseAsCompactedShortOptions,
        ParseAsLongOptions
    };
    void setSingleDashWordOptionMode(SingleDashWordOptionMode parsingMode);

    enum OptionsAfterPositionalArgumentsMode {
        ParseAsOptions,
        ParseAsPositionalArguments
    };
    void setOptionsAfterPositionalArgumentsMode(OptionsAfterPositionalArgumentsMode mode);

    bool addOption(const QCommandLineOption &commandLineOption);
    bool addOptions(const QList<QCommandLineOption> &options);

    QCommandLineOption addVersionOption();
    QCommandLineOption addHelpOption();
    void setApplicationDescription(const QString &description);
    QString applicationDescription() const;
    void addPositionalArgument(const QString &name, const QString &description, const QString &syntax = QString());
    void clearPositionalArguments();

    void process(const QStringList &arguments);
    void process(const QCoreApplication &app);

    bool parse(const QStringList &arguments);
    QString errorText() const;

    bool isSet(const QString &name) const;
    QString value(const QString &name) const;
    QStringList values(const QString &name) const;

    bool isSet(const QCommandLineOption &option) const;
    QString value(const QCommandLineOption &option) const;
    QStringList values(const QCommandLineOption &option) const;

    QStringList positionalArguments() const;
    QStringList optionNames() const;
    QStringList unknownOptionNames() const;

    Q_NORETURN void showVersion();
    Q_NORETURN void showHelp(int exitCode = 0);
    QString helpText() const;

private:
    Q_DISABLE_COPY(QCommandLineParser)

    QCommandLineParserPrivate * const d;
};

QT_END_NAMESPACE

#endif // QCOMMANDLINEPARSER_H
