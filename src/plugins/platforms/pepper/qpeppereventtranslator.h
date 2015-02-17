/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef QPEPPEREVENTTRANSLATOR_H
#define QPEPPEREVENTTRANSLATOR_H

#ifndef QT_NO_PEPPER_INTEGRATION

#include <QtCore/qloggingcategory.h>
#include <QtCore/qobject.h>
#include <QtGui/qwindow.h>

#include <ppapi/c/pp_input_event.h>
#include <ppapi/cpp/input_event.h>

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_EVENT_KEYBOARD)

class PepperEventTranslator : public QObject
{
    Q_OBJECT
public:
    PepperEventTranslator();
    bool processEvent(const pp::InputEvent &event);

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
