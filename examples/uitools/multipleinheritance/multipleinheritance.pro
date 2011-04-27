#! [0]
SOURCES = calculatorform.cpp main.cpp
HEADERS	= calculatorform.h
FORMS = calculatorform.ui
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/uitools/multipleinheritance
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/uitools/multipleinheritance
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000D7C1
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
