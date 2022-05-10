// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QMULTITOUCH_MAC_P_H
#define QMULTITOUCH_MAC_P_H

#include <QtCore/qglobal.h>
#include <qpa/qwindowsysteminterface.h>
#include <qhash.h>
#include <QtCore>
#include <QtGui/qpointingdevice.h>

#include <QtCore/private/qcore_mac_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(NSTouch);
QT_FORWARD_DECLARE_OBJC_ENUM(NSTouchPhase, unsigned long);

QT_BEGIN_NAMESPACE

class QCocoaTouch
{
    public:
        static QList<QWindowSystemInterface::TouchPoint> getCurrentTouchPointList(NSEvent *event, bool acceptSingleTouch);
        static void setMouseInDraggingState(bool inDraggingState);
        static QPointingDevice *getTouchDevice(QInputDevice::DeviceType type, quint64 id);

    private:
        static QHash<quint64, QPointingDevice*> _touchDevices;
        static QHash<qint64, QCocoaTouch*> _currentTouches;
        static QPointF _screenReferencePos;
        static QPointF _trackpadReferencePos;
        static int _idAssignmentCount;
        static int _touchCount;
        static bool _updateInternalStateOnly;

        QWindowSystemInterface::TouchPoint _touchPoint;
        qint64 _identity;

        QCocoaTouch(NSTouch *nstouch);
        ~QCocoaTouch();

        void updateTouchData(NSTouch *nstouch, NSTouchPhase phase);
        static QCocoaTouch *findQCocoaTouch(NSTouch *nstouch);
        static QEventPoint::State toTouchPointState(NSTouchPhase nsState);
};

QT_END_NAMESPACE

#endif // QMULTITOUCH_MAC_P_H

