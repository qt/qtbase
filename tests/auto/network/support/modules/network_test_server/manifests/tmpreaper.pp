class network_test_server::tmpreaper {

    # some tests will upload some files,
    # tmpreaper cleans them up.

    package { "tmpreaper":
        ensure  =>  present,
    }

    file {
        "/etc/cron.daily/writeableswatch":
            source  =>  "puppet:///modules/network_test_server/cron.daily/writeableswatch",
            require =>  Package["tmpreaper"],
            mode    =>  0755,
        ;
    }

}

