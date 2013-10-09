/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QLOGGINGCATEGORY_H
#define QLOGGINGCATEGORY_H

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QTracer;
class QTraceGuard;
class QLoggingCategoryPrivate;

class Q_CORE_EXPORT QLoggingCategory
{
    Q_DISABLE_COPY(QLoggingCategory)
public:
    explicit QLoggingCategory(const char *category);
    ~QLoggingCategory();

    bool isEnabled(QtMsgType type) const;
    void setEnabled(QtMsgType type, bool enable);

    bool isDebugEnabled() const { return enabledDebug; }
    bool isWarningEnabled() const { return enabledWarning; }
    bool isCriticalEnabled() const { return enabledCritical; }
    bool isTraceEnabled() const { return enabledTrace; }

    const char *categoryName() const { return name; }

    // allows usage of both factory method and variable in qCX macros
    QLoggingCategory &operator()() { return *this; }

    static QLoggingCategory *defaultCategory();

    typedef void (*CategoryFilter)(QLoggingCategory*);
    static CategoryFilter installFilter(CategoryFilter);

    static void setFilterRules(const QString &rules);

private:
    friend class QLoggingCategoryPrivate;
    friend class QLoggingRegistry;
    friend class QTraceGuard;
    friend class QTracer;

    QLoggingCategoryPrivate *d;
    const char *name;

    bool enabledDebug;
    bool enabledWarning;
    bool enabledCritical;
    bool enabledTrace;
    // reserve space for future use
    bool placeholder1;
    bool placeholder2;
    bool placeholder3;
};

class Q_CORE_EXPORT QTracer
{
    Q_DISABLE_COPY(QTracer)
public:
    QTracer() {}
    virtual ~QTracer() {}

    void addToCategory(QLoggingCategory &category);

    virtual void start() {}
    virtual void end() {}
    virtual void record(int) {}
    virtual void record(const char *) {}
    virtual void record(const QVariant &) {}
};

class Q_CORE_EXPORT QTraceGuard
{
    Q_DISABLE_COPY(QTraceGuard)
public:
    QTraceGuard(QLoggingCategory &category)
    {
        target = category.isTraceEnabled() ? &category : 0;
        if (target)
            start();
    }

    ~QTraceGuard()
    {
        if (target)
            end();
    }

    QTraceGuard &operator<<(int msg);
    QTraceGuard &operator<<(const char *msg);
    QTraceGuard &operator<<(const QVariant &msg);

private:
    void start();
    void end();

    QLoggingCategory *target;
};

#define Q_DECLARE_LOGGING_CATEGORY(name) \
    extern QLoggingCategory &name();

// relies on QLoggingCategory(QString) being thread safe!
#define Q_LOGGING_CATEGORY(name, string) \
    QLoggingCategory &name() \
    { \
        static QLoggingCategory category(string); \
        return category; \
    }

#define qCDebug(category) \
    for (bool enabled = category().isDebugEnabled(); enabled; enabled = false) \
        QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, category().categoryName()).debug()
#define qCWarning(category) \
    for (bool enabled = category().isWarningEnabled(); enabled; enabled = false) \
        QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, category().categoryName()).warning()
#define qCCritical(category) \
    for (bool enabled = category().isCriticalEnabled(); enabled; enabled = false) \
        QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, category().categoryName()).critical()
#define qCTrace(category) \
    for (bool enabled = category.isTraceEnabled(); enabled; enabled = false) \
        QTraceGuard(category)


#define Q_TRACE_GUARD_NAME_HELPER(line) qTraceGuard ## line
#define Q_TRACE_GUARD_NAME(line) Q_TRACE_GUARD_NAME_HELPER(line)

#define qCTraceGuard(category) \
    QTraceGuard Q_TRACE_GUARD_NAME(__LINE__)(category);


#if defined(QT_NO_DEBUG_OUTPUT)
#  undef qCDebug
#  define qCDebug(category) QT_NO_QDEBUG_MACRO()
#endif
#if defined(QT_NO_WARNING_OUTPUT)
#  undef qCWarning
#  define qCWarning(category) QT_NO_QWARNING_MACRO()
#endif

QT_END_NAMESPACE

#endif // QLOGGINGCATEGORY_H
