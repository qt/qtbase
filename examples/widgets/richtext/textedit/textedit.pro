TEMPLATE        = app
TARGET          = textedit

CONFIG          += qt warn_on

HEADERS         = textedit.h
SOURCES         = textedit.cpp \
                  main.cpp

RESOURCES += textedit.qrc
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/richtext/textedit
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html *.doc images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/richtext/textedit
INSTALLS += target sources

QT += widgets
!isEmpty(QT.printsupport.name): QT += printsupport
