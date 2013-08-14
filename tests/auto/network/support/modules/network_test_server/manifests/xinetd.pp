class network_test_server::xinetd {

    package { "xinetd":
        ensure  =>  present,
    }

    service {
        "xinetd":
            enable  =>  true,
            ensure  =>  running,
            require =>  File["/etc/xinetd.d/echo", "/etc/xinetd.d/daytime"],
        ;
    }

    file {
        "/etc/xinetd.d/echo":
            source  =>  "puppet:///modules/network_test_server/config/xinetd/echo",
            require =>  Package["xinetd"],
            notify  =>  Service["xinetd"],
        ;
        "/etc/xinetd.d/daytime":
            source  =>  "puppet:///modules/network_test_server/config/xinetd/daytime",
            require =>  Package["xinetd"],
            notify  =>  Service["xinetd"],
        ;
    }

}

