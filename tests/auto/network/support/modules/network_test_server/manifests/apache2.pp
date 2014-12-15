class network_test_server::apache2 {
    package {
        "apache2": ensure  =>  present;
    }

    exec {
        "download SPDY module":
            command => "/usr/bin/wget https://dl-ssl.google.com/dl/linux/direct/mod-spdy-beta_current_i386.deb --directory-prefix=/tmp",
            creates => "/tmp/mod-spdy-beta_current_i386.deb",
            notify  =>  Service["apache2"]
        ;
    }

    package {
        "mod_spdy":
            ensure   => installed,
            provider => dpkg,
            source   => "/tmp/mod-spdy-beta_current_i386.deb",
            notify   => Service["apache2"],
            require  => [ Package["apache2"], Exec["download SPDY module"] ]
    }

    service { "apache2":
        enable  =>  true,
        ensure  =>  running,
        require =>  Package["apache2", "mod_spdy"],
    }

    apache2_module {
        "dav_fs":           ensure  =>  present;
        "headers":          ensure  =>  present;

        "ssl":
            ensure  =>  present,
            require =>  File['/home/qt-test-server/ssl-certs/qt-test-server-cert.pem'],
        ;

        # enable mod_deflate, but make sure our custom deflate.conf is
        # deployed first (otherwise all requests will be gzipped and
        # tests will fail when they receive an unexpected Content-Length)
        "deflate":
            ensure  =>  present,
            require =>  File["/etc/apache2/mods-available/deflate.conf"],
        ;

        # used by auth-digest paths
        "auth_digest":      ensure  =>  present;
    }

    apache2_conf {
        "main.conf":    ensure  =>  present;
        "security":     ensure  =>  present;
        "ssl.conf":     ensure  =>  present;
        "dav.conf":     ensure  =>  present;
    }

    # replace the default mod_deflate conf with an empty one.
    #
    # The distro package typically provides a configuration for mod_deflate
    # which turns on deflate for all paths, for HTML and text formats.
    #
    # We do not want this, because it makes our Content-Length less predictable.
    # So, we'll make sure the global configuration does _not_ turn on deflate.
    #
    # Certain paths on the server will explicitly enable deflate (e.g. look for
    # `DEFLATE' in other .conf files)
    file { "/etc/apache2/mods-available/deflate.conf":
        source  =>  "puppet:///modules/network_test_server/config/apache2/deflate.conf",
        require =>  Package["apache2"],
        notify  =>  Service["apache2"],
    }

    # dav upload directory
    file { "/home/writeables/dav":
        ensure  =>  directory,
        mode    =>  1777,
        require =>  File["/home/writeables"],
        before  =>  Service['apache2'],
    }

    # Disable all of the "default" apache2 sites
    exec { "/usr/sbin/a2dissite '*'":
        onlyif  =>  "/bin/sh -c 'test $(ls /etc/apache2/sites-enabled | wc -l) -gt 0'",
        require =>  Package["apache2"],
        notify  =>  Service["apache2"],
    }

    # Deploy docs and scripts
    file { "/home/qt-test-server/www":
        ensure  =>  directory,
        recurse =>  remote,
        source  =>  "puppet:///modules/network_test_server/www",
        require =>  User["qt-test-server"],
        before  =>  Service['apache2'],
    }

    # Hardcoded timestamps for testing purposes on a few files
    file_timestamp {
        "/home/qt-test-server/www/htdocs/fluke.gif":
            timestamp   =>  '2007-05-22 12:04:57 GMT',
            require =>  File["/home/qt-test-server/www"],
        ;
        "/home/qt-test-server/www/htdocs/index.html":
            timestamp   =>  '2008-11-15 13:52 GMT',
            require =>  File["/home/qt-test-server/www"],
        ;
    }

    # Some testdata created by special means
    exec {
        "make mediumfile":
            command => "/bin/dd if=/dev/zero of=/home/qt-test-server/www/htdocs/mediumfile bs=1 count=0 seek=10000000",
            creates =>  "/home/qt-test-server/www/htdocs/mediumfile",
            require =>  File["/home/qt-test-server/www"],
        ;
    }

}

# Set mtime on file to given timestamp
# timestamp may be anything parseable by /bin/date
# FIXME: add support directly to puppet for this
define file_timestamp($timestamp) {
    $since_epoch = generate("/bin/sh", "-c", "date -d '$timestamp' +%s | tr -d '\n'")
    exec {
        "/usr/bin/touch -d '@$since_epoch' '$name'":
            onlyif  =>  "/bin/sh -c \"/usr/bin/stat -c '%Y' '$name' | /bin/grep -v -q '$since_epoch'\"",
            notify  =>  Service["apache2"],
        ;
    }
}

define apache2_module($ensure) {
    if $ensure == "present" {
        exec { "/usr/sbin/a2enmod $name":
            creates =>  "/etc/apache2/mods-enabled/$name.load",
            require =>  Package["apache2"],
            notify  =>  Service["apache2"],
        }
    }
    else {
        exec { "/usr/sbin/a2dismod $name":
            onlyif  =>  "/usr/bin/test -e /etc/apache2/mods-enabled/$name.load",
            require =>  Package["apache2"],
            notify  =>  Service["apache2"],
        }
    }
}

define apache2_site($ensure) {
    if $ensure == "present" {
    }
    else {
        exec { "/usr/sbin/a2dismod $name":
            onlyif  =>  "/usr/bin/test -e /etc/apache2/mods-enabled/$name.load",
            require =>  Package["apache2"],
            notify  =>  Service["apache2"],
        }
    }
}

define apache2_conf($ensure) {
    if $ensure == "present" {
        file { "/etc/apache2/conf.d/$name":
            source  =>  "puppet:///modules/network_test_server/config/apache2/$name",
            require =>  Package["apache2"],
            notify  =>  Service["apache2"],
        }
    }
    else {
        file { "/etc/apache2/conf.d/$name":
            ensure  =>  absent,
            notify  =>  Service["apache2"],
        }
    }
}


