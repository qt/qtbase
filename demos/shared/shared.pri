INCLUDEPATH += $$SHARED_FOLDER

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}
contains(CONFIG, debug_and_release_target) {    
    CONFIG(debug, debug|release) { 
        QMAKE_LIBDIR += $$SHARED_FOLDER/debug
    } else {
        QMAKE_LIBDIR += $$SHARED_FOLDER/release
    }
} else {
    QMAKE_LIBDIR += $$SHARED_FOLDER
}

hpux-acc*:LIBS += $$SHARED_FOLDER/libdemo_shared.a
hpuxi-acc*:LIBS += $$SHARED_FOLDER/libdemo_shared.a
symbian:LIBS += -ldemo_shared.lib
!hpuxi-acc*:!hpux-acc*:!symbian:LIBS += -ldemo_shared

