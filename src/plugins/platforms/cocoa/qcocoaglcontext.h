#ifndef QCOCOAGLCONTEXT_H
#define QCOCOAGLCONTEXT_H

#include <QtGui/QPlatformGLContext>
#include <QtGui/QWindowFormat>

#include <Cocoa/Cocoa.h>

QT_BEGIN_NAMESPACE

class QCocoaGLContext : public QPlatformGLContext
{
public:
    QCocoaGLContext(NSOpenGLView *glView);
    void makeCurrent();
    void doneCurrent();
    void swapBuffers();
    void* getProcAddress(const QString& procName);
    QWindowFormat windowFormat() const;
    static NSOpenGLPixelFormat *createNSOpenGLPixelFormat();
private:
    NSOpenGLView *m_glView;
};

QT_END_NAMESPACE

#endif // QCOCOAGLCONTEXT_H
