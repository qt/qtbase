DEFINES += QT_COMPAT_WARNINGS QT_NO_CAST_TO_ASCII 

INCLUDEPATH += $$COMMON_FOLDER

include($$COMMON_FOLDER/common.pri)

#build_all:!build_pass {
#    CONFIG -= build_all
#    CONFIG += release
#}
#contains(CONFIG, debug_and_release_target) {    
#    CONFIG(debug, debug|release) { 
#	LIBS+=-L$$COMMON_FOLDER/debug
#    } else {
#	LIBS+=-L$$COMMON_FOLDER/release
#    }
#} else {
#    LIBS += -L$$COMMON_FOLDER
#}
#
#LIBS += -ltestcommon
