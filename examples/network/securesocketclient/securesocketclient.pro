HEADERS   += certificateinfo.h \
             sslclient.h
SOURCES   += certificateinfo.cpp \
             main.cpp \
             sslclient.cpp
RESOURCES += securesocketclient.qrc
FORMS     += certificateinfo.ui \
             sslclient.ui \
             sslerrors.ui
QT        += network widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/securesocketclient
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.png *.jpg images
sources.path = $$[QT_INSTALL_EXAMPLES]/network/securesocketclient
INSTALLS += target sources

