/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_PEPPEREVENT_TRANSLATOR_H
#define QT_PEPPEREVENT_TRANSLATOR_H

#ifndef QT_NO_PEPPER_INTEGRATION
#include "ppapi/c/pp_input_event.h"
#include "ppapi/cpp/input_event.h"

#include <QtGui>

class PepperEventTranslator : public QObject
{
Q_OBJECT
public:
    PepperEventTranslator();
    bool processEvent(const pp::InputEvent& event);

    bool processMouseEvent(const pp::MouseInputEvent &event, PP_InputEvent_Type eventType);
    bool processWheelEvent(const pp::WheelInputEvent &event);
    bool processKeyEvent(const pp::KeyboardInputEvent &event, PP_InputEvent_Type eventType);
    bool processCharacterEvent(const pp::KeyboardInputEvent &event);

    Qt::MouseButton translatePepperMouseButton(PP_InputEvent_MouseButton pepperButton);
    Qt::MouseButtons translatePepperMouseModifiers(uint32_t modifier);
    Qt::Key translatePepperKey(uint32_t pepperKey, bool *outAlphanumretic);
    Qt::KeyboardModifiers translatePepperKeyModifiers(uint32_t modifier);
Q_SIGNALS:
    void getWindowAt(const QPoint &point, QWindow **window);
    void getKeyWindow(QWindow **window);
private:
    uint32_t currentPepperKey;
    QPoint currentMouseGlobalPos;
};

#endif

#endif // PEPPEREVENTTRANSLATOR_H
