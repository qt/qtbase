/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>

#include "qeglproperties_p.h"
#include "qeglcontext_p.h"

QT_BEGIN_NAMESPACE

// Initialize a property block.
QEglProperties::QEglProperties()
{
    props.append(EGL_NONE);
}

QEglProperties::QEglProperties(EGLConfig cfg)
{
    props.append(EGL_NONE);
    for (int name = 0x3020; name <= 0x304F; ++name) {
        EGLint value;
        if (name != EGL_NONE && eglGetConfigAttrib(QEgl::display(), cfg, name, &value))
            setValue(name, value);
    }
    eglGetError();  // Clear the error state.
}

// Fetch the current value associated with a property.
int QEglProperties::value(int name) const
{
    for (int index = 0; index < (props.size() - 1); index += 2) {
        if (props[index] == name)
            return props[index + 1];
    }

    // If the attribute has not been explicitly set, return the EGL default
    // The following defaults were taken from the EGL 1.4 spec:
    switch(name) {
    case EGL_BUFFER_SIZE: return 0;
    case EGL_RED_SIZE: return 0;
    case EGL_GREEN_SIZE: return 0;
    case EGL_BLUE_SIZE: return 0;
    case EGL_ALPHA_SIZE: return 0;
#ifdef EGL_LUMINANCE_SIZE
    case EGL_LUMINANCE_SIZE: return 0;
#endif
#ifdef EGL_ALPHA_MASK_SIZE
    case EGL_ALPHA_MASK_SIZE: return 0;
#endif
#ifdef EGL_BIND_TO_TEXTURE_RGB
    case EGL_BIND_TO_TEXTURE_RGB: return EGL_DONT_CARE;
#endif
#ifdef EGL_BIND_TO_TEXTURE_RGBA
    case EGL_BIND_TO_TEXTURE_RGBA: return EGL_DONT_CARE;
#endif
#ifdef EGL_COLOR_BUFFER_TYPE
    case EGL_COLOR_BUFFER_TYPE: return EGL_RGB_BUFFER;
#endif
    case EGL_CONFIG_CAVEAT: return EGL_DONT_CARE;
    case EGL_CONFIG_ID: return EGL_DONT_CARE;
    case EGL_DEPTH_SIZE: return 0;
    case EGL_LEVEL: return 0;
    case EGL_NATIVE_RENDERABLE: return EGL_DONT_CARE;
    case EGL_NATIVE_VISUAL_TYPE: return EGL_DONT_CARE;
    case EGL_MAX_SWAP_INTERVAL: return EGL_DONT_CARE;
    case EGL_MIN_SWAP_INTERVAL: return EGL_DONT_CARE;
#ifdef EGL_RENDERABLE_TYPE
    case EGL_RENDERABLE_TYPE: return EGL_OPENGL_ES_BIT;
#endif
    case EGL_SAMPLE_BUFFERS: return 0;
    case EGL_SAMPLES: return 0;
    case EGL_STENCIL_SIZE: return 0;
    case EGL_SURFACE_TYPE: return EGL_WINDOW_BIT;
    case EGL_TRANSPARENT_TYPE: return EGL_NONE;
    case EGL_TRANSPARENT_RED_VALUE: return EGL_DONT_CARE;
    case EGL_TRANSPARENT_GREEN_VALUE: return EGL_DONT_CARE;
    case EGL_TRANSPARENT_BLUE_VALUE: return EGL_DONT_CARE;

#ifdef EGL_VERSION_1_3
    case EGL_CONFORMANT: return 0;
    case EGL_MATCH_NATIVE_PIXMAP: return EGL_NONE;
#endif

    case EGL_MAX_PBUFFER_HEIGHT:
    case EGL_MAX_PBUFFER_WIDTH:
    case EGL_MAX_PBUFFER_PIXELS:
    case EGL_NATIVE_VISUAL_ID:
    case EGL_NONE:
        // Attribute does not affect config selection.
        return EGL_DONT_CARE;
    default:
        // Attribute is unknown in EGL <= 1.4.
        return EGL_DONT_CARE;
    }
}

// Set the value associated with a property, replacing an existing
// value if there is one.
void QEglProperties::setValue(int name, int value)
{
    for (int index = 0; index < (props.size() - 1); index += 2) {
        if (props[index] == name) {
            props[index + 1] = value;
            return;
        }
    }
    props[props.size() - 1] = name;
    props.append(value);
    props.append(EGL_NONE);
}

// Remove a property value.  Returns false if the property is not present.
bool QEglProperties::removeValue(int name)
{
    for (int index = 0; index < (props.size() - 1); index += 2) {
        if (props[index] == name) {
            while ((index + 2) < props.size()) {
                props[index] = props[index + 2];
                ++index;
            }
            props.resize(props.size() - 2);
            return true;
        }
    }
    return false;
}

