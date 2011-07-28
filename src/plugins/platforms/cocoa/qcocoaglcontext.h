#ifndef QCOCOAGLCONTEXT_H
#define QCOCOAGLCONTEXT_H

#include <QtCore/QWeakPointer>
#include <QtGui/QPlatformGLContext>
#include <QtGui/QGuiGLContext>
#include <QtGui/QWindow>

#undef slots
#include <Cocoa/Cocoa.h>

QT_BEGIN_NAMESPACE

class QCocoaGLContext : public QPlatformGLContext
{
public:
    QCocoaGLContext(const QSurfaceFormat &format, QPlatformGLContext *share);

    QSurfaceFormat format() const;

    void swapBuffers(QPlatformSurface *surface);

    bool makeCurrent(QPlatformSurface *surface);
    void doneCurrent();

    void (*getProcAddress(const QByteArray &procName)) ();

    void update();

    static NSOpenGLPixelFormat *createNSOpenGLPixelFormat();
    NSOpenGLContext *nsOpenGLContext() const;

private:
    void setActiveWindow(QWindow *window);

    NSOpenGLContext *m_context;
    QSurfaceFormat m_format;
    QWeakPointer<QWindow> m_currentWindow;
};

QT_END_NAMESPACE

#endif // QCOCOAGLCONTEXT_H
