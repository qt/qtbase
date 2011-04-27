SOURCES += s60framework/qs60mainapplication.cpp \
           s60framework/qs60mainappui.cpp \
           s60framework/qs60maindocument.cpp

HEADERS += s60framework/qs60mainapplication_p.h \
           s60framework/qs60mainapplication.h \
           s60framework/qs60mainappui.h \
           s60framework/qs60maindocument.h

!isEmpty(QT_LIBINFIX): DEFINES += QT_LIBINFIX_UNQUOTED=$$QT_LIBINFIX