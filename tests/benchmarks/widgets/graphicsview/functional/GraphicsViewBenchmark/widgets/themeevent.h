// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef __THEMEEVENT_H__
#define __THEMEEVENT_H__

#include <QEvent>
#include <QString>

static QEvent::Type ThemeEventType = (QEvent::Type) 1010;

class ThemeEvent : public QEvent
{
public:
    explicit ThemeEvent(const QString &newTheme, QEvent::Type type = ThemeEventType );
    ~ThemeEvent();

public:
    inline QString getTheme() { return m_theme; }

private:
    QString m_theme;
};


#endif /* __THEMEEVENT_H__ */
