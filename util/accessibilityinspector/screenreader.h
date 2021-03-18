/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
    explicit ScreenReader(QObject *parent = 0);
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
