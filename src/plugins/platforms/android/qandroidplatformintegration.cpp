/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidplatformintegration.h"

#include <QtCore/private/qjni_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QThread>
#include <QOffscreenSurface>

#include <QtEglSupport/private/qeglpbuffer_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformoffscreensurface.h>

#include "androidjnimain.h"
#include "qabstracteventdispatcher.h"
#include "qandroideventdispatcher.h"
#include "qandroidplatformbackingstore.h"
#include "qandroidplatformaccessibility.h"
#include "qandroidplatformclipboard.h"
#include "qandroidplatformforeignwindow.h"
#include "qandroidplatformfontdatabase.h"
#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformopenglwindow.h"
#include "qandroidplatformscreen.h"
#include "qandroidplatformservices.h"
#include "qandroidplatformtheme.h"
#include "qandroidsystemlocale.h"

QT_BEGIN_NAMESPACE

int QAndroidPlatformIntegration::m_defaultGeometryWidth = 320;
int QAndroidPlatformIntegration::m_defaultGeometryHeight = 455;
int QAndroidPlatformIntegration::m_defaultScreenWidth = 320;
int QAndroidPlatformIntegration::m_defaultScreenHeight = 455;
int QAndroidPlatformIntegration::m_defaultPhysicalSizeWidth = 50;
int QAndroidPlatformIntegration::m_defaultPhysicalSizeHeight = 71;

Qt::ScreenOrientation QAndroidPlatformIntegration::m_orientation = Qt::PrimaryOrientation;
Qt::ScreenOrientation QAndroidPlatformIntegration::m_nativeOrientation = Qt::PrimaryOrientation;

Qt::ApplicationState QAndroidPlatformIntegration::m_defaultApplicationState = Qt::ApplicationActive;

bool QAndroidPlatformIntegration::m_showPasswordEnabled = false;

void *QAndroidPlatformNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource=="JavaVM")
        return QtAndroid::javaVM();
    if (resource == "QtActivity")
        return QtAndroid::activity();
    if (resource == "QtService")
        return QtAndroid::service();
    if (resource == "AndroidStyleData") {
        if (m_androidStyle) {
            if (m_androidStyle->m_styleData.isEmpty())
                m_androidStyle->m_styleData = AndroidStyle::loadStyleData();
            return &m_androidStyle->m_styleData;
        }
        else
            return nullptr;
    }
    if (resource == "AndroidStandardPalette") {
        if (m_androidStyle)
            return &m_androidStyle->m_standardPalette;
        else
            return nullptr;
    }
    if (resource == "AndroidQWidgetFonts") {
        if (m_androidStyle)
            return &m_androidStyle->m_QWidgetsFonts;
        else
            return nullptr;
    }
    if (resource == "AndroidDeviceName") {
        static QString deviceName = QtAndroid::deviceName();
        return &deviceName;
    }
    return 0;
}

QAndroidPlatformIntegration::QAndroidPlatformIntegration(const QStringList &paramList)
    : m_touchDevice(nullptr)
#ifndef QT_NO_ACCESSIBILITY
    , m_accessibility(nullptr)
