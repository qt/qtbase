/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "androidjniinput.h"
#include "androidjnimain.h"
#include "qandroidplatformintegration.h"

#include <qpa/qwindowsysteminterface.h>
#include <QTouchEvent>
#include <QPointer>

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
# include <QDebug>
#endif

using namespace QtAndroid;

namespace QtAndroidInput
{
    static jmethodID m_showSoftwareKeyboardMethodID = 0;
    static jmethodID m_resetSoftwareKeyboardMethodID = 0;
    static jmethodID m_hideSoftwareKeyboardMethodID = 0;
    static jmethodID m_isSoftwareKeyboardVisibleMethodID = 0;
    static jmethodID m_updateSelectionMethodID = 0;

    static bool m_ignoreMouseEvents = false;

    static QList<QWindowSystemInterface::TouchPoint> m_touchPoints;

    static QPointer<QWindow> m_mouseGrabber;

    static int m_lastCursorPos = -1;

    void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << ">>> UPDATESELECTION" << selStart << selEnd << candidatesStart << candidatesEnd;
#endif
        if (candidatesStart == -1 && candidatesEnd == -1 && selStart == selEnd) {
            // Qt only gives us position inside the block, so if we move to the
            // same position in another block, the Android keyboard will believe
            // we have not changed position, and be terribly confused.
            if (selStart == m_lastCursorPos) {
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
                qDebug() << ">>> FAKEUPDATESELECTION" << selStart+1;
#endif
                env.jniEnv->CallStaticVoidMethod(applicationClass(), m_updateSelectionMethodID,
                                         selStart+1, selEnd+1, candidatesStart, candidatesEnd);
            }
            m_lastCursorPos = selStart;
        } else {
            m_lastCursorPos = -1;
        }
        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_updateSelectionMethodID,
                                         selStart, selEnd, candidatesStart, candidatesEnd);
    }

    void showSoftwareKeyboard(int left, int top, int width, int height, int inputHints)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(applicationClass(),
                                         m_showSoftwareKeyboardMethodID,
                                         left,
                                         top,
                                         width,
                                         height,
                                         inputHints);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ SHOWSOFTWAREKEYBOARD" << left << top << width << height << inputHints;
#endif
    }

    void resetSoftwareKeyboard()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_resetSoftwareKeyboardMethodID);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ RESETSOFTWAREKEYBOARD";
#endif
    }

    void hideSoftwareKeyboard()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_hideSoftwareKeyboardMethodID);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ HIDESOFTWAREKEYBOARD";
#endif
    }

    bool isSoftwareKeyboardVisible()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return false;

        bool visibility = env.jniEnv->CallStaticBooleanMethod(applicationClass(), m_isSoftwareKeyboardVisibleMethodID);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ ISSOFTWAREKEYBOARDVISIBLE" << visibility;
