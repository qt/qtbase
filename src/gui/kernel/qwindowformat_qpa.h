/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QPLATFORMWINDOWFORMAT_QPA_H
#define QPLATFORMWINDOWFORMAT_QPA_H

#include <QtGui/QPlatformWindow>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QWindowFormatPrivate;

class Q_GUI_EXPORT QWindowFormat
{
public:
    enum FormatOption {
        StereoBuffers           = 0x0001,
        WindowSurface           = 0x0002
    };
    Q_DECLARE_FLAGS(FormatOptions, FormatOption)

    enum ColorFormat {
        InvalidColorFormat,
        RGB565,
        RGBA5658,
        RGBA5551,
        RGB888,
        RGBA8888
    };

    enum SwapBehavior {
        DefaultSwapBehavior,
        SingleBuffer,
        DoubleBuffer,
        TripleBuffer
    };

    enum OpenGLContextProfile {
        NoProfile,
        CoreProfile,
        CompatibilityProfile
    };

    QWindowFormat();
    QWindowFormat(FormatOptions options);
    QWindowFormat(const QWindowFormat &other);
    QWindowFormat &operator=(const QWindowFormat &other);
    ~QWindowFormat();

    void setDepthBufferSize(int size);
    int depthBufferSize() const;

    void setStencilBufferSize(int size);
    int stencilBufferSize() const;

    void setSamples(int numSamples);
    int samples() const;

    void setSwapBehavior(SwapBehavior behavior);
    SwapBehavior swapBehavior() const;

    void setColorFormat(ColorFormat format);
    ColorFormat colorFormat() const;

    void setProfile(OpenGLContextProfile profile);
    OpenGLContextProfile profile() const;

    void setSharedContext(QPlatformGLContext *context);
    QPlatformGLContext *sharedGLContext() const;

    bool stereo() const;
    void setStereo(bool enable);
    bool windowSurface() const;
    void setWindowSurface(bool enable);

    void setOption(QWindowFormat::FormatOptions opt);
    bool testOption(QWindowFormat::FormatOptions opt) const;

private:
    QWindowFormatPrivate *d;

    void detach();

    friend Q_GUI_EXPORT bool operator==(const QWindowFormat&, const QWindowFormat&);
    friend Q_GUI_EXPORT bool operator!=(const QWindowFormat&, const QWindowFormat&);
#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const QWindowFormat &);
#endif
};

Q_GUI_EXPORT bool operator==(const QWindowFormat&, const QWindowFormat&);
Q_GUI_EXPORT bool operator!=(const QWindowFormat&, const QWindowFormat&);

#ifndef QT_NO_DEBUG_STREAM
Q_OPENGL_EXPORT QDebug operator<<(QDebug, const QWindowFormat &);
#endif

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowFormat::FormatOptions)

inline bool QWindowFormat::stereo() const
{
    return testOption(QWindowFormat::StereoBuffers);
}

inline bool QWindowFormat::windowSurface() const
{
    return testOption(QWindowFormat::WindowSurface);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif //QPLATFORMWINDOWFORMAT_QPA_H
