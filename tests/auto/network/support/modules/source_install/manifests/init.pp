define source_install(

    # URL of the source package to install
    $url = undef,
    $configureparams = undef,
    $creates,

){

    $source_filename = regsubst($url, '^.*/(.*)\?*','\1')
    $extract_folder = regsubst($source_filename, '^(.*)\.tar\.gz','\1')

    # directory to temporarily hold the downloaded archive
    # baselayout doesn't work in network_test_server. TODO: look at this
    # $tempdir = $baselayout::tempdir
    $tempdir = "/tmp"



    exec { "Download ${url}":
        cwd     =>  $tempdir,
        command =>  "/usr/bin/wget ${url}",
        timeout =>  3600,
        creates =>  "$tempdir/${source_filename}",
        notify  =>  Exec["Extract $source_filename"],
        unless  =>  "/usr/bin/test -e $creates",
    }

    exec { "Extract ${source_filename}":
        cwd         =>  $tempdir,
        command     =>  "/bin/tar -xvf ${source_filename}",
        creates     =>  "$tempdir/$extract_folder",
        subscribe   =>  Exec["Download ${url}"],
        notify      =>  Exec["Configure ${source_filename}"],
        refreshonly =>  true,
    }

    exec { "Configure ${source_filename}":
        cwd         =>  "$tempdir/$extract_folder",
        command     =>  "/bin/sh ./configure $configureparams",
        creates     =>  "$tempdir/$extract_folder/Makefile",
        subscribe   =>  Exec["Extract ${source_filename}"],
        notify      =>  Exec["Make ${source_filename}"],
        refreshonly =>  true,
    }

    exec { "Make ${source_filename}":
        cwd       =>  "$tempdir/$extract_folder",
        command   =>  "/usr/bin/make && /usr/bin/make install",
        creates   =>  $creates,
        subscribe =>  Exec["Configure ${source_filename}"],
    }

}
