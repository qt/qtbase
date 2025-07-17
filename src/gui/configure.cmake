# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs

# input freetype
set(INPUT_freetype "undefined" CACHE STRING "")
set_property(CACHE INPUT_freetype PROPERTY STRINGS undefined no qt system)

# input harfbuzz
set(INPUT_harfbuzz "undefined" CACHE STRING "")
set_property(CACHE INPUT_harfbuzz PROPERTY STRINGS undefined no qt system)

# input libjpeg
set(INPUT_libjpeg "undefined" CACHE STRING "")
set_property(CACHE INPUT_libjpeg PROPERTY STRINGS undefined no qt system)

# input libmd4c
set(INPUT_libmd4c "undefined" CACHE STRING "")
set_property(CACHE INPUT_libmd4c PROPERTY STRINGS undefined no qt system)

# input libpng
set(INPUT_libpng "undefined" CACHE STRING "")
set_property(CACHE INPUT_libpng PROPERTY STRINGS undefined no qt system)



#### Libraries
qt_set01(X11_SUPPORTED LINUX OR HPUX OR FREEBSD OR NETBSD OR OPENBSD OR SOLARIS OR
    HURD)
qt_find_package(ATSPI2 MODULE PROVIDED_TARGETS PkgConfig::ATSPI2 MODULE_NAME gui QMAKE_LIB atspi)
qt_find_package(DirectFB PROVIDED_TARGETS PkgConfig::DirectFB MODULE_NAME gui QMAKE_LIB directfb)
qt_find_package(Libdrm MODULE PROVIDED_TARGETS Libdrm::Libdrm MODULE_NAME gui QMAKE_LIB drm)
qt_find_package(PlatformGraphics
        PROVIDED_TARGETS PlatformGraphics::PlatformGraphics
        MODULE_NAME gui QMAKE_LIB platform_graphics)
qt_find_package(EGL MODULE PROVIDED_TARGETS EGL::EGL MODULE_NAME gui QMAKE_LIB egl)

qt_find_package(WrapSystemFreetype 2.2.0 MODULE
    PROVIDED_TARGETS WrapSystemFreetype::WrapSystemFreetype MODULE_NAME gui QMAKE_LIB freetype)
if(QT_FEATURE_system_zlib)
    qt_add_qmake_lib_dependency(freetype zlib)
endif()
qt_find_package(Fontconfig PROVIDED_TARGETS Fontconfig::Fontconfig MODULE_NAME gui QMAKE_LIB fontconfig)
qt_add_qmake_lib_dependency(fontconfig freetype)
qt_find_package(gbm MODULE PROVIDED_TARGETS gbm::gbm MODULE_NAME gui QMAKE_LIB gbm)
qt_find_package(WrapSystemHarfbuzz 2.6.0 MODULE
    PROVIDED_TARGETS WrapSystemHarfbuzz::WrapSystemHarfbuzz MODULE_NAME gui QMAKE_LIB harfbuzz)
qt_find_package(Libinput MODULE
    PROVIDED_TARGETS Libinput::Libinput MODULE_NAME gui QMAKE_LIB libinput)
qt_find_package_extend_sbom(TARGETS Libinput::Libinput
    COPYRIGHTS
        "Copyright © 2006-2009 Simon Thum"
        "Copyright © 2008-2012 Kristian Høgsberg"
        "Copyright © 2010-2012 Intel Corporation"
        "Copyright © 2010-2011 Benjamin Franzke"
        "Copyright © 2011-2012 Collabora, Ltd."
        "Copyright © 2013-2014 Jonas Ådahl"
        "Copyright © 2013-2015 Red Hat, Inc."
)
qt_find_package(WrapSystemJpeg MODULE
    PROVIDED_TARGETS WrapSystemJpeg::WrapSystemJpeg MODULE_NAME gui QMAKE_LIB libjpeg)
qt_find_package(WrapSystemMd4c MODULE
    PROVIDED_TARGETS WrapSystemMd4c::WrapSystemMd4c MODULE_NAME gui QMAKE_LIB libmd4c)
qt_find_package(WrapSystemPNG MODULE
    PROVIDED_TARGETS WrapSystemPNG::WrapSystemPNG MODULE_NAME gui QMAKE_LIB libpng)
if(QT_FEATURE_system_zlib)
    qt_add_qmake_lib_dependency(libpng zlib)
endif()
qt_find_package(Mtdev MODULE PROVIDED_TARGETS PkgConfig::Mtdev MODULE_NAME gui QMAKE_LIB mtdev)
qt_find_package(WrapOpenGL MODULE PROVIDED_TARGETS WrapOpenGL::WrapOpenGL MODULE_NAME gui QMAKE_LIB opengl)
qt_find_package(GLESv2 MODULE
    PROVIDED_TARGETS GLESv2::GLESv2 MODULE_NAME gui QMAKE_LIB opengl_es2)
qt_find_package(Tslib MODULE PROVIDED_TARGETS PkgConfig::Tslib MODULE_NAME gui QMAKE_LIB tslib)
qt_find_package(WrapVulkanHeaders MODULE PROVIDED_TARGETS WrapVulkanHeaders::WrapVulkanHeaders
    MODULE_NAME gui QMAKE_LIB vulkan MARK_OPTIONAL)
if(LINUX OR FREEBSD OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(Wayland MODULE PROVIDED_TARGETS Wayland::Server
                    MODULE_NAME gui QMAKE_LIB wayland_server)
    qt_find_package(Wayland MODULE PROVIDED_TARGETS Wayland::Client
                    MODULE_NAME gui QMAKE_LIB wayland_client)
    # Gui doesn't use these, but the wayland qpa plugin does, and we
    # need to list the rest of the provided targets here, so they are
    # promoted to global, and can be accessed by the SBOM at the root
    # project level. That's not possible to do in the wayland qpa subdir,
    # due to different cmake directory scopes.
    qt_find_package(Wayland MODULE PROVIDED_TARGETS Wayland::Cursor
                    MODULE_NAME gui QMAKE_LIB wayland_cursor)
    qt_find_package(Wayland MODULE PROVIDED_TARGETS Wayland::Egl
                    MODULE_NAME gui QMAKE_LIB wayland_egl)
endif()
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(X11 MODULE PROVIDED_TARGETS X11::X11 MODULE_NAME gui QMAKE_LIB xlib)
endif()
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(X11 MODULE PROVIDED_TARGETS X11::SM X11::ICE MODULE_NAME gui QMAKE_LIB x11sm)
endif()
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 1.11 MODULE PROVIDED_TARGETS XCB::XCB MODULE_NAME gui QMAKE_LIB xcb)
endif()
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 0.1.1 MODULE
        COMPONENTS CURSOR PROVIDED_TARGETS XCB::CURSOR MODULE_NAME gui QMAKE_LIB xcb_cursor)
endif()
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 0.3.9 MODULE
        COMPONENTS ICCCM PROVIDED_TARGETS XCB::ICCCM MODULE_NAME gui QMAKE_LIB xcb_icccm)
endif()
qt_add_qmake_lib_dependency(xcb_icccm xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 0.3.8 MODULE
        COMPONENTS UTIL PROVIDED_TARGETS XCB::UTIL MODULE_NAME gui QMAKE_LIB xcb_util)
endif()
qt_add_qmake_lib_dependency(xcb_util xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 0.3.9 MODULE
        COMPONENTS IMAGE PROVIDED_TARGETS XCB::IMAGE MODULE_NAME gui QMAKE_LIB xcb_image)
endif()
qt_add_qmake_lib_dependency(xcb_image xcb_shm xcb_util xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 0.3.9 MODULE
        COMPONENTS KEYSYMS PROVIDED_TARGETS XCB::KEYSYMS MODULE_NAME gui QMAKE_LIB xcb_keysyms)
endif()
qt_add_qmake_lib_dependency(xcb_keysyms xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 0.3.9 MODULE
        COMPONENTS RENDERUTIL PROVIDED_TARGETS XCB::RENDERUTIL MODULE_NAME gui QMAKE_LIB xcb_renderutil)
endif()
qt_add_qmake_lib_dependency(xcb_renderutil xcb xcb_render)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS RANDR PROVIDED_TARGETS XCB::RANDR MODULE_NAME gui QMAKE_LIB xcb_randr)
endif()
qt_add_qmake_lib_dependency(xcb_randr xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS SHAPE PROVIDED_TARGETS XCB::SHAPE MODULE_NAME gui QMAKE_LIB xcb_shape)
endif()
qt_add_qmake_lib_dependency(xcb_shape xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS SHM PROVIDED_TARGETS XCB::SHM MODULE_NAME gui QMAKE_LIB xcb_shm)
endif()
qt_add_qmake_lib_dependency(xcb_shm xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS SYNC PROVIDED_TARGETS XCB::SYNC MODULE_NAME gui QMAKE_LIB xcb_sync)
endif()
qt_add_qmake_lib_dependency(xcb_sync xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS XFIXES PROVIDED_TARGETS XCB::XFIXES MODULE_NAME gui QMAKE_LIB xcb_xfixes)
endif()
qt_add_qmake_lib_dependency(xcb_xfixes xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(X11_XCB MODULE PROVIDED_TARGETS X11::XCB MODULE_NAME gui QMAKE_LIB xcb_xlib)
endif()
qt_add_qmake_lib_dependency(xcb_xlib xcb xlib)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS XKB PROVIDED_TARGETS XCB::XKB MODULE_NAME gui QMAKE_LIB xcb_xkb)
endif()
qt_add_qmake_lib_dependency(xcb_xkb xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS RENDER PROVIDED_TARGETS XCB::RENDER MODULE_NAME gui QMAKE_LIB xcb_render)
endif()
qt_add_qmake_lib_dependency(xcb_render xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB MODULE
        COMPONENTS GLX PROVIDED_TARGETS XCB::GLX MODULE_NAME gui QMAKE_LIB xcb_glx)
