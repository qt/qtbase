class network_test_server {

  if $::hostname != "precise64" {
    warning("This puppet script is tailored for Vagrant's 'precice64' box")
  }

  include network_test_server::apache2
  include network_test_server::ssl_certs
  include network_test_server::squid
  include network_test_server::danted
  include network_test_server::vsftpd
  include network_test_server::frox
  include network_test_server::xinetd
  include network_test_server::cyrus
  include network_test_server::samba
  include network_test_server::tmpreaper
  include network_test_server::sshd
  include network_test_server::openssl_server_psk

  user { 'qt-test-server':
    ensure     => present,
    managehome => true,
  }

  user { 'qsockstest':
    ensure    =>  present,
    home      =>  "/dev/null",
    password  =>  mkpasswd('AtbhQrjz', 'password'),
    shell     =>  "/bin/false",
  }

  host {
      "qt-test-server.qt-test-net":
          ip           => $::ipaddress,
          host_aliases => [ "qt-test-server" ],
      ;
      "localhost.localdomain":
          ip           => "127.0.0.1",
          host_aliases => [ "localhost" ],
      ;
  }

  file {
      "/home/qt-test-server/passwords":
          source  =>  "puppet:///modules/network_test_server/config/passwords",
          require =>  User["qt-test-server"],
      ;
      "/home/qt-test-server/iptables":
          source  =>  "puppet:///modules/network_test_server/config/iptables",
          require =>  User["qt-test-server"],
          notify  =>  Exec["load iptables config"],
      ;
      "/etc/rc.local":
          source  =>  "puppet:///modules/network_test_server/init/rc.local",
          mode    =>  0755,
      ;
      "/home/writeables":
          ensure  =>  directory,
      ;
  }

  exec { "load iptables config":
      command     =>  "/bin/sh -c '/sbin/iptables-restore < /home/qt-test-server/iptables'",
      refreshonly =>  true,
      subscribe   =>  File["/home/qt-test-server/iptables"],
  }
}

