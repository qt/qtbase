/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#ifndef QPLATFORMHARDWARECOMPOSITOR_H
#define QPLATFORMHARDWARECOMPOSITOR_H

#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>

QT_BEGIN_NAMESPACE

class QPlatformGraphicsBuffer;

class Q_GUI_EXPORT QPlatformHardwareCompositor : public QObject
{
    Q_OBJECT

public:
    struct Layer {
        Layer();
        QMatrix4x4 transform;
        qreal opacity;
        QRectF subRect;
        QColor color;
        QPlatformGraphicsBuffer *buffer;
    };
    virtual ~QPlatformHardwareCompositor() {}
    virtual bool compose(const QVector<Layer> &layers) = 0;
};

QT_END_NAMESPACE

#endif // QPLATFORMHARDWARECOMPOSITOR_H
