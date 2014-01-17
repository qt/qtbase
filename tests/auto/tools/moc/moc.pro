CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_moc

#exists(/usr/include/boost/spirit.hpp) {
#    message("including boost headers in test")
#    DEFINES += PARSE_BOOST
#    # need to add explicitly so that it ends up in moc's search path
#    INCLUDEPATH += /usr/include
#}

INCLUDEPATH += testproject/include testproject

cross_compile: DEFINES += MOC_CROSS_COMPILED

HEADERS += using-namespaces.h no-keywords.h task87883.h c-comments.h backslash-newlines.h oldstyle-casts.h \
           slots-with-void-template.h qinvokable.h namespaced-flags.h trigraphs.h \
           escapes-in-string-literals.h cstyle-enums.h qprivateslots.h gadgetwithnoenums.h \
           dir-in-include-path.h single_function_keyword.h task192552.h task189996.h \
           task234909.h task240368.h pure-virtual-signals.h cxx11-enums.h \
           cxx11-final-classes.h \
           cxx11-explicit-override-control.h \
           forward-declared-param.h \
           parse-defines.h \
           function-with-attributes.h \
           plugin_metadata.h \
           single-quote-digit-separator-n3781.h \
           related-metaobjects-in-namespaces.h \
           qtbug-35657-gadget.h \
           related-metaobjects-in-gadget.h \
           related-metaobjects-name-conflict.h


if(*-g++*|*-icc*|*-clang*|*-llvm):!irix-*:!win32-*: HEADERS += os9-newlines.h win-newlines.h
if(*-g++*|*-clang*): HEADERS += dollars.h
SOURCES += tst_moc.cpp

QT = core sql network testlib
qtHaveModule(dbus) {
    DEFINES += WITH_DBUS
    QT += dbus
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

# tst_Moc::specifyMetaTagsFromCmdline()
# Ensure that plugin_metadata.h are moc-ed with some extra -M arguments:
QMAKE_MOC_OPTIONS += -Muri=com.company.app -Muri=com.company.app.private

# Define macro on the command lines used in  parse-defines.h
QMAKE_MOC_OPTIONS += "-DDEFINE_CMDLINE_EMPTY="  "\"-DDEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int, int> &i)\""

