/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
    explicit LogWidget(QWidget *parent = 0);
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
