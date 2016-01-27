/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
#ifndef QIBUSPLATFORMINPUTCONTEXT_H
#define QIBUSPLATFORMINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>

#include <QtCore/qpointer.h>
#include <QtCore/QLocale>
#include <QtDBus/qdbuspendingreply.h>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QWindow>

QT_BEGIN_NAMESPACE

class QIBusPlatformInputContextPrivate;
class QDBusVariant;

class QIBusFilterEventWatcher: public QDBusPendingCallWatcher
{
public:
    explicit QIBusFilterEventWatcher(const QDBusPendingCall &call,
                                     QObject *parent = 0,
                                     QWindow *window = 0,
                                     const Qt::KeyboardModifiers modifiers = 0,
                                     const QVariantList arguments = QVariantList())
    : QDBusPendingCallWatcher(call, parent)
    , m_window(window)
    , m_modifiers(modifiers)
    , m_arguments(arguments)
    {}
    ~QIBusFilterEventWatcher()
    {}

    inline QWindow *window() const { return m_window; }
    inline const Qt::KeyboardModifiers modifiers() const { return m_modifiers; }
    inline const QVariantList arguments() const { return m_arguments; }

private:
    QPointer<QWindow> m_window;
    const Qt::KeyboardModifiers m_modifiers;
    const QVariantList m_arguments;
};

class QIBusPlatformInputContext : public QPlatformInputContext
{
    Q_OBJECT
public:
    QIBusPlatformInputContext();
    ~QIBusPlatformInputContext();

    bool isValid() const Q_DECL_OVERRIDE;
    void setFocusObject(QObject *object) Q_DECL_OVERRIDE;

    void invokeAction(QInputMethod::Action a, int x) Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void commit() Q_DECL_OVERRIDE;
    void update(Qt::InputMethodQueries) Q_DECL_OVERRIDE;
    bool filterEvent(const QEvent *event) Q_DECL_OVERRIDE;
    QLocale locale() const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void commitText(const QDBusVariant &text);
    void updatePreeditText(const QDBusVariant &text, uint cursor_pos, bool visible);
    void cursorRectChanged();
    void deleteSurroundingText(int offset, uint n_chars);
    void surroundingTextRequired();
    void filterEventFinished(QDBusPendingCallWatcher *call);
    void socketChanged(const QString &str);
    void connectToBus();
    void globalEngineChanged(const QString &engine_name);

private:
    QIBusPlatformInputContextPrivate *d;
    bool m_eventFilterUseSynchronousMode;
    QFileSystemWatcher m_socketWatcher;
    QTimer m_timer;

    void connectToContextSignals();
};

QT_END_NAMESPACE

#endif
