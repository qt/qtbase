/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef __SUBMARINE__H__
#define __SUBMARINE__H__

//Qt
#include <QtWidgets/QGraphicsTransform>

#include "pixmapitem.h"

class Torpedo;

class SubMarine : public PixmapItem
{
Q_OBJECT
public:
    enum Movement {
       None = 0,
       Left,
       Right
    };
    enum { Type = UserType + 1 };
    SubMarine(int type, const QString &name, int points);

    int points() const;

    void setCurrentDirection(Movement direction);
    enum Movement currentDirection() const;

    void setCurrentSpeed(int speed);
    int currentSpeed() const;

    void launchTorpedo(int speed);
    void destroy();

    virtual int type() const Q_DECL_OVERRIDE;

    QGraphicsRotation *rotation() const { return graphicsRotation; }

signals:
    void subMarineDestroyed();
    void subMarineExecutionFinished();
    void subMarineStateChanged();

private:
    int subType;
    QString subName;
    int subPoints;
    int speed;
    Movement direction;
    QGraphicsRotation *graphicsRotation;
};

#endif //__SUBMARINE__H__
