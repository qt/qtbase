TARGET =   mv_readonly

TEMPLATE = app

QT += widgets
requires(qtConfig(tableview))

SOURCES += main.cpp \
           mymodel.cpp

HEADERS += mymodel.h


# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/1_readonly
INSTALLS += target
