/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QTX11EXTRAS_P_H
#define QTX11EXTRAS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qtguiglobal.h>

#include <xcb/xcb.h>

typedef struct _XDisplay Display;

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QX11Info
{
public:
    enum PeekOption {
        PeekDefault = 0,
        PeekFromCachedIndex = 1
    };
    Q_DECLARE_FLAGS(PeekOptions, PeekOption)

    static bool isPlatformX11();

    static int appDpiX(int screen=-1);
    static int appDpiY(int screen=-1);

    static quint32 appRootWindow(int screen=-1);
    static int appScreen();

    static quint32 appTime();
    static quint32 appUserTime();

    static void setAppTime(quint32 time);
    static void setAppUserTime(quint32 time);

    static quint32 getTimestamp();

    static QByteArray nextStartupId();
    static void setNextStartupId(const QByteArray &id);

    static Display *display();
    static xcb_connection_t *connection();

    static bool isCompositingManagerRunning(int screen = -1);

    static qint32 generatePeekerId();
    static bool removePeekerId(qint32 peekerId);
    typedef bool (*PeekerCallback)(xcb_generic_event_t *event, void *peekerData);
    static bool peekEventQueue(PeekerCallback peeker, void *peekerData = nullptr,
                               PeekOptions option = PeekDefault, qint32 peekerId = -1);

private:
    QX11Info();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QX11Info::PeekOptions)

QT_END_NAMESPACE

#endif // QTX11EXTRAS_P_H

