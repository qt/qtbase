# Qt kernel module

HEADERS += \
    text/qfont.h \
    text/qfontdatabase.h \
    text/qfontengine_p.h \
    text/qfontengineglyphcache_p.h \
    text/qfontinfo.h \
    text/qfontmetrics.h \
    text/qfont_p.h \
    text/qfontsubset_p.h \
    text/qtextengine_p.h \
    text/qtextlayout.h \
    text/qtextformat.h \
    text/qtextformat_p.h \
    text/qtextobject.h \
    text/qtextobject_p.h \
    text/qtextoption.h \
    text/qfragmentmap_p.h \
    text/qtextdocument.h \
    text/qtextdocument_p.h \
    text/qtexthtmlparser_p.h \
    text/qabstracttextdocumentlayout.h \
    text/qabstracttextdocumentlayout_p.h \
    text/qtextdocumentlayout_p.h \
    text/qtextcursor.h \
    text/qtextcursor_p.h \
    text/qtextdocumentfragment.h \
    text/qtextdocumentfragment_p.h \
    text/qtextimagehandler_p.h \
    text/qtexttable.h \
    text/qtextlist.h \
    text/qsyntaxhighlighter.h \
    text/qtextdocumentwriter.h \
    text/qtexttable_p.h \
    text/qstatictext_p.h \
    text/qstatictext.h \
    text/qrawfont.h \
    text/qrawfont_p.h \
    text/qglyphrun.h \
    text/qglyphrun_p.h \
    text/qdistancefield_p.h \
    text/qinputcontrol_p.h

SOURCES += \
    text/qfont.cpp \
    text/qfontengine.cpp \
    text/qfontengineglyphcache.cpp \
    text/qfontsubset.cpp \
    text/qfontmetrics.cpp \
    text/qfontdatabase.cpp \
    text/qtextengine.cpp \
    text/qtextlayout.cpp \
    text/qtextformat.cpp \
    text/qtextobject.cpp \
    text/qtextoption.cpp \
    text/qfragmentmap.cpp \
    text/qtextdocument.cpp \
    text/qtextdocument_p.cpp \
    text/qtexthtmlparser.cpp \
    text/qabstracttextdocumentlayout.cpp \
    text/qtextdocumentlayout.cpp \
    text/qtextcursor.cpp \
    text/qtextdocumentfragment.cpp \
    text/qtextimagehandler.cpp \
    text/qtexttable.cpp \
    text/qtextlist.cpp \
    text/qtextdocumentwriter.cpp \
    text/qsyntaxhighlighter.cpp \
    text/qstatictext.cpp \
    text/qrawfont.cpp \
    text/qglyphrun.cpp \
    text/qdistancefield.cpp \
    text/qinputcontrol.cpp

SOURCES += \
    text/qfontengine_qpf2.cpp \
    text/qplatformfontdatabase.cpp

HEADERS += \
    text/qplatformfontdatabase.h

qtConfig(harfbuzz) {
    QMAKE_USE_PRIVATE += harfbuzz

    SOURCES += text/qharfbuzzng.cpp
    HEADERS += text/qharfbuzzng_p.h
}

qtConfig(textodfwriter) {
    HEADERS += \
        text/qtextodfwriter_p.h \
        text/qzipreader_p.h \
        text/qzipwriter_p.h
    SOURCES += \
        text/qtextodfwriter.cpp \
        text/qzip.cpp
}

qtConfig(textmarkdownreader) {
    qtConfig(system-textmarkdownreader) {
        QMAKE_USE_PRIVATE += libmd4c
    } else {
        include($$PWD/../../3rdparty/md4c.pri)
    }
    HEADERS += \
        text/qtextmarkdownimporter_p.h
    SOURCES += \
        text/qtextmarkdownimporter.cpp
}

qtConfig(textmarkdownwriter) {
    HEADERS += \
        text/qtextmarkdownwriter_p.h
    SOURCES += \
        text/qtextmarkdownwriter.cpp
}

qtConfig(cssparser) {
    HEADERS += \
        text/qcssparser_p.h
    SOURCES += \
        text/qcssparser.cpp
}
