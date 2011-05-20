#include "qcocoaglcontext.h"
#include <qdebug.h>
#include <QtCore/private/qcore_mac_p.h>

#import <Cocoa/Cocoa.h>

QCocoaGLContext::QCocoaGLContext(NSOpenGLView *glView)
:m_glView(glView)
{

}

void QCocoaGLContext::makeCurrent()
{
    [[m_glView openGLContext] makeCurrentContext];
}
void QCocoaGLContext::doneCurrent()
{
    [NSOpenGLContext clearCurrentContext];
}

void QCocoaGLContext::swapBuffers()
{
    [[m_glView openGLContext] flushBuffer];
}

void* QCocoaGLContext::getProcAddress(const QString& procName)
{
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
            CFSTR("/System/Library/Frameworks/OpenGL.framework"), kCFURLPOSIXPathStyle, false);
    CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFStringRef procNameCF = QCFString::toCFStringRef(procName);
    void *proc = CFBundleGetFunctionPointerForName(bundle, procNameCF);
    CFRelease(url);
    CFRelease(bundle);
    CFRelease(procNameCF);
    return proc;
}

// Match up with createNSOpenGLPixelFormat below!
QWindowFormat QCocoaGLContext::windowFormat() const
{
    QWindowFormat format;
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);

/*
    format.setDepthBufferSize(24);
    format.setAccumBufferSize(0);
    format.setStencilBufferSize(8);
    format.setSampleBuffers(false);
    format.setSamples(1);
    format.setDepth(true);
    format.setRgba(true);
    format.setAlpha(true);
    format.setAccum(false);
    format.setStencil(true);
    format.setStereo(false);
    format.setDirectRendering(false);
*/
    return format;
}

NSOpenGLPixelFormat *QCocoaGLContext::createNSOpenGLPixelFormat()
{
    NSOpenGLPixelFormatAttribute attrs[] =
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 32,
        0
    };

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    return pixelFormat;
}

NSOpenGLContext *QCocoaGLContext::nsOpenGLContext() const
{
    return [m_glView openGLContext];
}

