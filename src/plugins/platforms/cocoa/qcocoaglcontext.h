#ifndef QCOCOAGLCONTEXT_H
#define QCOCOAGLCONTEXT_H

#include <QtCore/QWeakPointer>
#include <QtGui/QPlatformGLContext>
#include <QtGui/QGuiGLContext>
#include <QtGui/QWindow>

#include <Cocoa/Cocoa.h>

QT_BEGIN_NAMESPACE

class QCocoaGLSurface : public QPlatformGLSurface
{
public:
    QCocoaGLSurface(const QGuiGLFormat &format, QWindow *window)
        : QPlatformGLSurface(format)
        , window(window)
    {
    }

    QWindow *window;
};

class QCocoaGLContext : public QPlatformGLContext
{
public:
    QCocoaGLContext(const QGuiGLFormat &format, QPlatformGLContext *share);

    QGuiGLFormat format() const;

    void swapBuffers(const QPlatformGLSurface &surface);

    bool makeCurrent(const QPlatformGLSurface &surface);
    void doneCurrent();

    void (*getProcAddress(const QByteArray &procName)) ();

    void update();

    static NSOpenGLPixelFormat *createNSOpenGLPixelFormat();
    NSOpenGLContext *nsOpenGLContext() const;

private:
    void setActiveWindow(QWindow *window);

    NSOpenGLContext *m_context;
    QGuiGLFormat m_format;
    QWeakPointer<QWindow> m_currentWindow;
};

QT_END_NAMESPACE

#endif // QCOCOAGLCONTEXT_H
