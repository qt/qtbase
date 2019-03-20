# Generated from textedit.pro.

#####################################################################
## textedit Binary:
#####################################################################

add_qt_executable(textedit
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/textedit"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/textedit"
    SOURCES
        main.cpp
        textedit.cpp textedit.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(textedit "textedit" PREFIX "/" FILES
    example.html
    images/logo32.png
    images/mac/editcopy.png
    images/mac/editcut.png
    images/mac/editpaste.png
    images/mac/editredo.png
    images/mac/editundo.png
    images/mac/exportpdf.png
    images/mac/filenew.png
    images/mac/fileopen.png
    images/mac/fileprint.png
    images/mac/filesave.png
    images/mac/textbold.png
    images/mac/textcenter.png
    images/mac/textitalic.png
    images/mac/textjustify.png
    images/mac/textleft.png
    images/mac/textright.png
    images/mac/textunder.png
    images/mac/zoomin.png
    images/mac/zoomout.png
    images/win/editcopy.png
    images/win/editcut.png
    images/win/editpaste.png
    images/win/editredo.png
    images/win/editundo.png
    images/win/exportpdf.png
    images/win/filenew.png
    images/win/fileopen.png
    images/win/fileprint.png
    images/win/filesave.png
    images/win/textbold.png
    images/win/textcenter.png
    images/win/textitalic.png
    images/win/textjustify.png
    images/win/textleft.png
    images/win/textright.png
    images/win/textunder.png
    images/win/zoomin.png
    images/win/zoomout.png)


#### Keys ignored in scope 1:.:textedit.pro:<NONE>:
# EXAMPLE_FILES = "textedit.qdoc"
# TEMPLATE = "app"

## Scopes:
#####################################################################

extend_target(textedit CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)

#### Keys ignored in scope 3:.:textedit.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"
