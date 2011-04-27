contains(QT_CONFIG, egl): {
	CONFIG += egl

	HEADERS += \
	    egl/qegl_p.h \
	    egl/qeglcontext_p.h \
	    egl/qeglproperties_p.h

        SOURCES += \
            egl/qegl.cpp \
            egl/qeglproperties.cpp
        unix {
            !isEmpty(QMAKE_INCDIR_EGL){
                INCLUDEPATH += $$QMAKE_INCDIR_EGL
            }
            !isEmpty(QMAKE_LIBDIR_EGL){
                for(p, QMAKE_LIBDIR_EGL) {
                    exists($$p):LIBS += -L$$p
                }
            }

            !isEmpty(QMAKE_LIBS_EGL): LIBS += $$QMAKE_LIBS_EGL
        }

	wince*: SOURCES += egl/qegl_wince.cpp

	unix {
	    embedded {
	        SOURCES += egl/qegl_qws.cpp
	    } else {
	        qpa {
	            SOURCES += egl/qegl_qpa.cpp
	        } else {
	            symbian {
	                SOURCES += egl/qegl_symbian.cpp
	            } else {
	                SOURCES += egl/qegl_x11.cpp
	            }
	        }
	    }
	}
} else:symbian {
	DEFINES += QT_NO_EGL
	SOURCES += egl/qegl_stub.cpp
	SOURCES += egl/qeglproperties_stub.cpp
}
