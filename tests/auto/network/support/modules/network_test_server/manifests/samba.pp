class network_test_server::samba {

    package {
        "samba":    ensure  =>  present;
        "winbind":  ensure  =>  present;
    }

    service {
        "smbd":
            enable  =>  true,
            ensure  =>  running,
            require =>  Package["samba"],
        ;
    }

    file {
        "/etc/samba/smb.conf":
            source  =>  "puppet:///modules/network_test_server/config/samba/smb.conf",
            require =>  Package["samba"],
            notify  =>  Service["smbd"],
        ;
        "/etc/samba/smbpasswd":
            source  =>  "puppet:///modules/network_test_server/config/samba/smbpasswd",
            require =>  Package["samba"],
            notify  =>  Service["smbd"],
        ;
        "/etc/samba/smbusers":
            source  =>  "puppet:///modules/network_test_server/config/samba/smbusers",
            require =>  Package["samba"],
            notify  =>  Service["smbd"],
        ;
        "/home/writeables/smb":
            ensure  =>  directory,
            mode    =>  1777,
            require =>  File["/home/writeables"],
        ;
        "/home/qt-test-server/smb":
            source  =>  "puppet:///modules/network_test_server/smb",
            recurse =>  remote,
            # Make sure not to delete the programmatically created testdata
            purge   =>  false,
            require =>  User["qt-test-server"],
        ;
        "/home/qt-test-server/smb/testsharelargefile":
            ensure  =>  directory,
            require =>  File["/home/qt-test-server/smb"],
        ;
        "/home/qt-test-server/smb/testshare/tmp":
            ensure  =>  directory,
            require =>  File["/home/qt-test-server/smb"],
        ;
    }

    # Create large, sparse testdata file
    exec { "create testsharelargefile/test.bin":
        command =>  "/bin/sh -c \"set -e

echo -n LargeFile content at offset 8589914592 | \\
    dd of=/home/qt-test-server/smb/testsharelargefile/file.bin bs=1 seek=8589914592

    dd if=/dev/zero of=/home/qt-test-server/smb/testsharelargefile/file.bin count=1 bs=1 seek=8589934591

\"",
        creates =>  "/home/qt-test-server/smb/testsharelargefile/file.bin",
        require =>  File["/home/qt-test-server/smb/testsharelargefile"],
    }
}

