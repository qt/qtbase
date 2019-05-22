# Qt util module

HEADERS += \
        util/qdesktopservices.h \
        util/qhexstring_p.h \
        util/qvalidator.h \
        util/qgridlayoutengine_p.h \
        util/qabstractlayoutstyleinfo_p.h \
        util/qlayoutpolicy_p.h \
        util/qshaderformat_p.h \
        util/qshadergraph_p.h \
        util/qshadergraphloader_p.h \
        util/qshaderlanguage_p.h \
        util/qshadernode_p.h \
        util/qshadernodeport_p.h \
        util/qshadernodesloader_p.h \
        util/qtexturefiledata_p.h \
        util/qtexturefilereader_p.h \
        util/qtexturefilehandler_p.h \
        util/qpkmhandler_p.h \
        util/qktxhandler_p.h \
        util/qastchandler_p.h

SOURCES += \
        util/qdesktopservices.cpp \
        util/qvalidator.cpp \
        util/qgridlayoutengine.cpp \
        util/qabstractlayoutstyleinfo.cpp \
        util/qlayoutpolicy.cpp \
        util/qshaderformat.cpp \
        util/qshadergraph.cpp \
        util/qshadergraphloader.cpp \
        util/qshaderlanguage.cpp \
        util/qshadernode.cpp \
        util/qshadernodeport.cpp \
        util/qshadernodesloader.cpp \
        util/qtexturefiledata.cpp \
        util/qtexturefilereader.cpp \
        util/qpkmhandler.cpp \
        util/qktxhandler.cpp \
        util/qastchandler.cpp

qtConfig(regularexpression) {
    HEADERS += \
        util/qshadergenerator_p.h
    SOURCES += \
        util/qshadergenerator.cpp
}
