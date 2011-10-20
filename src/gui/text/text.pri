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
        text/qlinecontrol_p.h \
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
	text/qtextdocumentlayout_p.h \
        text/qtextcontrol_p.h \
        text/qtextcontrol_p_p.h \
        text/qtextcursor.h \
        text/qtextcursor_p.h \
	text/qtextdocumentfragment.h \
	text/qtextdocumentfragment_p.h \
	text/qtextimagehandler_p.h \
	text/qtexttable.h \
	text/qtextlist.h \
	text/qsyntaxhighlighter.h \
	text/qtextdocumentwriter.h \
	text/qcssparser_p.h \
	text/qtexttable_p.h \
	text/qzipreader_p.h \
	text/qzipwriter_p.h \
	text/qtextodfwriter_p.h \
	text/qstatictext_p.h \
	text/qstatictext.h \
        text/qrawfont.h \
        text/qrawfont_p.h \
    text/qglyphrun.h \
    text/qglyphrun_p.h \
    text/qharfbuzz_copy_p.h

SOURCES += \
	text/qfont.cpp \
	text/qfontengine.cpp \
	text/qfontsubset.cpp \
	text/qfontmetrics.cpp \
	text/qfontdatabase.cpp \
        text/qlinecontrol.cpp \
        text/qtextcontrol.cpp \
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
	text/qcssparser.cpp \
	text/qzip.cpp \
	text/qtextodfwriter.cpp \
	text/qstatictext.cpp \
        text/qrawfont.cpp \
    text/qglyphrun.cpp

contains(QT_CONFIG, directwrite) {
    LIBS_PRIVATE += -ldwrite
    HEADERS += text/qfontenginedirectwrite_p.h
    SOURCES += text/qfontenginedirectwrite.cpp
}

unix:x11 {
	HEADERS += \
		text/qfontengine_x11_p.h \
		text/qfontdatabase_x11.cpp \
		text/qfontengine_ft_p.h
	SOURCES += \
		text/qfont_x11.cpp \
		text/qfontengine_x11.cpp \
                text/qfontengine_ft.cpp \
                text/qrawfont_ft.cpp
}

SOURCES += \
      text/qfont_qpa.cpp \
      text/qfontengine_qpa.cpp \
      text/qplatformfontdatabase_qpa.cpp \
      text/qrawfont_qpa.cpp

HEADERS += \
      text/qplatformfontdatabase_qpa.h

symbian {
	SOURCES += \
		text/qfont_s60.cpp
	contains(QT_CONFIG, freetype) {
		SOURCES += \
                        text/qfontengine_ft.cpp \
                        text/qrawfont_ft.cpp
		HEADERS += \
			text/qfontengine_ft_p.h
		DEFINES += \
			QT_NO_FONTCONFIG
	} else {
		SOURCES += \
			text/qfontengine_s60.cpp
		HEADERS += \
			text/qfontengine_s60_p.h
	}
	LIBS += -lfntstr -lecom
}

DEFINES += QT_NO_OPENTYPE
INCLUDEPATH += ../3rdparty/harfbuzz/src
