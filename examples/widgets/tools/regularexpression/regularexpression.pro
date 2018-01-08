QT += widgets
requires(qtConfig(treewidget))

HEADERS       = regularexpressiondialog.h
SOURCES       = regularexpressiondialog.cpp \
                main.cpp

RESOURCES += regularexpression.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/regularexpression
INSTALLS += target