#endif
{
    Q_UNUSED(paramList);
    m_androidPlatformNativeInterface = new QAndroidPlatformNativeInterface();

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (Q_UNLIKELY(m_eglDisplay == EGL_NO_DISPLAY))
        qFatal("Could not open egl display");

    EGLint major, minor;
    if (Q_UNLIKELY(!eglInitialize(m_eglDisplay, &major, &minor)))
        qFatal("Could not initialize egl display");

    if (Q_UNLIKELY(!eglBindAPI(EGL_OPENGL_ES_API)))
        qFatal("Could not bind GL_ES API");

    m_primaryScreen = new QAndroidPlatformScreen();
    screenAdded(m_primaryScreen);
    m_primaryScreen->setPhysicalSize(QSize(m_defaultPhysicalSizeWidth, m_defaultPhysicalSizeHeight));
    m_primaryScreen->setSize(QSize(m_defaultScreenWidth, m_defaultScreenHeight));
    m_primaryScreen->setAvailableGeometry(QRect(0, 0, m_defaultGeometryWidth, m_defaultGeometryHeight));

    m_mainThread = QThread::currentThread();
    QtAndroid::setAndroidPlatformIntegration(this);

    m_androidFDB = new QAndroidPlatformFontDatabase();
    m_androidPlatformServices = new QAndroidPlatformServices();

#ifndef QT_NO_CLIPBOARD
    m_androidPlatformClipboard = new QAndroidPlatformClipboard();
#endif

    m_androidSystemLocale = new QAndroidSystemLocale;

#ifndef QT_NO_ACCESSIBILITY
        m_accessibility = new QAndroidPlatformAccessibility();
#endif // QT_NO_ACCESSIBILITY

    QJNIObjectPrivate javaActivity(QtAndroid::activity());
    if (!javaActivity.isValid())
        javaActivity = QtAndroid::service();

    if (javaActivity.isValid()) {
        QJNIObjectPrivate resources = javaActivity.callObjectMethod("getResources", "()Landroid/content/res/Resources;");
        QJNIObjectPrivate configuration = resources.callObjectMethod("getConfiguration", "()Landroid/content/res/Configuration;");

        int touchScreen = configuration.getField<jint>("touchscreen");
        if (touchScreen == QJNIObjectPrivate::getStaticField<jint>("android/content/res/Configuration", "TOUCHSCREEN_FINGER")
                || touchScreen == QJNIObjectPrivate::getStaticField<jint>("android/content/res/Configuration", "TOUCHSCREEN_STYLUS"))
        {
            m_touchDevice = new QTouchDevice;
            m_touchDevice->setType(QTouchDevice::TouchScreen);
            m_touchDevice->setCapabilities(QTouchDevice::Position
                                         | QTouchDevice::Area
                                         | QTouchDevice::Pressure
                                         | QTouchDevice::NormalizedPosition);

            QJNIObjectPrivate pm = javaActivity.callObjectMethod("getPackageManager", "()Landroid/content/pm/PackageManager;");
            Q_ASSERT(pm.isValid());
            if (pm.callMethod<jboolean>("hasSystemFeature","(Ljava/lang/String;)Z",
                                     QJNIObjectPrivate::getStaticObjectField("android/content/pm/PackageManager", "FEATURE_TOUCHSCREEN_MULTITOUCH_JAZZHAND", "Ljava/lang/String;").object())) {
                m_touchDevice->setMaximumTouchPoints(10);
            } else if (pm.callMethod<jboolean>("hasSystemFeature","(Ljava/lang/String;)Z",
                                            QJNIObjectPrivate::getStaticObjectField("android/content/pm/PackageManager", "FEATURE_TOUCHSCREEN_MULTITOUCH_DISTINCT", "Ljava/lang/String;").object())) {
                m_touchDevice->setMaximumTouchPoints(4);
            } else if (pm.callMethod<jboolean>("hasSystemFeature","(Ljava/lang/String;)Z",
                                            QJNIObjectPrivate::getStaticObjectField("android/content/pm/PackageManager", "FEATURE_TOUCHSCREEN_MULTITOUCH", "Ljava/lang/String;").object())) {
                m_touchDevice->setMaximumTouchPoints(2);
            }
            QWindowSystemInterface::registerTouchDevice(m_touchDevice);
        }

        auto contentResolver = javaActivity.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
        Q_ASSERT(contentResolver.isValid());
        QJNIObjectPrivate txtShowPassValue = QJNIObjectPrivate::callStaticObjectMethod("android/provider/Settings$System",
                                                                                       "getString",
                                                                                       "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;",
                                                                                       contentResolver.object(),
                                                                                       QJNIObjectPrivate::getStaticObjectField("android/provider/Settings$System", "TEXT_SHOW_PASSWORD", "Ljava/lang/String;").object());
        if (txtShowPassValue.isValid()) {
            bool ok = false;
            const int txtShowPass = txtShowPassValue.toString().toInt(&ok);
            m_showPasswordEnabled = ok ? (txtShowPass == 1) : false;
        }
    }

    QGuiApplicationPrivate::instance()->setApplicationState(m_defaultApplicationState);
}

static bool needsBasicRenderloopWorkaround()
{
    static bool needsWorkaround =
            QtAndroid::deviceName().compare(QLatin1String("samsung SM-T211"), Qt::CaseInsensitive) == 0
            || QtAndroid::deviceName().compare(QLatin1String("samsung SM-T210"), Qt::CaseInsensitive) == 0
            || QtAndroid::deviceName().compare(QLatin1String("samsung SM-T215"), Qt::CaseInsensitive) == 0;
    return needsWorkaround;
}

bool QAndroidPlatformIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
        case ApplicationState: return true;
        case ThreadedPixmaps: return true;
        case NativeWidgets: return QtAndroid::activity();
        case OpenGL: return QtAndroid::activity();
        case ForeignWindows: return QtAndroid::activity();
        case ThreadedOpenGL: return !needsBasicRenderloopWorkaround() && QtAndroid::activity();
        case RasterGLSurface: return QtAndroid::activity();
        default:
            return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *QAndroidPlatformIntegration::createPlatformBackingStore(QWindow *window) const
{
    if (!QtAndroid::activity())
        return nullptr;
    return new QAndroidPlatformBackingStore(window);
}

