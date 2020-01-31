# Generated from ssl.pro.

add_subdirectory(qpassworddigestor)
add_subdirectory(qsslcertificate)
add_subdirectory(qsslcipher)
add_subdirectory(qsslellipticcurve)
add_subdirectory(qsslerror)
add_subdirectory(qsslkey)

if(QT_FEATURE_ssl)

    if(QT_FEATURE_private_tests)
        add_subdirectory(qsslsocket)
        if(NOT WINRT)
            add_subdirectory(qsslsocket_onDemandCertificates_member)
            add_subdirectory(qsslsocket_onDemandCertificates_static)
        endif()

        if(QT_FEATURE_dtls)
            add_subdirectory(qdtlscookie)
            add_subdirectory(qdtls)
        endif()

        if(QT_FEATURE_ocsp)
            add_subdirectory(qocsp)
        endif()
    endif()
endif()

if(QT_FEATURE_ssl)

    if(QT_FEATURE_private_tests)
        add_subdirectory(qasn1element)
        add_subdirectory(qssldiffiehellmanparameters)
    endif()
endif()
