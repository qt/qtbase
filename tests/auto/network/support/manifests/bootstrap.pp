
# Run apt-get update once so we have all the latest packages
exec { 'apt-update':
  command => "/usr/bin/apt-get update",
  onlyif  => "/bin/sh -c '[ ! -f /var/cache/apt/pkgcache.bin ]'"
}

Exec['apt-update'] -> Package <| |>

package {
  'build-essential': ensure => present;
  'whois':           ensure => present;
}

# The puppet install of the base precise64 box doesn't have the
# neccecary gem for puppet to be able to set user passwords.
package { 'libshadow':
  ensure   => present,
  provider => 'gem',
  require  => Package['build-essential'],
}
