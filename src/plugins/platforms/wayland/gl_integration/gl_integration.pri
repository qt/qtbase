contains(QT_CONFIG, opengl) {
    DEFINES += QT_WAYLAND_GL_SUPPORT
    QT += opengl

HEADERS += \
    $$PWD/qwaylandglintegration.h

SOURCES += \
    $$PWD/qwaylandglintegration.cpp

    QT_WAYLAND_GL_CONFIG = $$(QT_WAYLAND_GL_CONFIG)
    contains(QT_CONFIG, opengles2) {
        isEqual(QT_WAYLAND_GL_CONFIG, wayland_egl) {
            QT_WAYLAND_GL_INTEGRATION = $$QT_WAYLAND_GL_CONFIG
            CONFIG += wayland_egl
        } else:isEqual(QT_WAYLAND_GL_CONFIG,readback) {
            QT_WAYLAND_GL_INTEGRATION = readback_egl
            CONFIG += readback_egl
        } else {
            QT_WAYLAND_GL_INTEGRATION = xcomposite_egl
            CONFIG += xcomposite_egl
        }
    } else:mac {
        QT_WAYLAND_GL_INTEGRATION = readback_cgl
        CONFIG += readback_cgl
    } else {
        isEqual(QT_WAYLAND_GL_CONFIG, readback) {
            QT_WAYLAND_GL_INTEGRATION = readback_glx
            CONFIG += readback_glx
        } else {
            QT_WAYLAND_GL_INTEGRATION = xcomposite_glx
            CONFIG += xcomposite_glx
        }
    }

    message("Wayland GL Integration: $$QT_WAYLAND_GL_INTEGRATION")
}


wayland_egl {
    include ($$PWD/wayland_egl/wayland_egl.pri)
}

readback_egl {
    include ($$PWD/readback_egl/readback_egl.pri)
}

readback_glx {
    include ($$PWD/readback_glx/readback_glx.pri)
}

readback_cgl {
    include ($$PWD/readback_cgl/readback_cgl.pri)
}

xcomposite_glx {
    include ($$PWD/xcomposite_glx/xcomposite_glx.pri)
}

xcomposite_egl {
    include ($$PWD/xcomposite_egl/xcomposite_egl.pri)
}
