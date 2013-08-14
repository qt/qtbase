class network_test_server::sshd {

    package { "openssh-server":
        ensure  =>  present,
    }

    service {
        "ssh":
            enable  =>  true,
            ensure  =>  running,
            require =>  Package["openssh-server"],
        ;
    }

}

