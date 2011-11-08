# Qt dialogs module

HEADERS += \
	dialogs/qcolordialog.h \
	dialogs/qcolordialog_p.h \
        dialogs/qfscompleter_p.h \
	dialogs/qdialog.h \
	dialogs/qdialog_p.h \
	dialogs/qerrormessage.h \
	dialogs/qfiledialog.h \
	dialogs/qfiledialog_p.h \
	dialogs/qfontdialog.h \
	dialogs/qfontdialog_p.h \
	dialogs/qinputdialog.h \
	dialogs/qmessagebox.h \
	dialogs/qprogressdialog.h \
        dialogs/qsidebar_p.h \
        dialogs/qfilesystemmodel.h \
        dialogs/qfilesystemmodel_p.h \
        dialogs/qfileinfogatherer_p.h \
        dialogs/qwizard.h

# TODO
false:mac {
    OBJECTIVE_SOURCES += dialogs/qfiledialog_mac.mm \
                         dialogs/qfontdialog_mac.mm \
                         dialogs/qnspanelproxy_mac.mm

# Compile qcolordialog_mac.mm with exception support, disregarding the -no-exceptions 
# configure option. (qcolordialog_mac needs to catch exceptions thrown by cocoa)
    EXCEPTION_SOURCES = dialogs/qcolordialog_mac.mm
    exceptions_compiler.commands = $$QMAKE_CXX -c
    exceptions_compiler.commands += $(CXXFLAGS) $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
    exceptions_compiler.commands += -fexceptions
    exceptions_compiler.dependency_type = TYPE_C
    exceptions_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    exceptions_compiler.input = EXCEPTION_SOURCES
    exceptions_compiler.variable_out = OBJECTS
    exceptions_compiler.name = compiling[exceptopns] ${QMAKE_FILE_IN}
    silent:exceptions_compiler.commands = @echo compiling[exceptopns] ${QMAKE_FILE_IN} && $$exceptions_compiler.commands
    QMAKE_EXTRA_COMPILERS += exceptions_compiler
}

win32 {
    HEADERS += dialogs/qwizard_win_p.h
    SOURCES += dialogs/qwizard_win.cpp
}

wince*: FORMS += dialogs/qfiledialog_embedded.ui
else: FORMS += dialogs/qfiledialog.ui

INCLUDEPATH += $$PWD
SOURCES += \
	dialogs/qcolordialog.cpp \
	dialogs/qdialog.cpp \
	dialogs/qerrormessage.cpp \
	dialogs/qfiledialog.cpp \
	dialogs/qfontdialog.cpp \
	dialogs/qinputdialog.cpp \
	dialogs/qmessagebox.cpp \
	dialogs/qprogressdialog.cpp \
        dialogs/qsidebar.cpp \
        dialogs/qfilesystemmodel.cpp \
        dialogs/qfileinfogatherer.cpp \
	dialogs/qwizard.cpp \

RESOURCES += dialogs/qmessagebox.qrc
