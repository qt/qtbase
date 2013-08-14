class network_test_server::danted {

    package { "dante-server":
        ensure  =>  present,
        require =>  File["/etc/init.d/danted", "/etc/init.d/danted-authenticating"],
    }

    service {
        "danted":
            enable  =>  true,
            ensure  =>  running,
            require =>  Package["dante-server"],
        ;
        "danted-authenticating":
            enable  =>  true,
            ensure  =>  running,
            require =>  Package["dante-server"],
        ;
    }

    file {
        "/etc/danted.conf":
            source  =>  "puppet:///modules/network_test_server/config/danted/danted.conf",
            require =>  Package["dante-server"],
            notify  =>  Service["danted"],
        ;
        "/etc/danted-authenticating.conf":
            source  =>  "puppet:///modules/network_test_server/config/danted/danted-authenticating.conf",
            require =>  Package["dante-server"],
            notify  =>  Service["danted-authenticating"],
        ;
        "/etc/init.d/danted":
            source  =>  "puppet:///modules/network_test_server/init/danted",
            mode    =>  755,
        ;
        "/etc/init.d/danted-authenticating":
            source  =>  "puppet:///modules/network_test_server/init/danted-authenticating",
            mode    =>  755,
        ;
    }

    file {
        "/lib/x86_64-linux-gnu/libc.so":
            ensure =>   'link',
            target =>   "/lib/x86_64-linux-gnu/libc.so.6",
        ;
    }
}

