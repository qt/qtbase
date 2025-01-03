// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#ifndef SCREENREADER_H
#define SCREENREADER_H

#include <QObject>
#include <QAccessible>
#include <QAccessibleBridge>

/*
    A Simple screen reader for touch-based user interfaces.

    Requires a text-to-speach backend. Currently implemented
    using festival on unix.
*/
class OptionsWidget;
class ScreenReader : public QObject
{
    Q_OBJECT
public:
    explicit ScreenReader(QObject *parent = nullptr);
    ~ScreenReader();

    void setRootObject(QObject *rootObject);
    void setOptionsWidget(OptionsWidget *optionsWidget);
public slots:
    void touchPoint(const QPoint &point);
    void activate();
protected slots:
    void processTouchPoint();
signals:
    void selected(QObject *object);

protected:
    void speak(const QString &text, const QString &voice = QString());
private:
    QAccessibleInterface *m_selectedInterface;
    QAccessibleInterface *m_rootInterface;
    OptionsWidget *m_optionsWidget;
    QPoint m_currentTouchPoint;
    bool m_activateCalled;
};

#endif // SCREENREADER_H
