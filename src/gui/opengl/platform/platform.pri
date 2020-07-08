qtConfig(xlib):qtConfig(opengl):!qtConfig(opengles2): \
    include(unix/unix.pri)
qtConfig(egl): \
    include(egl/egl.pri)