QPlatformOpenGLContext *QAndroidPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    if (!QtAndroid::activity())
        return nullptr;
    QSurfaceFormat format(context->format());
    format.setAlphaBufferSize(8);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    return new QAndroidPlatformOpenGLContext(format, context->shareHandle(), m_eglDisplay);
}

QPlatformOffscreenSurface *QAndroidPlatformIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    if (!QtAndroid::activity())
        return nullptr;
    QSurfaceFormat format(surface->requestedFormat());
    format.setAlphaBufferSize(8);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);

    return new QEGLPbuffer(m_eglDisplay, format, surface);
}

QPlatformWindow *QAndroidPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    if (!QtAndroid::activity())
        return nullptr;
    if (window->type() == Qt::ForeignWindow)
        return new QAndroidPlatformForeignWindow(window);
    else
        return new QAndroidPlatformOpenGLWindow(window, m_eglDisplay);
}

QAbstractEventDispatcher *QAndroidPlatformIntegration::createEventDispatcher() const
{
    return new QAndroidEventDispatcher;
}

QAndroidPlatformIntegration::~QAndroidPlatformIntegration()
{
    if (m_eglDisplay != EGL_NO_DISPLAY)
        eglTerminate(m_eglDisplay);

    delete m_androidPlatformNativeInterface;
    delete m_androidFDB;
    delete m_androidSystemLocale;

#ifndef QT_NO_CLIPBOARD
    delete m_androidPlatformClipboard;
#endif

    QtAndroid::setAndroidPlatformIntegration(NULL);
}

QPlatformFontDatabase *QAndroidPlatformIntegration::fontDatabase() const
{
    return m_androidFDB;
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QAndroidPlatformIntegration::clipboard() const
{
    return m_androidPlatformClipboard;
}
#endif

QPlatformInputContext *QAndroidPlatformIntegration::inputContext() const
{
    return &m_platformInputContext;
}

QPlatformNativeInterface *QAndroidPlatformIntegration::nativeInterface() const
{
    return m_androidPlatformNativeInterface;
}

QPlatformServices *QAndroidPlatformIntegration::services() const
{
    return m_androidPlatformServices;
}

QVariant QAndroidPlatformIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case PasswordMaskDelay:
        // this number is from a hard-coded value in Android code (cf. PasswordTransformationMethod)
        return m_showPasswordEnabled ? 1500 : 0;
    case ShowIsMaximized:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

Qt::WindowState QAndroidPlatformIntegration::defaultWindowState(Qt::WindowFlags flags) const
{
    // Don't maximize dialogs on Android
    if (flags & Qt::Dialog & ~Qt::Window)
        return Qt::WindowNoState;

    return QPlatformIntegration::defaultWindowState(flags);
}

static const QLatin1String androidThemeName("android");
QStringList QAndroidPlatformIntegration::themeNames() const
{
    return QStringList(QString(androidThemeName));
}

QPlatformTheme *QAndroidPlatformIntegration::createPlatformTheme(const QString &name) const
{
    if (androidThemeName == name)
        return new QAndroidPlatformTheme(m_androidPlatformNativeInterface);

    return 0;
}

void QAndroidPlatformIntegration::setDefaultDisplayMetrics(int gw, int gh, int sw, int sh, int screenWidth, int screenHeight)
{
    m_defaultGeometryWidth = gw;
    m_defaultGeometryHeight = gh;
    m_defaultPhysicalSizeWidth = sw;
    m_defaultPhysicalSizeHeight = sh;
    m_defaultScreenWidth = screenWidth;
    m_defaultScreenHeight = screenHeight;
}

void QAndroidPlatformIntegration::setDefaultDesktopSize(int gw, int gh)
{
    m_defaultGeometryWidth = gw;
    m_defaultGeometryHeight = gh;
}

void QAndroidPlatformIntegration::setScreenOrientation(Qt::ScreenOrientation currentOrientation,
                                                       Qt::ScreenOrientation nativeOrientation)
{
    m_orientation = currentOrientation;
    m_nativeOrientation = nativeOrientation;
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QAndroidPlatformIntegration::accessibility() const
{
    return m_accessibility;
}
#endif

void QAndroidPlatformIntegration::setDesktopSize(int width, int height)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setAvailableGeometry", Qt::AutoConnection, Q_ARG(QRect, QRect(0,0,width, height)));
}

void QAndroidPlatformIntegration::setDisplayMetrics(int width, int height)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setPhysicalSize", Qt::AutoConnection, Q_ARG(QSize, QSize(width, height)));
}

void QAndroidPlatformIntegration::setScreenSize(int width, int height)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setSize", Qt::AutoConnection, Q_ARG(QSize, QSize(width, height)));
}

QT_END_NAMESPACE
