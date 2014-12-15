class network_test_server::openssl_server_psk {

    service {
        "openssl_server_psk":
            enable  =>  true,
            ensure  =>  running,
            require =>  File["/home/qt-test-server/openssl_server_psk.sh"]
        ;
    }

    file {
        "/etc/init.d/openssl_server_psk":
            source  =>  "puppet:///modules/network_test_server/ssl/openssl_server_psk",
            mode    =>  0755,
            ensure  =>  present,
            notify  =>  Service["openssl_server_psk"],
        ;
        "/home/qt-test-server/openssl_server_psk.sh":
            source  =>  "puppet:///modules/network_test_server/ssl/openssl_server_psk.sh",
            mode    =>  0755,
            ensure  =>  present,
            notify  =>  Service["openssl_server_psk"],
            require =>  User["qt-test-server"],
        ;
    }

}

