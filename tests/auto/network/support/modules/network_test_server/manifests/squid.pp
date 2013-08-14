class network_test_server::squid {

    package { "squid3":
        ensure  =>  present,
        require =>  File[
            "/home/qt-test-server/passwords",
            "/etc/init.d/squid3-authenticating-ntlm"
        ],
    }

    # make sure we use only squid3
    # Although it may be allowed for both to be installed, it's simply confusing
    package { "squid":
        ensure  =>  absent,
    }

    service {
        "squid3":
            ensure   =>  running,
            require  =>  [ Package["squid3"], File["/etc/default/squid3", "/etc/squid3/squid.conf"] ],
            pattern  =>  '-f /etc/squid3/squid.conf',
            provider =>  'upstart',
        ;
        "squid3-authenticating-ntlm":
            ensure   =>  running,
            require  =>  [ Package["squid3"], File["/etc/default/squid3", "/etc/squid3/squid-authenticating-ntlm.conf", "/etc/init/squid3-authenticating-ntlm.conf"] ],
            pattern  =>  '-f /etc/squid3/squid-authenticating-ntlm.conf',
            provider =>  'upstart',
        ;
    }

    file {
        "/etc/squid3/squid.conf":
            source  =>  "puppet:///modules/network_test_server/config/squid/squid.conf",
            require =>  Package["squid3"],
            notify  =>  Service["squid3"],
        ;
        "/etc/squid3/squid-authenticating-ntlm.conf":
            source  =>  "puppet:///modules/network_test_server/config/squid/squid-authenticating-ntlm.conf",
            require =>  Package["squid3"],
            notify  =>  Service["squid3-authenticating-ntlm"],
        ;
        "/etc/init.d/squid3-authenticating-ntlm":
            ensure  =>  "/etc/init.d/squid3",
        ;
        "/etc/default/squid3":
            source  =>  "puppet:///modules/network_test_server/config/squid/etc_default_squid",
            require =>  Package["squid3"],
            notify  =>  Service["squid3", "squid3-authenticating-ntlm"],
        ;
    }

    file {
        "/etc/init/squid3-authenticating-ntlm.conf":
            source  =>  "puppet:///modules/network_test_server/init/squid3-authenticating-ntlm.conf",
            require =>  Package["squid3"],
        ;
    }
}

