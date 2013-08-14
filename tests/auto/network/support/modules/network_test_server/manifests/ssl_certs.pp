class network_test_server::ssl_certs {
    File {
        require => User["qt-test-server"],
    }

    file {
        "/home/qt-test-server/ssl-certs":
            ensure  =>  directory;
        "/home/qt-test-server/ssl-certs/private":
            ensure  =>  directory,
            require =>  File["/home/qt-test-server/ssl-certs"];
    }

    ssl_file {
        "qt-test-server-cert.pem":          ensure  =>  present;
        "private/qt-test-server-key.pem":   ensure  =>  present;
    }
}

define ssl_file($ensure) {
    if $ensure == "present" {
        file { "/home/qt-test-server/ssl-certs/$name":
            source  =>  "puppet:///modules/network_test_server/ssl/$name",
            require =>  File["/home/qt-test-server/ssl-certs/private"],
        }
    }
    else {
        file { "/home/qt-test-server/ssl-certs/$name":
            ensure  =>  absent,
        }
    }
}


