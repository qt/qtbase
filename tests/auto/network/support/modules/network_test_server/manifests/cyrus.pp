class network_test_server::cyrus {

    package {
        "cyrus-imapd-2.2":  ensure  =>  present;
    }

    service {
        "cyrus-imapd":
            enable    => true,
            ensure    => running,
            hasstatus => true,
            require   => [ Package["cyrus-imapd-2.2"], File["/etc/cyrus.conf"] ],
        ;
    }

    file {
        "/etc/cyrus.conf":
            source  =>  "puppet:///modules/network_test_server/config/cyrus/cyrus.conf",
            require =>  Package["cyrus-imapd-2.2"],
            notify  =>  Service["cyrus-imapd"],
        ;
        "/etc/imapd.conf":
            source  =>  "puppet:///modules/network_test_server/config/cyrus/imapd.conf",
            require =>  Package["cyrus-imapd-2.2"],
            notify  =>  Service["cyrus-imapd"],
        ;
    }
}

