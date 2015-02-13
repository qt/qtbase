SOURCES += gstreamer.cpp

CONFIG += link_pkgconfig

gst-0.10 {
    PKGCONFIG_PRIVATE += \
        gstreamer-0.10 \
        gstreamer-base-0.10 \
        gstreamer-audio-0.10 \
        gstreamer-video-0.10 \
        gstreamer-pbutils-0.10
} else:gst-1.0 {
    PKGCONFIG_PRIVATE += \
        gstreamer-1.0 \
        gstreamer-base-1.0 \
        gstreamer-audio-1.0 \
        gstreamer-video-1.0 \
        gstreamer-pbutils-1.0
}

CONFIG -= qt

