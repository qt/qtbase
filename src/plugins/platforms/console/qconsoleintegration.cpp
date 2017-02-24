/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qconsoleintegration.h"
#include "qconsolebackingstore.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>


#if defined(Q_OS_WIN)
#include <qpa/qplatformwindow.h>
#include <QtPlatformSupport/private/qbasicfontdatabase_p.h>
#include "QWindowsFontDatabase.h"
//#elif defined(QT_NO_FONTCONFIG)
//#include <qpa/qplatformfontdatabase.h>
#elif defined(Q_OS_MAC)
#include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>
#else
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#endif

#if !defined(Q_OS_WIN)
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#elif defined(Q_OS_WINRT)
#include <QtCore/private/qeventdispatcher_winrt_p.h>
#else
#include <QtCore/private/qeventdispatcher_win_p.h>
#endif

QT_BEGIN_NAMESPACE

static const char debugBackingStoreEnvironmentVariable[] = "QT_DEBUG_BACKINGSTORE";

static inline unsigned parseOptions(const QStringList &paramList) 
{  
	return 0;
}

QConsoleIntegration::QConsoleIntegration(const QStringList &parameters)
    : m_fontDatabase(0)
    , m_options(parseOptions(parameters))
{
    if (qEnvironmentVariableIsSet(debugBackingStoreEnvironmentVariable)
        && qEnvironmentVariableIntValue(debugBackingStoreEnvironmentVariable) > 0) {
        m_options |= DebugBackingStore | EnableFonts;
    }

    QConsoleScreen *mPrimaryScreen = new QConsoleScreen();

    mPrimaryScreen->mGeometry = QRect(0, 0, 240, 320);
    mPrimaryScreen->mDepth = 32;
    mPrimaryScreen->mFormat = QImage::Format_ARGB32_Premultiplied;

    screenAdded(mPrimaryScreen);
}

QConsoleIntegration::~QConsoleIntegration()
{
    delete m_fontDatabase;
}

bool QConsoleIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case MultipleWindows: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

// Dummy font database that does not scan the fonts directory to be
// used for command line tools like qmlplugindump that do not create windows
// unless DebugBackingStore is activated.
class DummyFontDatabase : public QPlatformFontDatabase
{
public:
    virtual void populateFontDatabase() Q_DECL_OVERRIDE {}
};

QPlatformFontDatabase *QConsoleIntegration::fontDatabase() const
{	
#if defined(Q_OS_WIN)
	m_fontDatabase = new QWindowsFontDatabase;
#elif defined(Q_OS_MAC)
	m_fontDatabase = new QCoreTextFontDatabase;
#else
	m_fontDatabase = new QGenericUnixFontDatabase;
#endif

    return m_fontDatabase;
}

QPlatformWindow *QConsoleIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);
    QPlatformWindow *w = new QPlatformWindow(window);
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QConsoleIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QConsoleBackingStore(window);
}

QAbstractEventDispatcher *QConsoleIntegration::createEventDispatcher() const
{
#ifdef Q_OS_WIN
#ifndef Q_OS_WINRT
    return new QEventDispatcherWin32;
#else // !Q_OS_WINRT
    return new QEventDispatcherWinRT;
#endif // Q_OS_WINRT
#else
    return createUnixEventDispatcher();
#endif
}

QConsoleIntegration *QConsoleIntegration::instance()
{
    return static_cast<QConsoleIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QT_END_NAMESPACE
