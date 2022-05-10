// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSize>
#include <QString>
#include <QFlags>

class Settings : public QObject {
    Q_OBJECT

public:
    enum Option {
        NoOptions = 0x1,
        UseListItemCache = 0x2,
        UseOpenGL = 0x4,
        OutputFps = 0x8,
        NoResourceUsage = 0x10,
        ManualTest = 0x20
    };
    Q_DECLARE_FLAGS(Options, Option)

    Settings();
    ~Settings();

    const QString &scriptName() const
        { return m_scriptName; }
    void setScriptName(const QString& scriptName)
        { m_scriptName = scriptName; }

    const QString &outputFileName() const
        { return m_outputFileName; }
    void setOutputFileName(const QString& outputFileName)
        { m_outputFileName = outputFileName; }

    int resultFormat() const
        { return m_resultFormat; }
    void setResultFormat(int resultFormat)
        { m_resultFormat = resultFormat; }

    const QSize& size() const
        { return m_size; }
    void setSize(const QSize& size)
        { m_size = size; }

    int angle() const
        { return m_angle; }
    void setAngle(int angle)
        { m_angle = angle; }

    const Options& options() const
        { return m_options; }
    void setOptions(Options options)
        { m_options = options; }

    int listItemCount()
        { return m_listItemCount; }

    void setListItemCount(int items)
        { m_listItemCount = items; }

private:

    QString m_scriptName;
    QString m_outputFileName;
    int m_resultFormat;
    QSize m_size;
    int m_angle;
    int m_listItemCount;
    Options m_options;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Settings::Options)

#endif