endif()
qt_add_qmake_lib_dependency(xcb_glx xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XCB 1.12 MODULE
        COMPONENTS XINPUT PROVIDED_TARGETS XCB::XINPUT MODULE_NAME gui QMAKE_LIB xcb_xinput)
endif()
qt_add_qmake_lib_dependency(xcb_xinput xcb)
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XKB 0.9.0 MODULE PROVIDED_TARGETS XKB::XKB MODULE_NAME gui QMAKE_LIB xkbcommon)
endif()
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XKB_COMMON_X11 0.9.0 MODULE
        PROVIDED_TARGETS PkgConfig::XKB_COMMON_X11 MODULE_NAME gui QMAKE_LIB xkbcommon_x11)
endif()
if((X11_SUPPORTED) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(XRender 0.6 MODULE
        PROVIDED_TARGETS PkgConfig::XRender MODULE_NAME gui QMAKE_LIB xrender)
endif()
qt_add_qmake_lib_dependency(xrender xlib)

# qt wayland client
if(LINUX OR FREEBSD OR QT_FIND_ALL_PACKAGES_ALWAYS)
    # EGL
    if(NOT TARGET EGL::EGL)
        qt_find_package(EGL MODULE
            PROVIDED_TARGETS EGL::EGL MODULE_NAME gui QMAKE_LIB egl MARK_OPTIONAL)
    endif()
    # and Libdrm
    if(NOT TARGET Libdrm::Libdrm)
        qt_find_package(Libdrm MODULE
            PROVIDED_TARGETS Libdrm::Libdrm
            MODULE_NAME gui
            QMAKE_LIB drm
            MARK_OPTIONAL)
    endif()
endif()

qt_find_package(Wayland 1.15 MODULE)
qt_find_package(WaylandScanner MODULE PROVIDED_TARGETS Wayland::Scanner)

qt_find_package(RenderDoc MODULE PROVIDED_TARGETS RenderDoc::RenderDoc)

#### Tests

if(TARGET PlatformGraphics::PlatformGraphics)
    set(plaform_graphics_libs PlatformGraphics::PlatformGraphics)
else()
    set(plaform_graphics_libs "")
endif()

# drm_atomic
qt_config_compile_test(drm_atomic
    LABEL "DRM Atomic API"
    LIBRARIES
        Libdrm::Libdrm
    CODE
"#include <stdlib.h>
#include <stdint.h>
extern \"C\" {
#include <xf86drmMode.h>
#include <xf86drm.h>
}

int main(void)
{
    /* BEGIN TEST: */
drmModeAtomicReq *request;
    /* END TEST: */
    return 0;
}
")

# egl-x11
qt_config_compile_test(egl_x11
    LABEL "EGL on X11"
    LIBRARIES
        EGL::EGL
        X11::X11
        ${plaform_graphics_libs}
    CODE
"// Check if EGL is compatible with X. Some EGL implementations, typically on
// embedded devices, are not intended to be used together with X. EGL support
// has to be disabled in plugins like xcb in this case since the native display,
// window and pixmap types will be different than what an X-based platform
// plugin would expect.
#define USE_X11
#include <EGL/egl.h>
#include <X11/Xlib.h>

int main(void)
{
    /* BEGIN TEST: */
Display *dpy = EGL_DEFAULT_DISPLAY;
EGLNativeDisplayType egldpy = XOpenDisplay(\"\");
dpy = egldpy;
EGLNativeWindowType w = XCreateWindow(dpy, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
XDestroyWindow(dpy, w);
XCloseDisplay(dpy);
    /* END TEST: */
    return 0;
}
")

# egl-brcm
qt_config_compile_test(egl_brcm
    LABEL "Broadcom EGL (Raspberry Pi)"
    LIBRARIES
        EGL::EGL
        ${plaform_graphics_libs}
    CODE
"#include <EGL/egl.h>
#include <bcm_host.h>

int main(void)
{
    /* BEGIN TEST: */
vc_dispmanx_display_open(0);
    /* END TEST: */
    return 0;
}
"# FIXME: use: unmapped library: bcm_host
)

# egl-egldevice
qt_config_compile_test(egl_egldevice
    LABEL "EGLDevice"
    LIBRARIES
        EGL::EGL
        ${plaform_graphics_libs}
    CODE
"#include <EGL/egl.h>
#include <EGL/eglext.h>

int main(void)
{
    /* BEGIN TEST: */
EGLDeviceEXT device = 0;
EGLStreamKHR stream = 0;
EGLOutputLayerEXT layer = 0;
(void) EGL_DRM_CRTC_EXT;
(void) EGL_DRM_MASTER_FD_EXT;
    /* END TEST: */
    return 0;
}
")

# egl-mali
qt_config_compile_test(egl_mali
    LABEL "Mali EGL"
    LIBRARIES
        EGL::EGL
        ${plaform_graphics_libs}
    CODE
"#include <EGL/fbdev_window.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

int main(void)
{
    /* BEGIN TEST: */
fbdev_window *w = 0;
    /* END TEST: */
    return 0;
}
")

# egl-mali-2
qt_config_compile_test(egl_mali_2
    LABEL "Mali 2 EGL"
    LIBRARIES
        EGL::EGL
        ${plaform_graphics_libs}
    CODE
"#include <EGL/egl.h>
#include <GLES2/gl2.h>

int main(void)
{
    /* BEGIN TEST: */
mali_native_window *w = 0;
    /* END TEST: */
    return 0;
}
")

# egl-viv

qt_config_compile_test(egl_viv
    LABEL "i.Mx6 EGL"
    LIBRARIES
        EGL::EGL
        ${plaform_graphics_libs}
    COMPILE_OPTIONS
        "-DEGL_API_FB=1"
    CODE
"#include <EGL/egl.h>
#include <EGL/eglvivante.h>

int main(void)
{
    /* BEGIN TEST: */
#ifdef __INTEGRITY
fbGetDisplay();
#else
// Do not rely on fbGetDisplay(), since the signature has changed over time.
// Stick to fbGetDisplayByIndex().
fbGetDisplayByIndex(0);
#endif
    /* END TEST: */
    return 0;
}
"# FIXME: qmake: ['DEFINES += EGL_API_FB=1', '!integrity: DEFINES += LINUX=1']
)

# egl-openwfd
qt_config_compile_test(egl_openwfd
    LABEL "OpenWFD EGL"
    LIBRARIES
        EGL::EGL
        ${plaform_graphics_libs}
    CODE
"#include <wfd.h>

int main(void)
{
    /* BEGIN TEST: */
wfdEnumerateDevices(nullptr, 0, nullptr);
    /* END TEST: */
    return 0;
}
")

# egl-rcar
qt_config_compile_test(egl_rcar
    LABEL "RCAR EGL"
    LIBRARIES
        EGL::EGL
        GLESv2::GLESv2
        ${plaform_graphics_libs}
    CODE
"#include <EGL/egl.h>
extern \"C\" {
extern unsigned long PVRGrfxServerInit(void);
}

int main(void)
{
    /* BEGIN TEST: */
PVRGrfxServerInit();
    /* END TEST: */
    return 0;
}
")

# evdev
qt_config_compile_test(evdev
    LABEL "evdev"
    CODE
"#if defined(__FreeBSD__)
#  include <dev/evdev/input.h>
#else
#  include <linux/input.h>
#  include <linux/kd.h>
#endif
enum {
    e1 = ABS_PRESSURE,
    e2 = ABS_X,
    e3 = REL_X,
    e4 = SYN_REPORT,
};

int main(void)
{
    /* BEGIN TEST: */
input_event buf[32];
(void) buf;
    /* END TEST: */
    return 0;
}
")

# vxworksevdev
qt_config_compile_test(vxworksevdev
    LABEL "VxWorks evdev"
"#include <evdevLib.h>
enum {
    e1 = EV_DEV_ABS,
    e2 = EV_DEV_PTR_ABS_X,
    e3 = EV_DEV_PTR_ABS_Y,
    e4 = EV_DEV_PTR_BTN_TOUCH,
};

int main(void)
{
    /* BEGIN TEST: */
EV_DEV_EVENT buf[32];
(void) buf;
    /* END TEST: */
    return 0;
}")

# integrityfb
qt_config_compile_test(integrityfb
    LABEL "INTEGRITY framebuffer"
    CODE
"#include <device/fbdriver.h>

int main(void)
{
    /* BEGIN TEST: */
FBDriver *driver = 0;
    /* END TEST: */
    return 0;
}
")

# linuxfb
qt_config_compile_test(linuxfb
    LABEL "LinuxFB"
    CODE
"#include <linux/fb.h>
#include <sys/kd.h>
#include <sys/ioctl.h>

int main(void)
{
    /* BEGIN TEST: */
fb_fix_screeninfo finfo;
fb_var_screeninfo vinfo;
int fd = 3;
ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
    /* END TEST: */
    return 0;
}
")

# opengles3
if(WASM)
    set(extra_compiler_options "-s FULL_ES3=1")
endif()

qt_config_compile_test(opengles3
    LABEL "OpenGL ES 3.0"
    LIBRARIES
        GLESv2::GLESv2
        ${plaform_graphics_libs}
    COMPILE_OPTIONS ${extra_compiler_options}
    CODE
"#ifdef __APPLE__
#  include <OpenGLES/ES3/gl.h>
#else
#  define GL_GLEXT_PROTOTYPES
#  include <GLES3/gl3.h>
#endif

int main(void)
{
    /* BEGIN TEST: */
static GLfloat f[6];
glGetStringi(GL_EXTENSIONS, 0);
glReadBuffer(GL_COLOR_ATTACHMENT1);
glUniformMatrix2x3fv(0, 0, GL_FALSE, f);
glMapBufferRange(GL_ARRAY_BUFFER, 0, 0, GL_MAP_READ_BIT);
    /* END TEST: */
    return 0;
}
")


# opengles31
qt_config_compile_test(opengles31
    LABEL "OpenGL ES 3.1"
    LIBRARIES
        GLESv2::GLESv2
        ${plaform_graphics_libs}
    CODE
"#include <GLES3/gl31.h>

int main(void)
{
    /* BEGIN TEST: */
glDispatchCompute(1, 1, 1);
glProgramUniform1i(0, 0, 0);
    /* END TEST: */
    return 0;
}
")

# opengles32
qt_config_compile_test(opengles32
    LABEL "OpenGL ES 3.2"
    LIBRARIES
        GLESv2::GLESv2
        ${plaform_graphics_libs}
    CODE
"#include <GLES3/gl32.h>

int main(void)
{
    /* BEGIN TEST: */
glFramebufferTexture(GL_TEXTURE_2D, GL_DEPTH_STENCIL_ATTACHMENT, 1, 0);
    /* END TEST: */
    return 0;
}
")

# xcb_syslibs
qt_config_compile_test(xcb_syslibs
    LABEL "XCB (extensions)"
    LIBRARIES
        XCB::CURSOR
        XCB::ICCCM
        XCB::UTIL
        XCB::IMAGE
        XCB::KEYSYMS
        XCB::RANDR
        XCB::RENDER
        XCB::RENDERUTIL
        XCB::SHAPE
        XCB::SHM
        XCB::SYNC
        XCB::XFIXES
        XCB::XKB
        XCB::XCB
    CODE
"// xkb.h is using a variable called 'explicit', which is a reserved keyword in C++
#define explicit dont_use_cxx_explicit
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_cursor.h>
#include <xcb/randr.h>
#include <xcb/render.h>
#include <xcb/shape.h>
#include <xcb/shm.h>
#include <xcb/sync.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_renderutil.h>
#include <xcb/xkb.h>
#undef explicit

int main(void)
{
    /* BEGIN TEST: */
int primaryScreen = 0;
xcb_connection_t *c = xcb_connect(\"\", &primaryScreen);
/* RENDER */
xcb_generic_error_t *error = nullptr;
xcb_render_query_pict_formats_cookie_t formatsCookie =
    xcb_render_query_pict_formats(c);
xcb_render_query_pict_formats_reply_t *formatsReply =
    xcb_render_query_pict_formats_reply(c, formatsCookie, &error);
/* RENDERUTIL: xcb_renderutil.h include won't compile unless version >= 0.3.9 */
xcb_render_util_find_standard_format(nullptr, XCB_PICT_STANDARD_ARGB_32);
/* XKB: This takes more arguments in xcb-xkb < 1.11 */
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_sizeof(nullptr, 0, 0, 0, 0, 0, 0, 0, 0);
    /* END TEST: */
    return 0;
}
")

# libinput_hires_wheel_support
qt_config_compile_test(libinput_hires_wheel_support
    LABEL "libinput hires wheel support"
    LIBRARIES
        Libinput::Libinput
    CODE
"#include <libinput.h>
int main(void)
{
    /* BEGIN TEST: */
libinput_event_type type = LIBINPUT_EVENT_POINTER_SCROLL_WHEEL;
libinput_event_pointer_get_scroll_value_v120(nullptr, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
    /* END TEST: */
    return 0;
}
")

# directwrite (assumes DirectWrite2)
qt_config_compile_test(directwrite
    LABEL "WINDOWS directwrite"
    LIBRARIES
        dwrite
    CODE
"#include <dwrite_2.h>
int main(int, char **)
{
    IUnknown *factory = nullptr;
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2),
                        &factory);
    return 0;
}
")

# directwrite3 (not present in MinGW)
qt_config_compile_test(directwrite3
    LABEL "WINDOWS directwrite3"
    LIBRARIES
        dwrite
    CODE
"#include <dwrite_3.h>
int main(int, char **)
{
    IUnknown *factory = nullptr;
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory6),
                        &factory);
    return 0;
}
")

# directwritecolrv1
qt_config_compile_test(directwritecolrv1
    LABEL "WINDOWS directwritecolrv1"
    LIBRARIES
        dwrite
    CODE
"#include <dwrite_3.h>
int main(int, char **)
{
    IUnknown *factory = nullptr;
    // Just check that the API is available for the build
    DWRITE_PAINT_ELEMENT paintElement;

    if (false) {
        DWRITE_COLOR_F dwColor;
        dwColor.r = 0;
        dwColor.g = 0;
        dwColor.b = 0;
        dwColor.a = 0;
        IDWritePaintReader *paintReader = nullptr;
        // Some versions of MinGW has a dwrite_3.h header with buggy generated APIs. One of these
        // is that the SetTextColor() function takes a pointer instead of a const-ref. We check
        // for this to disable the COLRv1 feature for broken headers.
        paintReader->SetTextColor(dwColor);
    }

    return 0;
}
")

qt_config_compile_test(d2d1
    LABEL "WINDOWS Direct2D"
    LIBRARIES
        d2d1
    CODE
"#include <d2d1.h>
int main(int, char **)
{
    void *factory = nullptr;
    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, GUID{}, &options, &factory);
    return 0;
}
")

qt_config_compile_test(d2d1_1
    LABEL "WINDOWS Direct2D 1.1"
    LIBRARIES
        d2d1
    CODE
"#include <d2d1_1.h>
int main(int, char **)
{
    ID2D1Factory1 *d2dFactory;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory);
    return 0;
}
")

qt_config_compile_test(renderdoc
    LIBRARIES
        RenderDoc::RenderDoc
    LABEL "RenderDoc header check"
    CODE
"#include <renderdoc_app.h>
int main(int, char **)
{
    if (RENDERDOC_Version::eRENDERDOC_API_Version_1_6_0)
        return 0;
    return 0;
}
")

# qtwayland client
# drm-egl-server
qt_config_compile_test(drm_egl_server
    LABEL "DRM EGL Server"
    LIBRARIES
    EGL::EGL
    CODE
    "
#include <EGL/egl.h>
#include <EGL/eglext.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
#ifdef EGL_MESA_drm_image
return 0;
#else
#error Requires EGL_MESA_drm_image to be defined
return 1;
#endif
    /* END TEST: */
    return 0;
}
")

# libhybris-egl-server
qt_config_compile_test(libhybris_egl_server
    LABEL "libhybris EGL Server"
    LIBRARIES
    EGL::EGL
    CODE
    "
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <hybris/eglplatformcommon/hybris_nativebufferext.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
#ifdef EGL_HYBRIS_native_buffer
return 0;
#else
#error Requires EGL_HYBRIS_native_buffer to be defined
return 1;
#endif
    /* END TEST: */
    return 0;
}
")

# dmabuf-server-buffer
qt_config_compile_test(dmabuf_server_buffer
    LABEL "Linux dma-buf Buffer Sharing"
    LIBRARIES
    EGL::EGL
    Libdrm::Libdrm
    CODE
    "
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <drm_fourcc.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
#ifdef EGL_LINUX_DMA_BUF_EXT
return 0;
#else
#error Requires EGL_LINUX_DMA_BUF_EXT
return 1;
#endif
    /* END TEST: */
    return 0;
}
")

# vulkan-server-buffer
qt_config_compile_test(vulkan_server_buffer
    LABEL "Vulkan Buffer Sharing"
    LIBRARIES
    Wayland::Client
    CODE
    "#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include <vulkan/vulkan.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
VkExportMemoryAllocateInfoKHR exportAllocInfo = {};
exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
return 0;
    /* END TEST: */
    return 0;
}
")

# egl_1_5-wayland
qt_config_compile_test(egl_1_5_wayland
    LABEL "EGL 1.5 with Wayland Platform"
    LIBRARIES
    EGL::EGL
    Wayland::Client
    CODE
    "
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, (struct wl_display *)(nullptr), nullptr);
    /* END TEST: */
    return 0;
}
")

#### Features

qt_feature("accessibility-atspi-bridge" PUBLIC PRIVATE
    LABEL "ATSPI Bridge"
    CONDITION QT_FEATURE_accessibility AND QT_FEATURE_dbus AND ATSPI2_FOUND
)
qt_feature_definition("accessibility-atspi-bridge" "QT_NO_ACCESSIBILITY_ATSPI_BRIDGE" NEGATE VALUE "1")
qt_feature("directfb" PRIVATE
    SECTION "Platform plugins"
    LABEL "DirectFB"
    AUTODETECT OFF
    CONDITION DirectFB_FOUND
)
qt_feature("directwrite" PRIVATE
    LABEL "DirectWrite"
    CONDITION TEST_directwrite
    EMIT_IF WIN32
)
qt_feature("directwrite3" PRIVATE
    LABEL "DirectWrite 3"
    CONDITION QT_FEATURE_directwrite AND TEST_directwrite3
    EMIT_IF WIN32
)
qt_feature("directwritecolrv1" PRIVATE
    LABEL "DirectWrite COLRv1 Support"
    CONDITION QT_FEATURE_directwrite3 AND TEST_directwritecolrv1
    EMIT_IF WIN32
)
qt_feature("direct2d" PRIVATE
    LABEL "Direct 2D"
    CONDITION WIN32 AND NOT WINRT AND TEST_d2d1
)
qt_feature("direct2d1_1" PRIVATE
    LABEL "Direct 2D 1.1"
    CONDITION QT_FEATURE_direct2d AND TEST_d2d1_1
)
qt_feature("emojisegmenter" PUBLIC PRIVATE
    SECTION "Fonts"
    LABEL "Emoji Segmenter"
    PURPOSE "Supports parsing complex emoji sequences for better font resolution."
)
qt_feature_definition("emojisegmenter" "QT_NO_EMOJISEGMENTER" NEGATE VALUE "1")
qt_feature("evdev" PRIVATE
    LABEL "evdev"
    CONDITION QT_FEATURE_thread AND TEST_evdev
)
qt_feature("vxworksevdev" PRIVATE
    LABEL "vxworksevdev"
    CONDITION QT_FEATURE_thread AND TEST_vxworksevdev
)
qt_feature("freetype" PUBLIC PRIVATE
    SECTION "Fonts"
    LABEL "FreeType"
    PURPOSE "Supports the FreeType 2 font engine (and its supported font formats)."
)
qt_feature_definition("freetype" "QT_NO_FREETYPE" NEGATE VALUE "1")
qt_feature("system-freetype" PRIVATE SYSTEM_LIBRARY
    LABEL "  Using system FreeType"
    AUTODETECT NOT MSVC
    CONDITION QT_FEATURE_freetype AND WrapSystemFreetype_FOUND
    ENABLE INPUT_freetype STREQUAL 'system'
    DISABLE INPUT_freetype STREQUAL 'qt'
)
qt_feature("fontconfig" PUBLIC PRIVATE
    LABEL "Fontconfig"
    AUTODETECT NOT APPLE
    CONDITION NOT APPLE AND NOT WIN32 AND QT_FEATURE_system_freetype AND Fontconfig_FOUND
)
qt_feature_definition("fontconfig" "QT_NO_FONTCONFIG" NEGATE VALUE "1")
qt_feature("gbm"
    LABEL "GBM"
    CONDITION gbm_FOUND
)
qt_feature_config("gbm" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("harfbuzz" PUBLIC PRIVATE
    LABEL "HarfBuzz"
)
qt_feature_definition("harfbuzz" "QT_NO_HARFBUZZ" NEGATE VALUE "1")
qt_feature("system-harfbuzz" PRIVATE SYSTEM_LIBRARY
    LABEL "  Using system HarfBuzz"
    AUTODETECT NOT APPLE AND NOT WIN32
    CONDITION QT_FEATURE_harfbuzz AND WrapSystemHarfbuzz_FOUND
    ENABLE INPUT_harfbuzz STREQUAL 'system'
    DISABLE INPUT_harfbuzz STREQUAL 'qt'
)
qt_feature("qqnx_imf" PRIVATE
    LABEL "IMF"
    CONDITION libs.imf OR FIXME
    EMIT_IF QNX
)
qt_feature("integrityfb" PRIVATE
    SECTION "Platform plugins"
    LABEL "INTEGRITY framebuffer"
    CONDITION INTEGRITY AND TEST_integrityfb
)
qt_feature("kms" PRIVATE
    LABEL "KMS"
    CONDITION Libdrm_FOUND
)
qt_feature_config("kms" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("drm_atomic" PRIVATE
    LABEL "DRM Atomic API"
    CONDITION Libdrm_FOUND AND TEST_drm_atomic
)
qt_feature("libinput" PRIVATE
    LABEL "libinput"
    CONDITION QT_FEATURE_libudev AND Libinput_FOUND
)
qt_feature("integrityhid" PRIVATE
    LABEL "INTEGRITY HID"
    CONDITION INTEGRITY AND libs.integrityhid OR FIXME
)
qt_feature("libinput-axis-api" PRIVATE
    LABEL "axis API in libinput"
    CONDITION QT_FEATURE_libinput AND ON
)
qt_feature("libinput-hires-wheel-support" PRIVATE
    LABEL "HiRes wheel support in libinput"
    CONDITION QT_FEATURE_libinput AND TEST_libinput_hires_wheel_support
)
qt_feature("lgmon"
    LABEL "lgmon"
    CONDITION libs.lgmon OR FIXME
    EMIT_IF QNX
)
qt_feature_config("lgmon" QMAKE_PRIVATE_CONFIG)
qt_feature("linuxfb" PRIVATE
    SECTION "Platform plugins"
    LABEL "LinuxFB"
    CONDITION TEST_linuxfb AND QT_FEATURE_regularexpression
)
qt_feature("vsp2" PRIVATE
    LABEL "VSP2"
    AUTODETECT OFF
    CONDITION libs.v4l2 OR FIXME
)
qt_feature("vnc" PRIVATE
    SECTION "Platform plugins"
    LABEL "VNC"
    CONDITION ( UNIX AND NOT ANDROID AND NOT APPLE AND NOT WASM ) AND ( QT_FEATURE_regularexpression AND QT_FEATURE_network )
)
qt_feature("mtdev" PRIVATE
    LABEL "mtdev"
    CONDITION Mtdev_FOUND
)
qt_feature("opengles2" PUBLIC
    LABEL "OpenGL ES 2.0"
    CONDITION NOT WIN32 AND NOT WATCHOS AND NOT QT_FEATURE_opengl_desktop AND GLESv2_FOUND
    ENABLE INPUT_opengl STREQUAL 'es2'
    DISABLE INPUT_opengl STREQUAL 'desktop' OR INPUT_opengl STREQUAL 'dynamic' OR INPUT_opengl STREQUAL 'no'
)
qt_feature_config("opengles2" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("opengles3" PUBLIC
    LABEL "OpenGL ES 3.0"
    CONDITION QT_FEATURE_opengles2 AND TEST_opengles3
)
qt_feature("opengles31" PUBLIC
    LABEL "OpenGL ES 3.1"
    CONDITION QT_FEATURE_opengles3 AND TEST_opengles31
)
qt_feature("opengles32" PUBLIC
    LABEL "OpenGL ES 3.2"
    CONDITION QT_FEATURE_opengles31 AND TEST_opengles32
)
qt_feature("opengl-desktop"
    LABEL "Desktop OpenGL"
    AUTODETECT NOT WIN32
    CONDITION ( WIN32 AND ( MSVC OR WrapOpenGL_FOUND ) ) OR ( NOT WATCHOS AND NOT WIN32 AND NOT WASM AND WrapOpenGL_FOUND )
    ENABLE INPUT_opengl STREQUAL 'desktop'
    DISABLE INPUT_opengl STREQUAL 'es2' OR INPUT_opengl STREQUAL 'dynamic' OR INPUT_opengl STREQUAL 'no'
)
qt_feature("opengl-dynamic"
    LABEL "Dynamic OpenGL"
    CONDITION WIN32
    DISABLE INPUT_opengl STREQUAL 'no' OR INPUT_opengl STREQUAL 'desktop'
)
qt_feature("dynamicgl" PUBLIC
    LABEL "Dynamic OpenGL: dynamicgl"
    CONDITION QT_FEATURE_opengl_dynamic
    DISABLE INPUT_opengl STREQUAL 'no' OR INPUT_opengl STREQUAL 'desktop'
)
qt_feature_definition("opengl-dynamic" "QT_OPENGL_DYNAMIC")
qt_feature("opengl" PUBLIC
    LABEL "OpenGL"
    CONDITION QT_FEATURE_opengl_desktop OR QT_FEATURE_opengl_dynamic OR QT_FEATURE_opengles2
)
qt_feature_definition("opengl" "QT_NO_OPENGL" NEGATE VALUE "1")
qt_feature("vkgen" PRIVATE
    LABEL "vkgen"
    CONDITION QT_FEATURE_xmlstreamreader
)
qt_feature("vulkan" PUBLIC
    LABEL "Vulkan"
    CONDITION QT_FEATURE_library AND QT_FEATURE_vkgen AND WrapVulkanHeaders_FOUND
)
qt_feature("metal" PUBLIC
    LABEL "Metal"
    CONDITION MACOS OR IOS OR VISIONOS
)
qt_feature("vkkhrdisplay" PRIVATE
    SECTION "Platform plugins"
    LABEL "VK_KHR_display"
    CONDITION NOT ANDROID AND NOT APPLE AND NOT WIN32 AND NOT WASM AND QT_FEATURE_vulkan
)
qt_feature("openvg" PUBLIC
    LABEL "OpenVG"
    CONDITION libs.openvg OR FIXME
)
qt_feature("egl" PUBLIC
    LABEL "EGL"
    CONDITION ( QT_FEATURE_opengl OR QT_FEATURE_openvg ) AND EGL_FOUND AND ( QT_FEATURE_dlopen OR NOT UNIX OR INTEGRITY OR VXWORKS)
)
qt_feature_definition("egl" "QT_NO_EGL" NEGATE VALUE "1")
qt_feature("egl_x11" PRIVATE
    LABEL "EGL on X11"
    CONDITION QT_FEATURE_thread AND QT_FEATURE_egl AND TEST_egl_x11
)
qt_feature("eglfs" PRIVATE
    SECTION "Platform plugins"
    LABEL "EGLFS"
    CONDITION NOT ANDROID AND NOT APPLE AND NOT WIN32 AND NOT WASM AND NOT QNX AND QT_FEATURE_egl
)
qt_feature("eglfs_brcm" PRIVATE
    LABEL "EGLFS Raspberry Pi"
    CONDITION QT_FEATURE_eglfs AND TEST_egl_brcm
)
qt_feature("eglfs_egldevice" PRIVATE
    LABEL "EGLFS EGLDevice"
    CONDITION QT_FEATURE_eglfs AND TEST_egl_egldevice AND QT_FEATURE_kms
)
qt_feature("eglfs_gbm" PRIVATE
    LABEL "EGLFS GBM"
    CONDITION QT_FEATURE_eglfs AND gbm_FOUND AND QT_FEATURE_kms
)
qt_feature("eglfs_vsp2" PRIVATE
    LABEL "EGLFS VSP2"
    CONDITION QT_FEATURE_eglfs AND gbm_FOUND AND QT_FEATURE_kms AND QT_FEATURE_vsp2
)
qt_feature("eglfs_mali" PRIVATE
    LABEL "EGLFS Mali"
    CONDITION QT_FEATURE_eglfs AND ( TEST_egl_mali OR TEST_egl_mali_2 )
)
qt_feature("eglfs_viv" PRIVATE
    LABEL "EGLFS i.Mx6"
    CONDITION QT_FEATURE_eglfs AND TEST_egl_viv
)
qt_feature("eglfs_rcar" PRIVATE
    LABEL "EGLFS RCAR"
    CONDITION INTEGRITY AND QT_FEATURE_eglfs AND TEST_egl_rcar
)
qt_feature("eglfs_viv_wl" PRIVATE
    LABEL "EGLFS i.Mx6 Wayland"
    CONDITION QT_FEATURE_eglfs_viv AND TARGET Wayland::Server
)
qt_feature("eglfs_openwfd" PRIVATE
    LABEL "EGLFS OpenWFD"
    CONDITION INTEGRITY AND QT_FEATURE_eglfs AND TEST_egl_openwfd
)
qt_feature("eglfs_x11" PRIVATE
    LABEL "EGLFS X11"
    CONDITION QT_FEATURE_eglfs AND QT_FEATURE_xcb_xlib AND QT_FEATURE_egl_x11
)
qt_feature("gif" PRIVATE
    LABEL "GIF"
    CONDITION QT_FEATURE_imageformatplugin
)
qt_feature_definition("gif" "QT_NO_IMAGEFORMAT_GIF" NEGATE)
qt_feature("ico" PUBLIC PRIVATE
    LABEL "ICO"
    CONDITION QT_FEATURE_imageformatplugin
)
qt_feature_definition("ico" "QT_NO_ICO" NEGATE VALUE "1")
qt_feature("jpeg" PRIVATE
    LABEL "JPEG"
    CONDITION QT_FEATURE_imageformatplugin
    DISABLE INPUT_libjpeg STREQUAL 'no'
)
qt_feature_definition("jpeg" "QT_NO_IMAGEFORMAT_JPEG" NEGATE VALUE "1")
qt_feature("system-jpeg" PRIVATE SYSTEM_LIBRARY
    LABEL "  Using system libjpeg"
    CONDITION QT_FEATURE_jpeg AND JPEG_FOUND
    ENABLE INPUT_libjpeg STREQUAL 'system'
    DISABLE INPUT_libjpeg STREQUAL 'qt'
)
qt_feature("png" PRIVATE
    LABEL "PNG"
    DISABLE INPUT_libpng STREQUAL 'no'
)
qt_feature_definition("png" "QT_NO_IMAGEFORMAT_PNG" NEGATE)
qt_feature("system-png" PRIVATE SYSTEM_LIBRARY
    LABEL "  Using system libpng"
    AUTODETECT QT_FEATURE_system_zlib
    CONDITION QT_FEATURE_png AND WrapSystemPNG_FOUND
    ENABLE INPUT_libpng STREQUAL 'system'
    DISABLE INPUT_libpng STREQUAL 'qt'
)
qt_feature("imageio-text-loading" PRIVATE
    LABEL "Image Text section loading"
)
qt_feature_definition("imageio-text-loading" "QT_NO_IMAGEIO_TEXT_LOADING" NEGATE)
qt_feature("sessionmanager" PUBLIC
    SECTION "Kernel"
    LABEL "Session Management"
    PURPOSE "Provides an interface to the windowing system's session management."
)
qt_feature_definition("sessionmanager" "QT_NO_SESSIONMANAGER" NEGATE VALUE "1")
qt_feature("tslib" PRIVATE
    LABEL "tslib"
    CONDITION Tslib_FOUND AND NOT INTEGRITY
)
qt_feature("tuiotouch" PRIVATE
    LABEL "TuioTouch"
    PURPOSE "Provides the TuioTouch input plugin."
    CONDITION QT_FEATURE_network AND QT_FEATURE_udpsocket
)
qt_feature("xcb" PUBLIC
    SECTION "Platform plugins"
    LABEL "XCB"
    AUTODETECT NOT APPLE
    CONDITION QT_FEATURE_thread AND TARGET XCB::XCB AND TEST_xcb_syslibs AND QT_FEATURE_xkbcommon_x11
)
qt_feature("xcb-glx-plugin" PUBLIC
    LABEL "GLX Plugin"
    CONDITION QT_FEATURE_xcb_xlib AND QT_FEATURE_opengl AND NOT QT_FEATURE_opengles2
    EMIT_IF QT_FEATURE_xcb
)
qt_feature("xcb-glx" PRIVATE
    LABEL "  XCB GLX"
    CONDITION XCB_GLX_FOUND
    EMIT_IF QT_FEATURE_xcb AND QT_FEATURE_xcb_glx_plugin
)
qt_feature("xcb-egl-plugin" PRIVATE
    LABEL "EGL-X11 Plugin"
    CONDITION QT_FEATURE_egl AND QT_FEATURE_opengl
    EMIT_IF QT_FEATURE_xcb
)
qt_feature("xcb-native-painting" PRIVATE
    LABEL "Native painting (experimental)"
    AUTODETECT OFF
    CONDITION QT_FEATURE_xcb_xlib AND QT_FEATURE_fontconfig AND XRender_FOUND
    EMIT_IF QT_FEATURE_xcb
)
qt_feature("xrender" PRIVATE
    LABEL "XRender for native painting"
    CONDITION QT_FEATURE_xcb_native_painting
    EMIT_IF QT_FEATURE_xcb AND QT_FEATURE_xcb_native_painting
)
qt_feature("xcb-xlib" PRIVATE
    LABEL "XCB Xlib"
    CONDITION QT_FEATURE_xlib AND X11_XCB_FOUND
)
qt_feature("xcb-sm" PRIVATE
    LABEL "xcb-sm"
    CONDITION QT_FEATURE_sessionmanager AND X11_SM_FOUND
    EMIT_IF QT_FEATURE_xcb
)
qt_feature("system-xcb-xinput" PRIVATE SYSTEM_LIBRARY
    LABEL "Using system-provided xcb-xinput"
    AUTODETECT OFF
    CONDITION XCB_XINPUT_FOUND
    ENABLE INPUT_bundled_xcb_xinput STREQUAL 'no'
    DISABLE INPUT_bundled_xcb_xinput STREQUAL 'yes'
    EMIT_IF QT_FEATURE_xcb
)
qt_feature("xkbcommon" PRIVATE
    LABEL "xkbcommon"
    CONDITION XKB_FOUND
)
qt_feature("xkbcommon-x11" PRIVATE
    LABEL "xkbcommon-x11"
    CONDITION QT_FEATURE_xkbcommon AND XKB_COMMON_X11_FOUND
)
qt_feature("xlib" PRIVATE
    LABEL "XLib"
    AUTODETECT NOT APPLE OR QT_FEATURE_xcb
    CONDITION X11_FOUND
)
qt_feature("texthtmlparser" PUBLIC
    SECTION "Kernel"
    LABEL "HtmlParser"
    PURPOSE "Provides a parser for HTML."
)
qt_feature_definition("texthtmlparser" "QT_NO_TEXTHTMLPARSER" NEGATE VALUE "1")
qt_feature("textmarkdownreader" PUBLIC
    SECTION "Kernel"
    LABEL "MarkdownReader"
    PURPOSE "Provides a Markdown (CommonMark and GitHub) reader"
    ENABLE INPUT_libmd4c STREQUAL 'system' OR INPUT_libmd4c STREQUAL 'qt' OR INPUT_libmd4c STREQUAL 'yes'
    DISABLE INPUT_libmd4c STREQUAL 'no'
)
qt_feature("system-textmarkdownreader" PUBLIC SYSTEM_LIBRARY
    SECTION "Kernel"
    LABEL "  Using system libmd4c"
    CONDITION QT_FEATURE_textmarkdownreader AND WrapSystemMd4c_FOUND
    ENABLE INPUT_libmd4c STREQUAL 'system'
    DISABLE INPUT_libmd4c STREQUAL 'qt'
)
qt_feature("textmarkdownwriter" PUBLIC
    SECTION "Kernel"
    LABEL "MarkdownWriter"
    CONDITION QT_FEATURE_regularexpression
    PURPOSE "Provides a Markdown (CommonMark and GitHub) writer"
)
qt_feature("textodfwriter" PUBLIC
    SECTION "Kernel"
    LABEL "OdfWriter"
    PURPOSE "Provides an ODF writer."
    CONDITION QT_FEATURE_xmlstreamwriter
)
qt_feature_definition("textodfwriter" "QT_NO_TEXTODFWRITER" NEGATE VALUE "1")
qt_feature("cssparser" PUBLIC
    SECTION "Kernel"
    LABEL "CssParser"
    PURPOSE "Provides a parser for Cascading Style Sheets."
)
qt_feature_definition("cssparser" "QT_NO_CSSPARSER" NEGATE VALUE "1")
qt_feature("draganddrop" PUBLIC
    SECTION "Kernel"
    LABEL "Drag and Drop"
    PURPOSE "Supports the drag and drop mechanism."
    CONDITION QT_FEATURE_imageformat_xpm
)
qt_feature_definition("draganddrop" "QT_NO_DRAGANDDROP" NEGATE VALUE "1")
qt_feature("action" PUBLIC
    SECTION "Kernel"
    LABEL "Q(Gui)Action(Group)"
    PURPOSE "Provides abstract user interface actions."
)
qt_feature_definition("action" "QT_NO_ACTION" NEGATE VALUE "1")
qt_feature("cursor" PUBLIC
    SECTION "Kernel"
    LABEL "QCursor"
    PURPOSE "Provides mouse cursors."
)
qt_feature_definition("cursor" "QT_NO_CURSOR" NEGATE VALUE "1")
qt_feature("clipboard" PUBLIC
    SECTION "Kernel"
    LABEL "QClipboard"
    PURPOSE "Provides cut and paste operations."
    CONDITION NOT INTEGRITY AND NOT QNX AND NOT rtems
)
qt_feature_definition("clipboard" "QT_NO_CLIPBOARD" NEGATE VALUE "1")
qt_feature("wheelevent" PUBLIC
    SECTION "Kernel"
    LABEL "QWheelEvent"
    PURPOSE "Supports wheel events."
)
qt_feature_definition("wheelevent" "QT_NO_WHEELEVENT" NEGATE VALUE "1")
qt_feature("tabletevent" PUBLIC
    SECTION "Kernel"
    LABEL "QTabletEvent"
    PURPOSE "Supports tablet events."
)
qt_feature_definition("tabletevent" "QT_NO_TABLETEVENT" NEGATE VALUE "1")
qt_feature("im" PUBLIC
    SECTION "Kernel"
    LABEL "QInputContext"
    PURPOSE "Provides complex input methods."
)
qt_feature_definition("im" "QT_NO_IM" NEGATE VALUE "1")
qt_feature("highdpiscaling" PUBLIC
    SECTION "Kernel"
    LABEL "High DPI Scaling"
    PURPOSE "Provides automatic scaling of DPI-unaware applications on high-DPI displays."
)
qt_feature_definition("highdpiscaling" "QT_NO_HIGHDPISCALING" NEGATE VALUE "1")
qt_feature("validator" PUBLIC
    SECTION "Widgets"
    LABEL "QValidator"
    PURPOSE "Supports validation of input text."
)
qt_feature_definition("validator" "QT_NO_VALIDATOR" NEGATE VALUE "1")
qt_feature("standarditemmodel" PUBLIC
    SECTION "ItemViews"
    LABEL "QStandardItemModel"
    PURPOSE "Provides a generic model for storing custom data."
    CONDITION QT_FEATURE_itemmodel
)
qt_feature_definition("standarditemmodel" "QT_NO_STANDARDITEMMODEL" NEGATE VALUE "1")
qt_feature("filesystemmodel" PUBLIC
    SECTION "File I/O"
    LABEL "QFileSystemModel"
    PURPOSE "Provides a data model for the local filesystem."
    CONDITION QT_FEATURE_itemmodel
)
qt_feature_definition("filesystemmodel" "QT_NO_FILESYSTEMMODEL" NEGATE VALUE "1")
qt_feature("imageformatplugin" PUBLIC
    SECTION "Images"
    LABEL "QImageIOPlugin"
    PURPOSE "Provides a base for writing a image format plugins."
)
qt_feature_definition("imageformatplugin" "QT_NO_IMAGEFORMATPLUGIN" NEGATE VALUE "1")
qt_feature("movie" PUBLIC
    SECTION "Images"
    LABEL "QMovie"
    PURPOSE "Supports animated images."
)
qt_feature_definition("movie" "QT_NO_MOVIE" NEGATE VALUE "1")
qt_feature("imageformat_bmp" PUBLIC
    SECTION "Images"
    LABEL "BMP Image Format"
    PURPOSE "Supports Microsoft's Bitmap image file format."
)
qt_feature_definition("imageformat_bmp" "QT_NO_IMAGEFORMAT_BMP" NEGATE VALUE "1")
qt_feature("imageformat_ppm" PUBLIC
    SECTION "Images"
    LABEL "PPM Image Format"
    PURPOSE "Supports the Portable Pixmap image file format."
)
qt_feature_definition("imageformat_ppm" "QT_NO_IMAGEFORMAT_PPM" NEGATE VALUE "1")
qt_feature("imageformat_xbm" PUBLIC
    SECTION "Images"
    LABEL "XBM Image Format"
    PURPOSE "Supports the X11 Bitmap image file format."
)
qt_feature_definition("imageformat_xbm" "QT_NO_IMAGEFORMAT_XBM" NEGATE VALUE "1")
qt_feature("imageformat_xpm" PUBLIC
    SECTION "Images"
    LABEL "XPM Image Format"
    PURPOSE "Supports the X11 Pixmap image file format."
)
qt_feature_definition("imageformat_xpm" "QT_NO_IMAGEFORMAT_XPM" NEGATE VALUE "1")
qt_feature("imageformat_png" PUBLIC
    SECTION "Images"
    LABEL "PNG Image Format"
    PURPOSE "Supports the Portable Network Graphics image file format."
)
qt_feature_definition("imageformat_png" "QT_NO_IMAGEFORMAT_PNG" NEGATE VALUE "1")
qt_feature("imageformat_jpeg" PUBLIC
    SECTION "Images"
    LABEL "JPEG Image Format"
    PURPOSE "Supports the Joint Photographic Experts Group image file format."
)
qt_feature_definition("imageformat_jpeg" "QT_NO_IMAGEFORMAT_JPEG" NEGATE VALUE "1")
qt_feature("image_heuristic_mask" PUBLIC
    SECTION "Images"
    LABEL "QImage::createHeuristicMask()"
    PURPOSE "Supports creating a 1-bpp heuristic mask for images."
)
qt_feature_definition("image_heuristic_mask" "QT_NO_IMAGE_HEURISTIC_MASK" NEGATE VALUE "1")
qt_feature("image_text" PUBLIC
    SECTION "Images"
    LABEL "Image Text"
    PURPOSE "Supports image file text strings."
)
qt_feature_definition("image_text" "QT_NO_IMAGE_TEXT" NEGATE VALUE "1")
qt_feature("picture" PUBLIC
    SECTION "Painting"
    LABEL "QPicture"
    PURPOSE "Supports recording and replaying QPainter commands."
)
qt_feature_definition("picture" "QT_NO_PICTURE" NEGATE VALUE "1")
qt_feature("colornames" PUBLIC
    SECTION "Painting"
    LABEL "Color Names"
    PURPOSE "Supports color names such as \"red\", used by QColor and by some HTML documents."
)
qt_feature_definition("colornames" "QT_NO_COLORNAMES" NEGATE VALUE "1")
qt_feature("pdf" PUBLIC
    SECTION "Painting"
    LABEL "QPdf"
    PURPOSE "Provides a PDF backend for QPainter."
    CONDITION QT_FEATURE_temporaryfile
)
qt_feature_definition("pdf" "QT_NO_PDF" NEGATE VALUE "1")
qt_feature("desktopservices" PUBLIC
    SECTION "Utilities"
    LABEL "QDesktopServices"
    PURPOSE "Provides methods for accessing common desktop services."
)
qt_feature_definition("desktopservices" "QT_NO_DESKTOPSERVICES" NEGATE VALUE "1")
qt_feature("systemtrayicon" PUBLIC
    SECTION "Utilities"
    LABEL "QSystemTrayIcon"
    PURPOSE "Provides an icon for an application in the system tray."
    CONDITION QT_FEATURE_temporaryfile
)
qt_feature_definition("systemtrayicon" "QT_NO_SYSTEMTRAYICON" NEGATE VALUE "1")
qt_feature("accessibility" PUBLIC
    SECTION "Utilities"
    LABEL "Accessibility"
    PURPOSE "Provides accessibility support."
)
qt_feature_definition("accessibility" "QT_NO_ACCESSIBILITY" NEGATE VALUE "1")
qt_feature("multiprocess" PRIVATE
    SECTION "Utilities"
    LABEL "Multi process"
    PURPOSE "Provides support for detecting the desktop environment, launching external processes and opening URLs."
    CONDITION NOT INTEGRITY AND NOT rtems
)
qt_feature("whatsthis" PUBLIC
    SECTION "Widget Support"
    LABEL "QWhatsThis"
    PURPOSE "Supports displaying \"What's this\" help."
)
qt_feature_definition("whatsthis" "QT_NO_WHATSTHIS" NEGATE VALUE "1")
qt_feature("raster-64bit" PRIVATE
    SECTION "Painting"
    LABEL "QPainter - 64 bit raster"
    PURPOSE "Internal painting support for 64 bit (16 bpc) rasterization."
)
qt_feature("raster-fp" PRIVATE
    SECTION "Painting"
    LABEL "QPainter - floating point raster"
    PURPOSE "Internal painting support for floating point rasterization."
    CONDITION NOT VXWORKS # QTBUG-115777
)

qt_feature("qtgui-threadpool" PRIVATE
    SECTION "Painting"
    LABEL "Multi-threaded image and painting helpers"
    PURPOSE "Multi-threaded image transforms and QPainter fills."
    CONDITION QT_FEATURE_thread AND NOT WASM
)

qt_feature("undocommand" PUBLIC
    SECTION "Utilities"
    LABEL "QUndoCommand"
    PURPOSE "Applies (redo or) undo of a single change in a document."
)
qt_feature_definition("undocommand" "QT_NO_UNDOCOMMAND" NEGATE VALUE "1")
qt_feature("undostack" PUBLIC
    SECTION "Utilities"
    LABEL "QUndoStack"
    PURPOSE "Provides the ability to (redo or) undo a list of changes in a document."
    CONDITION QT_FEATURE_undocommand
)
qt_feature_definition("undostack" "QT_NO_UNDOSTACK" NEGATE VALUE "1")
qt_feature("undogroup" PUBLIC
    SECTION "Utilities"
    LABEL "QUndoGroup"
    PURPOSE "Provides the ability to cluster QUndoCommands."
    CONDITION QT_FEATURE_undostack
)
qt_feature("graphicsframecapture" PRIVATE
    SECTION "Utilities"
    LABEL "QGraphicsFrameCapture"
    PURPOSE "Provides a way to capture 3D graphics API calls for a rendered frame."
    CONDITION TEST_renderdoc OR (MACOS OR IOS)
)
qt_feature_definition("undogroup" "QT_NO_UNDOGROUP" NEGATE VALUE "1")
qt_feature("wayland" PUBLIC
    SECTION "Platform plugins"
    LABEL "Wayland"
    CONDITION TARGET Wayland::Client
)
qt_feature("waylandscanner" PUBLIC
    SECTION "Wayland Scanner tool"
    LABEL "Wayland Scanner"
    CONDITION TARGET Wayland::Scanner
)

# qt wayland client
qt_feature("wayland-client" PRIVATE
    LABEL "Client"
    CONDITION NOT WIN32 AND QT_FEATURE_wayland AND QT_FEATURE_waylandscanner
)
qt_feature("wayland-server" PRIVATE
    LABEL "Qt Wayland Compositor"
    CONDITION NOT WIN32 AND QT_FEATURE_wayland AND QT_FEATURE_waylandscanner
)
qt_feature("wayland-egl" PRIVATE
    LABEL "EGL"
    CONDITION (QT_FEATURE_wayland_client OR QT_FEATURE_wayland_server)
              AND QT_FEATURE_opengl AND QT_FEATURE_egl
              AND (NOT QNX OR QT_FEATURE_egl_extension_platform_wayland)
)
qt_feature("wayland-brcm" PRIVATE
    LABEL "Raspberry Pi"
    CONDITION (QT_FEATURE_wayland_client OR QT_FEATURE_wayland_server) AND QT_FEATURE_eglfs_brcm
)
qt_feature("wayland-drm-egl-server-buffer" PRIVATE
    LABEL "DRM EGL"
    CONDITION (QT_FEATURE_wayland_client OR QT_FEATURE_wayland_server) AND QT_FEATURE_opengl
              AND QT_FEATURE_egl AND TEST_drm_egl_server
              AND (NOT QNX OR QT_FEATURE_egl_extension_platform_wayland)
)
qt_feature("wayland-libhybris-egl-server-buffer" PRIVATE
    LABEL "libhybris EGL"
    CONDITION (QT_FEATURE_wayland_client OR QT_FEATURE_wayland_server) AND QT_FEATURE_opengl
              AND QT_FEATURE_egl AND TEST_libhybris_egl_server
)
qt_feature("wayland-dmabuf-server-buffer" PRIVATE
    LABEL "Linux dma-buf server buffer"
    CONDITION (QT_FEATURE_wayland_client OR QT_FEATURE_wayland_server) AND QT_FEATURE_opengl
              AND QT_FEATURE_egl AND TEST_dmabuf_server_buffer
)
qt_feature("wayland-shm-emulation-server-buffer" PRIVATE
    LABEL "Shm emulation server buffer"
    CONDITION (QT_FEATURE_wayland_client OR QT_FEATURE_wayland_server) AND QT_FEATURE_opengl AND QT_FEATURE_sharedmemory
)
qt_feature("wayland-vulkan-server-buffer" PRIVATE
    LABEL "Vulkan-based server buffer"
    CONDITION (QT_FEATURE_wayland_client OR QT_FEATURE_wayland_server) AND QT_FEATURE_vulkan
              AND QT_FEATURE_opengl AND QT_FEATURE_egl AND TEST_vulkan_server_buffer
)
qt_feature("wayland-datadevice" PRIVATE
    CONDITION QT_FEATURE_draganddrop OR QT_FEATURE_clipboard
)
qt_feature("wayland-client-primary-selection" PRIVATE
    LABEL "primary-selection clipboard"
    CONDITION QT_FEATURE_clipboard
)
qt_feature("wayland-client-fullscreen-shell-v1" PRIVATE
    LABEL "fullscreen-shell-v1"
    CONDITION QT_FEATURE_wayland_client
)
qt_feature("wayland-client-wl-shell" PRIVATE
    LABEL "wl-shell (deprecated)"
    CONDITION QT_FEATURE_wayland_client
)
qt_feature("wayland-client-xdg-shell" PRIVATE
    LABEL "xdg-shell"
    CONDITION QT_FEATURE_wayland_client
)
qt_feature("egl-extension-platform-wayland" PRIVATE
    LABEL "EGL wayland platform extension"
    CONDITION QT_FEATURE_wayland_client AND QT_FEATURE_opengl AND QT_FEATURE_egl
              AND TEST_egl_1_5_wayland
)
qt_feature("run-opengl-tests" PRIVATE
    LABEL "Run opengl tests"
    PURPOSE "Provides the ability to skip tests which require opengl to run"
    CONDITION QT_FEATURE_opengl
)


qt_configure_add_summary_section(NAME "Qt Gui")
qt_configure_add_summary_entry(ARGS "accessibility")
qt_configure_add_summary_entry(ARGS "emojisegmenter")
qt_configure_add_summary_entry(ARGS "freetype")
qt_configure_add_summary_entry(ARGS "system-freetype")
qt_configure_add_summary_entry(ARGS "harfbuzz")
qt_configure_add_summary_entry(ARGS "system-harfbuzz")
qt_configure_add_summary_entry(ARGS "fontconfig")
qt_configure_add_summary_section(NAME "Image formats")
qt_configure_add_summary_entry(ARGS "gif")
qt_configure_add_summary_entry(ARGS "ico")
qt_configure_add_summary_entry(ARGS "jpeg")
qt_configure_add_summary_entry(ARGS "system-jpeg")
qt_configure_add_summary_entry(ARGS "png")
qt_configure_add_summary_entry(ARGS "system-png")
qt_configure_end_summary_section() # end of "Image formats" section
qt_configure_add_summary_section(NAME "Text formats")
qt_configure_add_summary_entry(ARGS "texthtmlparser")
qt_configure_add_summary_entry(ARGS "cssparser")
qt_configure_add_summary_entry(ARGS "textodfwriter")
qt_configure_add_summary_entry(ARGS "textmarkdownreader")
qt_configure_add_summary_entry(ARGS "system-textmarkdownreader")
qt_configure_add_summary_entry(ARGS "textmarkdownwriter")
qt_configure_end_summary_section() # end of "Text formats" section
qt_configure_add_summary_entry(ARGS "egl")
qt_configure_add_summary_entry(ARGS "openvg")
qt_configure_add_summary_section(NAME "OpenGL")
qt_configure_add_summary_entry(ARGS "opengl-desktop")
qt_configure_add_summary_entry(
    ARGS "opengl-dynamic"
    CONDITION WIN32
)
qt_configure_add_summary_entry(ARGS "opengles2")
qt_configure_add_summary_entry(ARGS "opengles3")
qt_configure_add_summary_entry(ARGS "opengles31")
qt_configure_add_summary_entry(ARGS "opengles32")
qt_configure_end_summary_section() # end of "OpenGL" section
qt_configure_add_summary_entry(ARGS "vulkan")
qt_configure_add_summary_entry(ARGS "metal")
qt_configure_add_summary_entry(ARGS "graphicsframecapture")
qt_configure_add_summary_entry(ARGS "sessionmanager")
qt_configure_add_summary_entry(
    ARGS "qtgui-threadpool"
    CONDITION QT_FEATURE_thread
)
qt_configure_end_summary_section() # end of "Qt Gui" section
qt_configure_add_summary_section(NAME "Features used by QPA backends")
qt_configure_add_summary_entry(ARGS "evdev")
qt_configure_add_summary_entry(ARGS "libinput")
qt_configure_add_summary_entry(ARGS "libinput_hires_wheel_support")
qt_configure_add_summary_entry(ARGS "integrityhid")
qt_configure_add_summary_entry(ARGS "mtdev")
qt_configure_add_summary_entry(ARGS "tslib")
qt_configure_add_summary_entry(ARGS "xkbcommon")
qt_configure_add_summary_entry(ARGS "vxworksevdev")
qt_configure_add_summary_section(NAME "X11 specific")
qt_configure_add_summary_entry(ARGS "xlib")
qt_configure_add_summary_entry(ARGS "xcb-xlib")
qt_configure_add_summary_entry(ARGS "egl_x11")
qt_configure_add_summary_entry(ARGS "xkbcommon-x11")
qt_configure_add_summary_entry(ARGS "xcb-sm")
qt_configure_end_summary_section() # end of "X11 specific" section
qt_configure_end_summary_section() # end of "Features used by QPA backends" section
qt_configure_add_summary_section(NAME "QPA backends")
qt_configure_add_summary_entry(ARGS "directfb")
qt_configure_add_summary_entry(ARGS "eglfs")
qt_configure_add_summary_section(NAME "EGLFS details")
qt_configure_add_summary_entry(ARGS "eglfs_openwfd")
qt_configure_add_summary_entry(ARGS "eglfs_viv")
qt_configure_add_summary_entry(ARGS "eglfs_viv_wl")
qt_configure_add_summary_entry(ARGS "eglfs_rcar")
qt_configure_add_summary_entry(ARGS "eglfs_egldevice")
qt_configure_add_summary_entry(ARGS "eglfs_gbm")
qt_configure_add_summary_entry(ARGS "eglfs_vsp2")
qt_configure_add_summary_entry(ARGS "eglfs_mali")
qt_configure_add_summary_entry(ARGS "eglfs_brcm")
qt_configure_add_summary_entry(ARGS "eglfs_x11")
qt_configure_end_summary_section() # end of "EGLFS details" section
qt_configure_add_summary_entry(ARGS "linuxfb")
qt_configure_add_summary_entry(ARGS "vnc")
qt_configure_add_summary_entry(ARGS "vkkhrdisplay")
qt_configure_add_summary_entry(
    ARGS "integrityfb"
    CONDITION INTEGRITY
)
qt_configure_add_summary_section(NAME "QNX")
qt_configure_add_summary_entry(ARGS "lgmon")
qt_configure_add_summary_entry(ARGS "qqnx_imf")
qt_configure_end_summary_section() # end of "QNX" section
qt_configure_add_summary_section(NAME "XCB")
qt_configure_add_summary_entry(ARGS "system-xcb-xinput")
qt_configure_add_summary_section(NAME "GL integrations")
qt_configure_add_summary_entry(ARGS "xcb-glx-plugin")
qt_configure_add_summary_entry(ARGS "xcb-glx")
qt_configure_add_summary_entry(ARGS "xcb-egl-plugin")
qt_configure_end_summary_section() # end of "GL integrations" section
qt_configure_end_summary_section() # end of "XCB" section
qt_configure_add_summary_section(NAME "Windows")
qt_configure_add_summary_entry(ARGS "direct2d")
qt_configure_add_summary_entry(ARGS "direct2d1_1")
qt_configure_add_summary_entry(ARGS "directwrite")
qt_configure_add_summary_entry(ARGS "directwrite3")
qt_configure_add_summary_entry(ARGS "directwritecolrv1")
qt_configure_end_summary_section() # end of "Windows" section
qt_configure_add_summary_section(NAME "Wayland")
qt_configure_add_summary_entry(ARGS "wayland-client")
qt_configure_add_summary_section(NAME "Hardware Integrations")
qt_configure_add_summary_entry(ARGS "wayland-egl")
qt_configure_add_summary_entry(ARGS "wayland-brcm")
qt_configure_add_summary_entry(ARGS "wayland-drm-egl-server-buffer")
qt_configure_add_summary_entry(ARGS "wayland-libhybris-egl-server-buffer")
qt_configure_add_summary_entry(ARGS "wayland-dmabuf-server-buffer")
qt_configure_add_summary_entry(ARGS "wayland-shm-emulation-server-buffer")
qt_configure_add_summary_entry(ARGS "wayland-vulkan-server-buffer")
qt_configure_end_summary_section() # end of "Qt Wayland Drivers" section
qt_configure_add_summary_section(NAME "Shell Integrations")
qt_configure_add_summary_entry(ARGS "wayland-client-xdg-shell")
qt_configure_add_summary_entry(ARGS "wayland-client-wl-shell")
qt_configure_end_summary_section() # end of "Shell Integrations" section
qt_configure_end_summary_section() # end of "Wayland" section
qt_configure_end_summary_section() # end of "QPA backends" section
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "XCB support on macOS is minimal and untested. Some features will not work properly or at all (e.g. OpenGL, desktop services or accessibility), or may depend on your system and XQuartz setup."
    CONDITION QT_FEATURE_xcb AND APPLE
)
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "Disabling X11 Accessibility Bridge: D-Bus or AT-SPI is missing."
    CONDITION QT_FEATURE_accessibility AND QT_FEATURE_xcb AND NOT QT_FEATURE_accessibility_atspi_bridge
)
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "No QPA platform plugin enabled! This will produce a Qt that cannot run GUI applications.  See \"Platform backends\" in the output of --help."
    CONDITION QT_FEATURE_gui AND LINUX AND NOT ANDROID AND NOT QT_FEATURE_xcb AND NOT QT_FEATURE_eglfs AND NOT QT_FEATURE_directfb AND NOT QT_FEATURE_linuxfb
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "The OpenGL functionality tests failed! You might need to modify the OpenGL package search path by setting the OpenGL_DIR CMake variable to the OpenGL library's installation directory."
    CONDITION QT_FEATURE_gui AND NOT WATCHOS AND NOT VISIONOS AND ( NOT INPUT_opengl STREQUAL 'no' ) AND NOT QT_FEATURE_opengl_desktop AND NOT QT_FEATURE_opengles2 AND NOT QT_FEATURE_opengl_dynamic
)
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "Accessibility disabled. This configuration of Qt is unsupported."
    CONDITION NOT QT_FEATURE_accessibility
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "XCB plugin requires xkbcommon and xkbcommon-x11, but -no-xkbcommon was provided."
    CONDITION ( NOT INPUT_xcb STREQUAL '' ) AND ( NOT INPUT_xcb STREQUAL 'no' ) AND INPUT_xkbcommon STREQUAL 'no'
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "The desktopservices feature is required on macOS and iOS, and cannot be disabled."
    CONDITION APPLE AND NOT QT_FEATURE_desktopservices
)
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "Qt Gui has been built without 'qtwaylandscanner' feature. This feature is required for building Qt Wayland Client."
    CONDITION NOT QT_FEATURE_waylandscanner AND QT_FEATURE_wayland_client
)
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "Qt Gui has been built without 'wayland' feature. This feature is required for building Qt Wayland Client."
    CONDITION NOT QT_FEATURE_wayland AND QT_FEATURE_wayland_client
)

#### Inputs



#### Libraries


#### Tests


#### Features

