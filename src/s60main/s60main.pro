# Additional Qt project file for qtmain lib on Symbian
TEMPLATE = lib
TARGET	 = qtmain
DESTDIR	 = $$QMAKE_LIBDIR_QT
QT       =

CONFIG	+= staticlib warn_on
CONFIG	-= qt shared

symbian {
    # Note: UID only needed for ensuring that no filename generation conflicts occur
    TARGET.UID3 = 0x2001E61F
    CONFIG      +=  png zlib
    CONFIG	-=  jpeg
    INCLUDEPATH	+=  tmp $$QMAKE_INCDIR_QT/QtCore $$MW_LAYER_SYSTEMINCLUDE
    SOURCES	 =  qts60main.cpp \
                    qts60main_mcrt0.cpp \
                    newallocator_hook.cpp

    # s60main needs to be built in ARM mode for GCCE to work.
    CONFIG += do_not_build_as_thumb

    # staticlib should not have any lib depencies in s60
    # This seems not to work, some hard coded libs are still added as dependency
    LIBS =

    # Workaround for abld toolchain problem to make ARMV6 qtmain.lib link with GCCE apps
    symbian-abld: QMAKE_CXXFLAGS.ARMCC += --dllimport_runtime

    # Having MMP_RULES_DONT_EXPORT_ALL_CLASS_IMPEDIMENTA will cause s60main.lib be unlinkable
    # against GCCE apps, so remove it
    MMP_RULES -= $$MMP_RULES_DONT_EXPORT_ALL_CLASS_IMPEDIMENTA
    symbian-armcc:QMAKE_CXXFLAGS *= --export_all_vtbl
} else {
    error("$$_FILE_ is intended only for Symbian!")
}

include(../qbase.pri)
