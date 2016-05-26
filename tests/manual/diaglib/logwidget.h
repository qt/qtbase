/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