void QEglProperties::setDeviceType(int devType)
{
    if (devType == QInternal::Pixmap || devType == QInternal::Image)
        setValue(EGL_SURFACE_TYPE, EGL_PIXMAP_BIT);
    else if (devType == QInternal::Pbuffer)
        setValue(EGL_SURFACE_TYPE, EGL_PBUFFER_BIT);
    else
        setValue(EGL_SURFACE_TYPE, EGL_WINDOW_BIT);
}


// Sets the red, green, blue, and alpha sizes based on a pixel format.
// Normally used to match a configuration request to the screen format.
void QEglProperties::setPixelFormat(QImage::Format pixelFormat)
{
    int red, green, blue, alpha;
    switch (pixelFormat) {
        case QImage::Format_RGB32:
        case QImage::Format_RGB888:
            red = green = blue = 8; alpha = 0; break;
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
            red = green = blue = alpha = 8; break;
        case QImage::Format_RGB16:
            red = 5; green = 6; blue = 5; alpha = 0; break;
        case QImage::Format_ARGB8565_Premultiplied:
            red = 5; green = 6; blue = 5; alpha = 8; break;
        case QImage::Format_RGB666:
            red = green = blue = 6; alpha = 0; break;
        case QImage::Format_ARGB6666_Premultiplied:
            red = green = blue = alpha = 6; break;
        case QImage::Format_RGB555:
            red = green = blue = 5; alpha = 0; break;
        case QImage::Format_ARGB8555_Premultiplied:
            red = green = blue = 5; alpha = 8; break;
        case QImage::Format_RGB444:
            red = green = blue = 4; alpha = 0; break;
        case QImage::Format_ARGB4444_Premultiplied:
            red = green = blue = alpha = 4; break;
        default:
            qWarning() << "QEglProperties::setPixelFormat(): Unsupported pixel format";
            red = green = blue = alpha = 1; break;
    }
    setValue(EGL_RED_SIZE, red);
    setValue(EGL_GREEN_SIZE, green);
    setValue(EGL_BLUE_SIZE, blue);
    setValue(EGL_ALPHA_SIZE, alpha);
}

void QEglProperties::setRenderableType(QEgl::API api)
{
#ifdef EGL_RENDERABLE_TYPE
#if defined(QT_OPENGL_ES_2)
    if (api == QEgl::OpenGL)
        setValue(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT);
#elif defined(QT_OPENGL_ES)
    if (api == QEgl::OpenGL)
        setValue(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT);
#elif defined(EGL_OPENGL_BIT)
    if (api == QEgl::OpenGL)
        setValue(EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT);
#endif
#ifdef EGL_OPENVG_BIT
    if (api == QEgl::OpenVG)
        setValue(EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT);
#endif
#else
    Q_UNUSED(api);
#endif
}

// Reduce the complexity of a configuration request to ask for less
// because the previous request did not result in success.  Returns
// true if the complexity was reduced, or false if no further
// reductions in complexity are possible.
bool QEglProperties::reduceConfiguration()
{
#ifdef EGL_SWAP_BEHAVIOR
    if (value(EGL_SWAP_BEHAVIOR) != EGL_DONT_CARE)
        removeValue(EGL_SWAP_BEHAVIOR);
#endif

#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
    // For OpenVG, we sometimes try to create a surface using a pre-multiplied format. If we can't
    // find a config which supports pre-multiplied formats, remove the flag on the surface type:
    EGLint surfaceType = value(EGL_SURFACE_TYPE);
    if (surfaceType & EGL_VG_ALPHA_FORMAT_PRE_BIT) {
        surfaceType ^= EGL_VG_ALPHA_FORMAT_PRE_BIT;
        setValue(EGL_SURFACE_TYPE, surfaceType);
        return true;
    }
#endif
    // EGL chooses configs with the highest color depth over
    // those with smaller (but faster) lower color depths. One
    // way around this is to set EGL_BUFFER_SIZE to 16, which
    // trumps the others. Of course, there may not be a 16-bit
    // config available, so it's the first restraint we remove.
    if (value(EGL_BUFFER_SIZE) == 16) {
        removeValue(EGL_BUFFER_SIZE);
        return true;
    }
    if (removeValue(EGL_SAMPLE_BUFFERS)) {
        removeValue(EGL_SAMPLES);
        return true;
    }
    if (removeValue(EGL_ALPHA_SIZE)) {
#if defined(EGL_BIND_TO_TEXTURE_RGBA) && defined(EGL_BIND_TO_TEXTURE_RGB)
        if (removeValue(EGL_BIND_TO_TEXTURE_RGBA))
            setValue(EGL_BIND_TO_TEXTURE_RGB, TRUE);
#endif
        return true;
    }
    if (removeValue(EGL_STENCIL_SIZE))
        return true;
    if (removeValue(EGL_DEPTH_SIZE))
        return true;
#ifdef EGL_BIND_TO_TEXTURE_RGB
    if (removeValue(EGL_BIND_TO_TEXTURE_RGB))
        return true;
#endif
    return false;
}

