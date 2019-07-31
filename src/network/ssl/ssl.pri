# OpenSSL support; compile in QSslSocket.

HEADERS += ssl/qasn1element_p.h \
           ssl/qssl.h \
           ssl/qssl_p.h \
           ssl/qsslcertificate.h \
           ssl/qsslcertificate_p.h \
           ssl/qsslcertificateextension.h \
           ssl/qsslcertificateextension_p.h

SOURCES += ssl/qasn1element.cpp \
           ssl/qssl.cpp \
           ssl/qsslcertificate.cpp \
           ssl/qsslcertificateextension.cpp

!qtConfig(openssl): SOURCES += ssl/qsslcertificate_qt.cpp

qtConfig(ssl) {
    HEADERS += ssl/qsslconfiguration.h \
               ssl/qsslconfiguration_p.h \
               ssl/qsslcipher.h \
               ssl/qsslcipher_p.h \
               ssl/qssldiffiehellmanparameters.h \
               ssl/qssldiffiehellmanparameters_p.h \
               ssl/qsslellipticcurve.h \
               ssl/qsslerror.h \
               ssl/qsslkey.h \
               ssl/qsslkey_p.h \
               ssl/qsslsocket.h \
               ssl/qsslsocket_p.h \
               ssl/qsslpresharedkeyauthenticator.h \
               ssl/qsslpresharedkeyauthenticator_p.h
    SOURCES += ssl/qsslconfiguration.cpp \
               ssl/qsslcipher.cpp \
               ssl/qssldiffiehellmanparameters.cpp \
               ssl/qsslellipticcurve.cpp \
               ssl/qsslkey_p.cpp \
               ssl/qsslerror.cpp \
               ssl/qsslsocket.cpp \
               ssl/qsslpresharedkeyauthenticator.cpp

    winrt {
        HEADERS += ssl/qsslsocket_winrt_p.h
        SOURCES += ssl/qsslcertificate_winrt.cpp \
                   ssl/qssldiffiehellmanparameters_dummy.cpp \
                   ssl/qsslkey_qt.cpp \
                   ssl/qsslkey_winrt.cpp \
                   ssl/qsslsocket_winrt.cpp \
                   ssl/qsslellipticcurve_dummy.cpp
    }

    qtConfig(securetransport) {
        HEADERS += ssl/qsslsocket_mac_p.h
        SOURCES += ssl/qssldiffiehellmanparameters_dummy.cpp \
                   ssl/qsslkey_qt.cpp \
                   ssl/qsslkey_mac.cpp \
                   ssl/qsslsocket_mac_shared.cpp \
                   ssl/qsslsocket_mac.cpp \
                   ssl/qsslellipticcurve_dummy.cpp
    }

    qtConfig(dtls) {
        HEADERS += ssl/qdtls.h \
                   ssl/qdtls_p.h

        SOURCES += ssl/qdtls.cpp
    }

    qtConfig(openssl) {
        HEADERS += ssl/qsslcontext_openssl_p.h \
                   ssl/qsslsocket_openssl_p.h \
                   ssl/qsslsocket_openssl_symbols_p.h
        SOURCES += ssl/qsslsocket_openssl_symbols.cpp \
                   ssl/qssldiffiehellmanparameters_openssl.cpp \
                   ssl/qsslcertificate_openssl.cpp \
                   ssl/qsslellipticcurve_openssl.cpp \
                   ssl/qsslkey_openssl.cpp \
                   ssl/qsslsocket_openssl.cpp \
                   ssl/qsslcontext_openssl.cpp \

        qtConfig(dtls) {
            HEADERS += ssl/qdtls_openssl_p.h
            SOURCES += ssl/qdtls_openssl.cpp
        }

        qtConfig(opensslv11) {
            HEADERS += ssl/qsslsocket_openssl11_symbols_p.h
            SOURCES += ssl/qsslsocket_openssl11.cpp \
                       ssl/qsslcontext_openssl11.cpp

            QMAKE_CXXFLAGS += -DOPENSSL_API_COMPAT=0x10100000L
        } else {
            HEADERS += ssl/qsslsocket_opensslpre11_symbols_p.h
            SOURCES += ssl/qsslsocket_opensslpre11.cpp \
                       ssl/qsslcontext_opensslpre11.cpp
        }

        darwin:SOURCES += ssl/qsslsocket_mac_shared.cpp

        android:!android-embedded: SOURCES += ssl/qsslsocket_openssl_android.cpp

        # Add optional SSL libs
        # Static linking of OpenSSL with msvc:
        #   - Binaries http://slproweb.com/products/Win32OpenSSL.html
        #   - also needs -lWs2_32 -lGdi32 -lAdvapi32 -lCrypt32 -lUser32
        #   - libs in <OPENSSL_DIR>\lib\VC\static
        #   - configure: -openssl-linked -openssl-linked OPENSSL_INCDIR="%OPENSSL_DIR%\include" OPENSSL_LIBDIR="%OPENSSL_DIR%\lib\VC\static" OPENSSL_LIBS="-lWs2_32 -lGdi32 -lAdvapi32 -lCrypt32 -lUser32" OPENSSL_LIBS_DEBUG="-llibssl64MDd -llibcrypto64MDd" OPENSSL_LIBS_RELEASE="-llibssl64MD -llibcrypto64MD"

        qtConfig(openssl-linked): \
            QMAKE_USE_FOR_PRIVATE += openssl
        else: \
            QMAKE_USE_FOR_PRIVATE += openssl/nolink
        win32 {
            LIBS_PRIVATE += -lcrypt32
            HEADERS += ssl/qwindowscarootfetcher_p.h
            SOURCES += ssl/qwindowscarootfetcher.cpp
        }
    }
}

HEADERS += ssl/qpassworddigestor.h
SOURCES += ssl/qpassworddigestor.cpp
