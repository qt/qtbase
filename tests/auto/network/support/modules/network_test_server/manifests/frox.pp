class network_test_server::frox {

    source_install { "frox" :
        creates => "/usr/local/sbin/frox",
        url => "http://frox.sourceforge.net/download/frox-0.7.18.tar.gz",
        configureparams => "--enable-configfile=/etc/frox.conf",
    }

    service {
        "frox":
            enable  =>  true,
            ensure  =>  running,
            require =>  [ Source_Install["frox"], File["/etc/default/frox", "/etc/frox.conf" ] ],
        ;
    }

    file {
        "/etc/default/frox":
            source  =>  "puppet:///modules/network_test_server/config/frox/etc_default_frox",
            require =>  Source_Install["frox"],
            notify  =>  Service["frox"],
        ;
        "/etc/frox.conf":
            source  =>  "puppet:///modules/network_test_server/config/frox/frox.conf",
            require =>  Source_Install["frox"],
            notify  =>  Service["frox"],
        ;
        "/etc/init.d/frox":
            source  =>  "puppet:///modules/network_test_server/init/frox",
            require =>  Source_Install["frox"],
            notify  =>  Service["frox"],
            mode    =>  755,
        ;
    }
}

