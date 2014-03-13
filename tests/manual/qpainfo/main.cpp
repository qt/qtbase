/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QGuiApplication>
#include <QStyleHints>
#include <QLibraryInfo>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformtheme.h>
#include <QScreen>
#include <QStringList>
#include <QVariant>
#include <QFont>
#include <QFontDatabase>
#include <QSysInfo>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QDir>
#ifndef QT_NO_OPENGL
#  include <QOpenGLContext>
#  include <QOpenGLFunctions>
#endif // QT_NO_OPENGL
#include <QWindow>

#include <iostream>
#include <string>

std::ostream &operator<<(std::ostream &str, const QSize &s)
{
    str << s.width() << 'x' << s.height();
    return str;
}

std::ostream &operator<<(std::ostream &str, const QSizeF &s)
{
    str << s.width() << 'x' << s.height();
    return str;
}

std::ostream &operator<<(std::ostream &str, const QRect &r)
{
    str << r.size() << '+' << r.x() << '+' << r.y();
    return str;
}

std::ostream &operator<<(std::ostream &str, const QStringList &l)
{
    for (int i = 0; i < l.size(); ++i) {
        if (i)
            str << ',';
        str << l.at(i).toStdString();
    }
    return str;
}

std::ostream &operator<<(std::ostream &str, const QFont &f)
{
    std::cout << '"' << f.family().toStdString() << "\" "  << f.pointSize();
    return str;
}

#ifndef QT_NO_OPENGL

std::ostream &operator<<(std::ostream &str, const QSurfaceFormat &format)
{
    str << "Version: " << format.majorVersion() << '.'
        << format.minorVersion() << " Profile: " << format.profile()
        << " Swap behavior: " << format.swapBehavior()
        << " Buffer size (RGB";
    if (format.hasAlpha())
        str << 'A';
    str << "): " << format.redBufferSize() << ',' << format.greenBufferSize()
        << ',' << format.blueBufferSize();
    if (format.hasAlpha())
        str << ',' << format.alphaBufferSize();
    if (const int dbs = format.depthBufferSize())
        str << " Depth buffer: " << dbs;
    if (const int sbs = format.stencilBufferSize())
        str << " Stencil buffer: " << sbs;
    const int samples = format.samples();
    if (samples > 0)
        str << " Samples: " << samples;
    return str;
}

void dumpGlInfo(std::ostream &str)
{
    QOpenGLContext context;
    if (context.create()) {
#  ifdef QT_OPENGL_DYNAMIC
        str << "Dynamic GL ";
#  endif
        switch (context.openGLModuleType()) {
        case QOpenGLContext::DesktopGL:
            str << "DesktopGL";
            break;
        case QOpenGLContext::GLES2:
            str << "GLES2";
            break;
        case QOpenGLContext::GLES1:
            str << "GLES1";
            break;
        }
        QWindow window;
        window.setSurfaceType(QSurface::OpenGLSurface);
        window.create();
        context.makeCurrent(&window);
        QOpenGLFunctions functions(&context);

        str << " Vendor: " << functions.glGetString(GL_VENDOR)
            << "\nRenderer: " << functions.glGetString(GL_RENDERER)
            << "\nVersion: " << functions.glGetString(GL_VERSION)
            << "\nShading language: " << functions.glGetString(GL_SHADING_LANGUAGE_VERSION)
            <<  "\nFormat: " << context.format();
    } else {
        str << "Unable to create an Open GL context.\n";
    }
}

#endif // !QT_NO_OPENGL

static QStringList toNativeSeparators(QStringList in)
{
    for (int i = 0; i < in.size(); ++i)
        in[i] = QDir::toNativeSeparators(in.at(i));
    return in;
}

#define DUMP_CAPABILITY(integration, capability) \
    if (platformIntegration->hasCapability(QPlatformIntegration::capability)) \
        std::cout << ' ' << #capability;

#define DUMP_STANDARDPATH(location) \
    std::cout << "  " << #location << ": \"" \
        << QStandardPaths::displayName(QStandardPaths::location).toStdString() << '"' \
        << ' ' << toNativeSeparators(QStandardPaths::standardLocations(QStandardPaths::location)) << '\n';

