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

    virtual int type() const;

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
