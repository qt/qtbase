QT      +=  network widgets
SOURCES =   main.cpp searchbox.cpp googlesuggest.cpp
HEADERS  =  searchbox.h googlesuggest.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/googlesuggest
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/network/googlesuggest
INSTALLS += target sources


simulator: warning(This example might not fully work on Simulator platform)
