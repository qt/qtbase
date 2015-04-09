/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPEPPEREVENTTRANSLATOR_H
#define QPEPPEREVENTTRANSLATOR_H

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPoint>

#include <ppapi/c/pp_input_event.h>
#include <ppapi/cpp/input_event.h>

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_EVENT_KEYBOARD)

class QWindow;
class QPepperEventTranslator : public QObject
{
    Q_OBJECT
public:
    QPepperEventTranslator();
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

#endif // PEPPEREVENTTRANSLATOR_H