static void addTag(QString& str, const QString& tag)
{
    int lastnl = str.lastIndexOf(QLatin1String("\n"));
    if (lastnl == -1)
        lastnl = 0;
    if ((str.length() - lastnl) >= 50)
        str += QLatin1String("\n   ");
    str += tag;
}

// Convert a property list to a string suitable for debug output.
QString QEglProperties::toString() const
{
    QString str;
    int val;

    val = value(EGL_CONFIG_ID);
    if (val != EGL_DONT_CARE) {
        str += QLatin1String("id=");
        str += QString::number(val);
        str += QLatin1Char(' ');
    }

#ifdef EGL_RENDERABLE_TYPE
    val = value(EGL_RENDERABLE_TYPE);
    if (val != EGL_DONT_CARE) {
        str += QLatin1String("type=");
        QStringList types;
        if ((val & EGL_OPENGL_ES_BIT) != 0)
            types += QLatin1String("es1");
#ifdef EGL_OPENGL_ES2_BIT
        if ((val & EGL_OPENGL_ES2_BIT) != 0)
            types += QLatin1String("es2");
#endif
#ifdef EGL_OPENGL_BIT
        if ((val & EGL_OPENGL_BIT) != 0)
            types += QLatin1String("gl");
#endif
        if ((val & EGL_OPENVG_BIT) != 0)
            types += QLatin1String("vg");
        if ((val & ~7) != 0)
            types += QString::number(val);
        str += types.join(QLatin1String(","));
    } else {
        str += QLatin1String("type=any");
    }
#else
    str += QLatin1String("type=es1");
#endif

    int red = value(EGL_RED_SIZE);
    int green = value(EGL_GREEN_SIZE);
    int blue = value(EGL_BLUE_SIZE);
    int alpha = value(EGL_ALPHA_SIZE);
    int bufferSize = value(EGL_BUFFER_SIZE);
    if (bufferSize == (red + green + blue + alpha))
        bufferSize = 0;
    str += QLatin1String(" rgba=");
    str += QString::number(red);
    str += QLatin1Char(',');
    str += QString::number(green);
    str += QLatin1Char(',');
    str += QString::number(blue);
    str += QLatin1Char(',');
    str += QString::number(alpha);
    if (bufferSize != 0) {
        // Only report buffer size if different than r+g+b+a.
        str += QLatin1String(" buffer-size=");
        str += QString::number(bufferSize);
    }

#ifdef EGL_COLOR_BUFFER_TYPE
    val = value(EGL_COLOR_BUFFER_TYPE);
    if (val == EGL_LUMINANCE_BUFFER) {
        addTag(str, QLatin1String(" color-buffer-type=luminance"));
    } else if (val != EGL_DONT_CARE && val != EGL_RGB_BUFFER) {
        addTag(str, QLatin1String(" color-buffer-type="));
        str += QString::number(val, 16);
    }
#endif

    val = value(EGL_DEPTH_SIZE);
    if (val != 0) {
        addTag(str, QLatin1String(" depth="));
        str += QString::number(val);
    }

    val = value(EGL_STENCIL_SIZE);
    if (val != 0) {
        addTag(str, QLatin1String(" stencil="));
        str += QString::number(val);
    }

    val = value(EGL_SURFACE_TYPE);
    if (val != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" surface-type="));
        QStringList types;
        if ((val & EGL_WINDOW_BIT) != 0)
            types += QLatin1String("window");
        if ((val & EGL_PIXMAP_BIT) != 0)
            types += QLatin1String("pixmap");
        if ((val & EGL_PBUFFER_BIT) != 0)
            types += QLatin1String("pbuffer");
#ifdef EGL_VG_COLORSPACE_LINEAR_BIT
        if ((val & EGL_VG_COLORSPACE_LINEAR_BIT) != 0)
            types += QLatin1String("vg-colorspace-linear");
#endif
#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
        if ((val & EGL_VG_ALPHA_FORMAT_PRE_BIT) != 0)
            types += QLatin1String("vg-alpha-format-pre");
