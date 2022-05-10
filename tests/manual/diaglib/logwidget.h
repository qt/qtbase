// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QPlainTextEdit>

// Instantiate as follows: LogFunctionGuard guard(Q_FUNC_INFO)
class LogFunctionGuard {
    Q_DISABLE_COPY(LogFunctionGuard)
public:
    explicit LogFunctionGuard(const char *name);
    ~LogFunctionGuard();

private:
    const char *m_name;
};

class LogWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();

    static LogWidget *instance() { return m_instance; }

    static QString startupMessage();
    static void install();
    static void uninstall();

    static int indent()          { return m_indent; }
    static void setIndent(int i) { m_indent = i; }

    static bool lineNumberingEnabled()          { return m_lineNumberingEnabled; }
    static void setLineNumberingEnabled(bool l) { m_lineNumberingEnabled = l; }

    static bool showMessageType()          { return m_showMessageType; }
    static void setShowMessageType(bool s) { m_showMessageType = s; }

public slots:
    void appendText(const QString &);

private:
    static int m_indent;
    static bool m_lineNumberingEnabled;
    static bool m_showMessageType;
    static LogWidget *m_instance;
};

inline LogFunctionGuard::LogFunctionGuard(const char *name) : m_name(name)
{
    qDebug(">%s", m_name);
    LogWidget::setIndent(LogWidget::indent() + 2);
}

inline LogFunctionGuard::~LogFunctionGuard()
{
    LogWidget::setIndent(LogWidget::indent() - 2);
    qDebug("<%s", m_name);
}

#endif // LOGWIDGET_H
