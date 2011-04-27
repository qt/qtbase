load(qttest_p4)
SOURCES  += tst_qhttp.cpp


QT = core network

wince*: {
    webFiles.files = webserver/*
    webFiles.path = webserver
    cgi.files = webserver/cgi-bin/*
    cgi.path = webserver/cgi-bin
    addFiles.files = rfc3252.txt trolltech
    addFiles.path = .
    DEPLOYMENT += addFiles webFiles cgi
    DEFINES += SRCDIR=\\\"\\\"
} else:symbian {
    webFiles.files = webserver/*
    webFiles.path = webserver
    cgi.files = webserver/cgi-bin/*
    cgi.path = webserver/cgi-bin
    addFiles.files = rfc3252.txt trolltech
    addFiles.path = .
    DEPLOYMENT += addFiles webFiles cgi
    TARGET.CAPABILITY = NetworkServices
} else:vxworks*: {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