#endif
        return visibility;
    }


    static void mouseDown(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {
        if (m_ignoreMouseEvents)
            return;

        QPoint globalPos(x,y);
        QWindow *tlw = topLevelWindowAt(globalPos);
        m_mouseGrabber = tlw;
        QPoint localPos = tlw ? (globalPos - tlw->position()) : globalPos;
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::LeftButton));
    }

    static void mouseUp(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {
        QPoint globalPos(x,y);
        QWindow *tlw = m_mouseGrabber.data();
        if (!tlw)
            tlw = topLevelWindowAt(globalPos);
        QPoint localPos = tlw ? (globalPos -tlw->position()) : globalPos;
        QWindowSystemInterface::handleMouseEvent(tlw, localPos, globalPos
                                                , Qt::MouseButtons(Qt::NoButton));
        m_ignoreMouseEvents = false;
        m_mouseGrabber = 0;
    }

    static void mouseMove(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {

        if (m_ignoreMouseEvents)
            return;

        QPoint globalPos(x,y);
        QWindow *tlw = m_mouseGrabber.data();
        if (!tlw)
            tlw = topLevelWindowAt(globalPos);
        QPoint localPos = tlw ? (globalPos-tlw->position()) : globalPos;
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::LeftButton));
    }

    static void longPress(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {
        //### TODO: add proper API for Qt 5.2
        static bool rightMouseFromLongPress = qgetenv("QT_NECESSITAS_COMPATIBILITY_LONG_PRESS").toInt();
        if (!rightMouseFromLongPress)
            return;
        m_ignoreMouseEvents = true;
        QPoint globalPos(x,y);
        QWindow *tlw = topLevelWindowAt(globalPos);
        QPoint localPos = tlw ? (globalPos-tlw->position()) : globalPos;

        // Release left button
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::NoButton));

        // Press right button
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::RightButton));
    }

    static void touchBegin(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/)
    {
        m_touchPoints.clear();
    }

    static void touchAdd(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint id, jint action, jboolean /*primary*/, jint x, jint y, jfloat size, jfloat pressure)
    {
        Qt::TouchPointState state = Qt::TouchPointStationary;
        switch (action) {
        case 0:
            state = Qt::TouchPointPressed;
            break;
        case 1:
            state = Qt::TouchPointMoved;
            break;
        case 2:
            state = Qt::TouchPointStationary;
            break;
        case 3:
            state = Qt::TouchPointReleased;
            break;
        }

        const int dw = desktopWidthPixels();
        const int dh = desktopHeightPixels();
        QWindowSystemInterface::TouchPoint touchPoint;
        touchPoint.id = id;
        touchPoint.pressure = pressure;
        touchPoint.normalPosition = QPointF(double(x / dw), double(y / dh));
        touchPoint.state = state;
        touchPoint.area = QRectF(x - double(dw*size) / 2.0,
                                 y - double(dh*size) / 2.0,
                                 double(dw*size),
                                 double(dh*size));
        m_touchPoints.push_back(touchPoint);
    }

    static void touchEnd(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint /*action*/)
    {
        if (m_touchPoints.isEmpty())
            return;

        QAndroidPlatformIntegration *platformIntegration = QtAndroid::androidPlatformIntegration();
        if (!platformIntegration)
            return;

        QTouchDevice *touchDevice = platformIntegration->touchDevice();
        if (touchDevice == 0) {
            touchDevice = new QTouchDevice;
            touchDevice->setType(QTouchDevice::TouchScreen);
            touchDevice->setCapabilities(QTouchDevice::Position
                                         | QTouchDevice::Area
                                         | QTouchDevice::Pressure
                                         | QTouchDevice::NormalizedPosition);
            QWindowSystemInterface::registerTouchDevice(touchDevice);
            platformIntegration->setTouchDevice(touchDevice);
        }

        QWindow *window = QtAndroid::topLevelWindowAt(m_touchPoints.at(0).area.center().toPoint());
        QWindowSystemInterface::handleTouchEvent(window, touchDevice, m_touchPoints);
    }

    static int mapAndroidKey(int key)
    {
        // 0--9        0x00000007 -- 0x00000010
        if (key >= 0x00000007 && key <= 0x00000010)
            return Qt::Key_0 + key - 0x00000007;

        // A--Z        0x0000001d -- 0x00000036
        if (key >= 0x0000001d && key <= 0x00000036)
            return Qt::Key_A + key - 0x0000001d;

        switch (key) {
            case 0x00000039:
            case 0x0000003a:
                return Qt::Key_Alt;

            case 0x0000004b:
                return Qt::Key_Apostrophe;

            case 0x00000004: // KEYCODE_BACK
                return Qt::Key_Back;

            case 0x00000049:
                return Qt::Key_Backslash;

            case 0x00000005:
                return Qt::Key_Call;

            case 0x0000001b: // KEYCODE_CAMERA
                return Qt::Key_Camera;

            case 0x0000001c:
                return Qt::Key_Clear;

            case 0x00000037:
                return Qt::Key_Comma;

            case 0x00000043: // KEYCODE_DEL
                return Qt::Key_Backspace;

            case 0x00000017: // KEYCODE_DPAD_CENTER
                return Qt::Key_Enter;

            case 0x00000014: // KEYCODE_DPAD_DOWN
                return Qt::Key_Down;

            case 0x00000015: //KEYCODE_DPAD_LEFT
                return Qt::Key_Left;

            case 0x00000016: //KEYCODE_DPAD_RIGHT
                return Qt::Key_Right;

            case 0x00000013: //KEYCODE_DPAD_UP
                return Qt::Key_Up;

            case 0x00000006: //KEYCODE_ENDCALL
                return Qt::Key_Hangup;

            case 0x00000042:
                return Qt::Key_Return;

            case 0x00000041: //KEYCODE_ENVELOPE
                return Qt::Key_LaunchMail;

            case 0x00000046:
                return Qt::Key_Equal;

            case 0x00000040:
                return Qt::Key_Explorer;

            case 0x00000003:
                return Qt::Key_Home;

            case 0x00000047:
                return Qt::Key_BracketLeft;

            case 0x0000005a: // KEYCODE_MEDIA_FAST_FORWARD
                return Qt::Key_AudioForward;

            case 0x00000057:
                return Qt::Key_MediaNext;

            case 0x00000055:
                return Qt::Key_MediaPlay;

            case 0x00000058:
                return Qt::Key_MediaPrevious;

            case 0x00000059: // KEYCODE_MEDIA_REWIND
                return Qt::Key_AudioRewind;

            case 0x00000056:
                return Qt::Key_MediaStop;

            case 0x00000052: //KEYCODE_MENU
                return Qt::Key_Menu;

            case 0x00000045:
                return Qt::Key_Minus;

            case 0x0000005b: // KEYCODE_MUTE
                return Qt::Key_MicMute;

            case 0x0000004e:
                return Qt::Key_NumLock;

            case 0x00000038:
                return Qt::Key_Period;

            case 0x00000051:
                return Qt::Key_Plus;

            case 0x0000001a:
                return Qt::Key_PowerOff;

            case 0x00000048:
                return Qt::Key_BracketRight;

            case 0x00000054:
                return Qt::Key_Search;

            case 0x0000004a:
                return Qt::Key_Semicolon;

            case 0x0000003b:
            case 0x0000003c:
                return Qt::Key_Shift;

            case 0x0000004c:
                return Qt::Key_Slash;

            case 0x00000001:
                return Qt::Key_Left;

            case 0x00000002:
                return Qt::Key_Right;

            case 0x0000003e:
                return Qt::Key_Space;

            case 0x0000003f: // KEYCODE_SYM
                return Qt::Key_Meta;

            case 0x0000003d:
                return Qt::Key_Tab;

            case 0x00000019:
                return Qt::Key_VolumeDown;

            case 0x000000a4: // KEYCODE_VOLUME_MUTE
                return Qt::Key_VolumeMute;

            case 0x00000018:
                return Qt::Key_VolumeUp;

            case 0x00000011: // KEYCODE_STAR
                return Qt::Key_Asterisk;

            case 0x00000012: // KEYCODE_POUND
                return Qt::Key_NumberSign;

            case 0x00000050: // KEYCODE_FOCUS
                return Qt::Key_CameraFocus;

            case 0x00000070: // KEYCODE_FORWARD_DEL
                return Qt::Key_Delete;

            case 0x00000080: // KEYCODE_MEDIA_CLOSE
                return Qt::Key_Close;

            case 0x00000081: // KEYCODE_MEDIA_EJECT
                return Qt::Key_Eject;

            case 0x00000082: // KEYCODE_MEDIA_RECORD
                return Qt::Key_MediaRecord;

            case 0x000000b7: // KEYCODE_PROG_RED
                return Qt::Key_Red;

            case 0x000000b8: // KEYCODE_PROG_GREEN
                return Qt::Key_Green;

            case 0x000000b9: // KEYCODE_PROG_YELLOW
                return Qt::Key_Yellow;

            case 0x000000ba: // KEYCODE_PROG_BLUE
                return Qt::Key_Blue;

            case 0x000000a5: // KEYCODE_INFO
                return Qt::Key_Info;

            case 0x000000a6: // KEYCODE_CHANNEL_UP
                return Qt::Key_ChannelUp;

            case 0x000000a7: // KEYCODE_CHANNEL_DOWN
                return Qt::Key_ChannelDown;

            case 0x000000a8: // KEYCODE_ZOOM_IN
                return Qt::Key_ZoomIn;

            case 0x000000a9: // KEYCODE_ZOOM_OUT
                return Qt::Key_ZoomOut;

            case 0x000000ac: // KEYCODE_GUIDE
                return Qt::Key_Guide;

            case 0x000000af: // KEYCODE_CAPTIONS
                return Qt::Key_Subtitle;

            case 0x000000b0: // KEYCODE_SETTINGS
                return Qt::Key_Settings;

            case 0x000000d0: // KEYCODE_CALENDAR
                return Qt::Key_Calendar;

            case 0x000000d1: // KEYCODE_MUSIC
                return Qt::Key_Music;

            case 0x000000d2: // KEYCODE_CALCULATOR
                return Qt::Key_Calculator;

            case 0x00000000: // KEYCODE_UNKNOWN
                return Qt::Key_unknown;

            case 0x00000053: // KEYCODE_NOTIFICATION ?!?!?
            case 0x0000004f: // KEYCODE_HEADSETHOOK ?!?!?
            case 0x00000044: // KEYCODE_GRAVE ?!?!?
                return Qt::Key_Any;

            default:
                return 0;
        }
    }

    static void keyDown(JNIEnv */*env*/, jobject /*thiz*/, jint key, jint unicode, jint modifier)
    {
        Qt::KeyboardModifiers modifiers;
        if (modifier & 1)
            modifiers |= Qt::ShiftModifier;

        if (modifier & 2)
            modifiers |= Qt::AltModifier;

        if (modifier & 4)
            modifiers |= Qt::MetaModifier;

        QWindowSystemInterface::handleKeyEvent(0,
                                               QEvent::KeyPress,
                                               mapAndroidKey(key),
                                               modifiers,
                                               QChar(unicode),
                                               false);
    }

    static void keyUp(JNIEnv */*env*/, jobject /*thiz*/, jint key, jint unicode, jint modifier)
    {
        Qt::KeyboardModifiers modifiers;
        if (modifier & 1)
            modifiers |= Qt::ShiftModifier;

        if (modifier & 2)
            modifiers |= Qt::AltModifier;

        if (modifier & 4)
            modifiers |= Qt::MetaModifier;

        QWindowSystemInterface::handleKeyEvent(0,
                                               QEvent::KeyRelease,
                                               mapAndroidKey(key),
                                               modifiers,
                                               QChar(unicode),
                                               false);
    }

    static void keyboardVisibilityChanged(JNIEnv */*env*/, jobject /*thiz*/, jboolean /*visibility*/)
    {
        QAndroidInputContext *inputContext = QAndroidInputContext::androidInputContext();
        if (inputContext)
            inputContext->emitInputPanelVisibleChanged();
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ KEYBOARDVISIBILITYCHANGED" << inputContext;
#endif
    }

    static JNINativeMethod methods[] = {
        {"touchBegin","(I)V",(void*)touchBegin},
        {"touchAdd","(IIIZIIFF)V",(void*)touchAdd},
        {"touchEnd","(II)V",(void*)touchEnd},
        {"mouseDown", "(III)V", (void *)mouseDown},
        {"mouseUp", "(III)V", (void *)mouseUp},
        {"mouseMove", "(III)V", (void *)mouseMove},
        {"longPress", "(III)V", (void *)longPress},
        {"keyDown", "(III)V", (void *)keyDown},
        {"keyUp", "(III)V", (void *)keyUp},
        {"keyboardVisibilityChanged", "(Z)V", (void *)keyboardVisibilityChanged}
    };

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

    bool registerNatives(JNIEnv *env)
    {
        jclass appClass = QtAndroid::applicationClass();

        if (env->RegisterNatives(appClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
            __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
            return false;
        }

        GET_AND_CHECK_STATIC_METHOD(m_showSoftwareKeyboardMethodID, appClass, "showSoftwareKeyboard", "(IIIII)V");
        GET_AND_CHECK_STATIC_METHOD(m_resetSoftwareKeyboardMethodID, appClass, "resetSoftwareKeyboard", "()V");
        GET_AND_CHECK_STATIC_METHOD(m_hideSoftwareKeyboardMethodID, appClass, "hideSoftwareKeyboard", "()V");
        GET_AND_CHECK_STATIC_METHOD(m_isSoftwareKeyboardVisibleMethodID, appClass, "isSoftwareKeyboardVisible", "()Z");
        GET_AND_CHECK_STATIC_METHOD(m_updateSelectionMethodID, appClass, "updateSelection", "(IIII)V");
        return true;
    }
}
