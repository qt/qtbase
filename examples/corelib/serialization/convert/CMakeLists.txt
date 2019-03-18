# Generated from convert.pro.

#####################################################################
## convert Binary:
#####################################################################

add_qt_executable(convert
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/serialization/convert"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/serialization/convert"
    SOURCES
        cborconverter.cpp cborconverter.h
        converter.h
        datastreamconverter.cpp datastreamconverter.h
        jsonconverter.cpp jsonconverter.h
        main.cpp
        nullconverter.cpp nullconverter.h
        textconverter.cpp textconverter.h
        xmlconverter.cpp xmlconverter.h
    LIBRARIES
        # Remove: gui
)

#### Keys ignored in scope 1:.:convert.pro:<NONE>:
# CONFIG = "-app_bundle"
# TEMPLATE = "app"
