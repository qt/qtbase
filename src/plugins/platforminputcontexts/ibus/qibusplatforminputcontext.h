/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
