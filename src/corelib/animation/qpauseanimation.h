/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QPAUSEANIMATION_H
#define QPAUSEANIMATION_H

#include <QtCore/qanimationgroup.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QPauseAnimationPrivate;

class Q_CORE_EXPORT QPauseAnimation : public QAbstractAnimation
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration)
public:
    QPauseAnimation(QObject *parent = nullptr);
    QPauseAnimation(int msecs, QObject *parent = nullptr);
    ~QPauseAnimation();

    int duration() const override;
    void setDuration(int msecs);

protected:
    bool event(QEvent *e) override;
    void updateCurrentTime(int) override;

private:
    Q_DISABLE_COPY(QPauseAnimation)
    Q_DECLARE_PRIVATE(QPauseAnimation)
};

QT_END_NAMESPACE

#endif // QPAUSEANIMATION_H
