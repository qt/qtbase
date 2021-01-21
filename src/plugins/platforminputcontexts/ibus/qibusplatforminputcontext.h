/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
#ifndef QIBUSPLATFORMINPUTCONTEXT_H
#define QIBUSPLATFORMINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>

#include <QtCore/qpointer.h>
#include <QtCore/QLocale>
#include <QtDBus/qdbuspendingreply.h>
#if QT_CONFIG(filesystemwatcher)
#include <QFileSystemWatcher>
#endif
#include <QTimer>
#include <QWindow>

QT_BEGIN_NAMESPACE

class QIBusPlatformInputContextPrivate;
class QDBusVariant;

class QIBusFilterEventWatcher: public QDBusPendingCallWatcher
{
public:
    explicit QIBusFilterEventWatcher(const QDBusPendingCall &call,
                                     QObject *parent = nullptr,
                                     QWindow *window = nullptr,
                                     const Qt::KeyboardModifiers modifiers = { },
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

    bool isValid() const override;
    void setFocusObject(QObject *object) override;

    void invokeAction(QInputMethod::Action a, int x) override;
    void reset() override;
    void commit() override;
    void update(Qt::InputMethodQueries) override;
    bool filterEvent(const QEvent *event) override;
    QLocale locale() const override;
    bool hasCapability(Capability capability) const override;

public Q_SLOTS:
    void commitText(const QDBusVariant &text);
    void updatePreeditText(const QDBusVariant &text, uint cursor_pos, bool visible);
    void forwardKeyEvent(uint keyval, uint keycode, uint state);
    void cursorRectChanged();
    void deleteSurroundingText(int offset, uint n_chars);
    void surroundingTextRequired();
    void hidePreeditText();
    void showPreeditText();
    void filterEventFinished(QDBusPendingCallWatcher *call);
    void socketChanged(const QString &str);
    void busRegistered(const QString &str);
    void busUnregistered(const QString &str);
    void connectToBus();
    void globalEngineChanged(const QString &engine_name);

private:
    QIBusPlatformInputContextPrivate *d;
    bool m_eventFilterUseSynchronousMode;
#if QT_CONFIG(filesystemwatcher)
    QFileSystemWatcher m_socketWatcher;
#endif
    QTimer m_timer;

    void connectToContextSignals();
};

QT_END_NAMESPACE

#endif