#define DUMP_LIBRARYPATH(loc) \
    std::cout << "  " << #loc << ": " << QDir::toNativeSeparators(QLibraryInfo::location(QLibraryInfo::loc)).toStdString() << '\n';

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    const QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    std::cout << QLibraryInfo::build() << " on \"" << QGuiApplication::platformName().toStdString() << "\" "
              << (QSysInfo::ByteOrder == QSysInfo::LittleEndian ? "little endian" : "big endian") << '/'
              << '\n';

#if defined(Q_OS_WIN)
    std::cout << std::hex << "Windows version: 0x" << QSysInfo::windowsVersion() << std::dec << '\n';
#elif defined(Q_OS_MAC)
    std::cout << std::hex << "Mac OS version: 0x" << QSysInfo::macVersion() << std::dec << '\n';
#endif

    std::cout << "\nLibrary info:\n";
    DUMP_LIBRARYPATH(PrefixPath)
    DUMP_LIBRARYPATH(DocumentationPath)
    DUMP_LIBRARYPATH(HeadersPath)
    DUMP_LIBRARYPATH(LibrariesPath)
    DUMP_LIBRARYPATH(LibraryExecutablesPath)
    DUMP_LIBRARYPATH(BinariesPath)
    DUMP_LIBRARYPATH(PluginsPath)
    DUMP_LIBRARYPATH(ImportsPath)
    DUMP_LIBRARYPATH(Qml2ImportsPath)
    DUMP_LIBRARYPATH(ArchDataPath)
    DUMP_LIBRARYPATH(DataPath)
    DUMP_LIBRARYPATH(TranslationsPath)
    DUMP_LIBRARYPATH(ExamplesPath)
    DUMP_LIBRARYPATH(TestsPath)

    std::cout << "\nStandard paths:\n";
    DUMP_STANDARDPATH(DesktopLocation)
    DUMP_STANDARDPATH(DocumentsLocation)
    DUMP_STANDARDPATH(FontsLocation)
    DUMP_STANDARDPATH(ApplicationsLocation)
    DUMP_STANDARDPATH(MusicLocation)
    DUMP_STANDARDPATH(MoviesLocation)
    DUMP_STANDARDPATH(PicturesLocation)
    DUMP_STANDARDPATH(TempLocation)
    DUMP_STANDARDPATH(HomeLocation)
    DUMP_STANDARDPATH(DataLocation)
    DUMP_STANDARDPATH(CacheLocation)
    DUMP_STANDARDPATH(GenericDataLocation)
    DUMP_STANDARDPATH(RuntimeLocation)
    DUMP_STANDARDPATH(ConfigLocation)
    DUMP_STANDARDPATH(DownloadLocation)
    DUMP_STANDARDPATH(GenericCacheLocation)

    std::cout << "\nPlatform capabilities:";
    DUMP_CAPABILITY(platformIntegration, ThreadedPixmaps)
    DUMP_CAPABILITY(platformIntegration, OpenGL)
    DUMP_CAPABILITY(platformIntegration, ThreadedOpenGL)
    DUMP_CAPABILITY(platformIntegration, SharedGraphicsCache)
    DUMP_CAPABILITY(platformIntegration, BufferQueueingOpenGL)
    DUMP_CAPABILITY(platformIntegration, WindowMasks)
    DUMP_CAPABILITY(platformIntegration, MultipleWindows)
    DUMP_CAPABILITY(platformIntegration, ApplicationState)
    DUMP_CAPABILITY(platformIntegration, ForeignWindows)
    DUMP_CAPABILITY(platformIntegration, AllGLFunctionsQueryable)
    std::cout << '\n';

    const QStyleHints *styleHints = QGuiApplication::styleHints();
    std::cout << "\nStyle hints: mouseDoubleClickInterval=" << styleHints->mouseDoubleClickInterval() << " startDragDistance="
              << styleHints->startDragDistance() << " startDragTime=" << styleHints->startDragTime()
              << " startDragVelocity=" << styleHints->startDragVelocity() << " keyboardInputInterval=" << styleHints->keyboardInputInterval()
              << " keyboardAutoRepeatRate=" << styleHints->keyboardAutoRepeatRate() << " cursorFlashTime=" << styleHints->cursorFlashTime()
              << " showIsFullScreen=" << styleHints->showIsFullScreen() << " passwordMaskDelay=" << styleHints->passwordMaskDelay()
              << " fontSmoothingGamma=" << styleHints->fontSmoothingGamma() << " useRtlExtensions=" << styleHints->useRtlExtensions()
              << " mousePressAndHoldInterval=" << styleHints->mousePressAndHoldInterval() << '\n';

    const QPlatformTheme *platformTheme = QGuiApplicationPrivate::platformTheme();
    std::cout << "\nTheme:\n  Styles: " << platformTheme->themeHint(QPlatformTheme::StyleNames).toStringList();
    const QString iconTheme = platformTheme->themeHint(QPlatformTheme::SystemIconThemeName).toString();
    if (!iconTheme.isEmpty()) {
        std::cout << "\n  Icon theme: " << iconTheme.toStdString()
            << ", " << platformTheme->themeHint(QPlatformTheme::SystemIconFallbackThemeName).toString().toStdString()
            << " from " << platformTheme->themeHint(QPlatformTheme::IconThemeSearchPaths).toStringList() << '\n';
    }
    if (const QFont *systemFont = platformTheme->font())
        std::cout << "  System font: " << *systemFont<< '\n';
    std::cout << "  General font : " << QFontDatabase::systemFont(QFontDatabase::GeneralFont) << '\n'
              << "  Fixed font   : " << QFontDatabase::systemFont(QFontDatabase::FixedFont) << '\n'
              << "  Title font   : " << QFontDatabase::systemFont(QFontDatabase::TitleFont) << '\n'
              << "  Smallest font: " << QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont) << "\n\n";

    if (platformTheme->usePlatformNativeDialog(QPlatformTheme::FileDialog))
        std::cout << "  Native file dialog\n";
    if (platformTheme->usePlatformNativeDialog(QPlatformTheme::ColorDialog))
        std::cout << "  Native color dialog\n";
    if (platformTheme->usePlatformNativeDialog(QPlatformTheme::FontDialog))
        std::cout << "  Native font dialog\n";

    const QList<QScreen*> screens = QGuiApplication::screens();
    const int screenCount = screens.size();
    std::cout << "\nScreens: " << screenCount << '\n';
    for (int s = 0; s < screenCount; ++s) {
        const QScreen *screen = screens.at(s);
        std::cout << (screen == QGuiApplication::primaryScreen() ? '*' : ' ')
                  << '#' << ' ' << s << " \"" << screen->name().toStdString() << '"'
                  << " Depth: " << screen->depth()
            << "\n  Geometry: " << screen->geometry() << " Available: " << screen->availableGeometry();
        if (screen->geometry() != screen->virtualGeometry())
            std::cout << "\n  Virtual geometry: " << screen->virtualGeometry() << " Available: " << screen->availableVirtualGeometry();
        if (screen->virtualSiblings().size() > 1)
            std::cout << "\n  " << screen->virtualSiblings().size() << " virtual siblings";
        std::cout << "\n  Physical size: " << screen->physicalSize() << " mm"
                  << "  Refresh: " << screen->refreshRate() << " Hz"
            << "\n  Physical DPI: " << screen->physicalDotsPerInchX()
            << ',' << screen->physicalDotsPerInchY()
            << " Logical DPI: " << screen->logicalDotsPerInchX()
            << ',' << screen->logicalDotsPerInchY()
            << "\n  DevicePixelRatio: " << screen->devicePixelRatio()
            << " Primary orientation: " << screen->primaryOrientation()
            << "\n  Orientation: " << screen->orientation()
            << " OrientationUpdateMask: " << screen->orientationUpdateMask()
            << "\n\n";
    }

#ifndef QT_NO_OPENGL
    dumpGlInfo(std::cout);
    std::cout << "\n\n";
#endif // !QT_NO_OPENGL

    return 0;
}