#endif
        if ((val & ~(EGL_WINDOW_BIT | EGL_PIXMAP_BIT | EGL_PBUFFER_BIT
#ifdef EGL_VG_COLORSPACE_LINEAR_BIT
                     | EGL_VG_COLORSPACE_LINEAR_BIT
#endif
#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
                     | EGL_VG_ALPHA_FORMAT_PRE_BIT
#endif
                     )) != 0) {
            types += QString::number(val);
        }
        str += types.join(QLatin1String(","));
    }

    val = value(EGL_CONFIG_CAVEAT);
    if (val != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" caveat="));
        if (val == EGL_NONE)
            str += QLatin1String("none");
        else if (val == EGL_SLOW_CONFIG)
            str += QLatin1String("slow");
        else if (val == EGL_NON_CONFORMANT_CONFIG)
            str += QLatin1String("non-conformant");
        else
            str += QString::number(val, 16);
    }

    val = value(EGL_LEVEL);
    if (val != 0) {
        addTag(str, QLatin1String(" level="));
        str += QString::number(val);
    }

    int width, height, pixels;
    width = value(EGL_MAX_PBUFFER_WIDTH);
    height = value(EGL_MAX_PBUFFER_HEIGHT);
    pixels = value(EGL_MAX_PBUFFER_PIXELS);
    if (height != EGL_DONT_CARE || width != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" max-pbuffer-size="));
        str += QString::number(width);
        str += QLatin1Char('x');
        str += QString::number(height);
        if (pixels != (width * height)) {
            addTag(str, QLatin1String(" max-pbuffer-pixels="));
            str += QString::number(pixels);
        }
    }

    val = value(EGL_NATIVE_RENDERABLE);
    if (val != EGL_DONT_CARE) {
        if (val)
            addTag(str, QLatin1String(" native-renderable=true"));
        else
            addTag(str, QLatin1String(" native-renderable=false"));
    }

    val = value(EGL_NATIVE_VISUAL_ID);
    if (val != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" visual-id="));
        str += QString::number(val);
    }

    val = value(EGL_NATIVE_VISUAL_TYPE);
    if (val != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" visual-type="));
        str += QString::number(val);
    }

#ifdef EGL_PRESERVED_RESOURCES
    val = value(EGL_PRESERVED_RESOURCES);
    if (val != EGL_DONT_CARE) {
        if (val)
            addTag(str, QLatin1String(" preserved-resources=true"));
        else
            addTag(str, QLatin1String(" preserved-resources=false"));
    }
#endif

    val = value(EGL_SAMPLES);
    if (val != 0) {
        addTag(str, QLatin1String(" samples="));
        str += QString::number(val);
    }

    val = value(EGL_SAMPLE_BUFFERS);
    if (val != 0) {
        addTag(str, QLatin1String(" sample-buffers="));
        str += QString::number(val);
    }

    val = value(EGL_TRANSPARENT_TYPE);
    if (val == EGL_TRANSPARENT_RGB) {
        addTag(str, QLatin1String(" transparent-rgb="));
        str += QString::number(value(EGL_TRANSPARENT_RED_VALUE));
        str += QLatin1Char(',');
        str += QString::number(value(EGL_TRANSPARENT_GREEN_VALUE));
        str += QLatin1Char(',');
        str += QString::number(value(EGL_TRANSPARENT_BLUE_VALUE));
    }

#if defined(EGL_BIND_TO_TEXTURE_RGB) && defined(EGL_BIND_TO_TEXTURE_RGBA)
    val = value(EGL_BIND_TO_TEXTURE_RGB);
    int val2 = value(EGL_BIND_TO_TEXTURE_RGBA);
    if (val != EGL_DONT_CARE || val2 != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" bind-texture="));
        if (val == EGL_TRUE)
            str += QLatin1String("rgb");
        else
            str += QLatin1String("no-rgb");
        if (val2 == EGL_TRUE)
            str += QLatin1String(",rgba");
        else
            str += QLatin1String(",no-rgba");
    }
#endif

#ifdef EGL_MIN_SWAP_INTERVAL
    val = value(EGL_MIN_SWAP_INTERVAL);
    if (val != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" min-swap-interval="));
        str += QString::number(val);
    }
#endif

#ifdef EGL_MIN_SWAP_INTERVAL
    val = value(EGL_MAX_SWAP_INTERVAL);
    if (val != EGL_DONT_CARE) {
        addTag(str, QLatin1String(" max-swap-interval="));
        str += QString::number(val);
    }
#endif

#ifdef EGL_LUMINANCE_SIZE
    val = value(EGL_LUMINANCE_SIZE);
    if (val != 0) {
        addTag(str, QLatin1String(" luminance="));
        str += QString::number(val);
    }
#endif

#ifdef EGL_ALPHA_MASK_SIZE
    val = value(EGL_ALPHA_MASK_SIZE);
    if (val != 0) {
        addTag(str, QLatin1String(" alpha-mask="));
        str += QString::number(val);
    }
#endif

#ifdef EGL_CONFORMANT
    val = value(EGL_CONFORMANT);
    if (val != 0) {
        if (val)
            addTag(str, QLatin1String(" conformant=true"));
        else
            addTag(str, QLatin1String(" conformant=false"));
    }
#endif

    return str;
}

QT_END_NAMESPACE


