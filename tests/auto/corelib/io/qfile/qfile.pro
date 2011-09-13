TEMPLATE = subdirs
wince*|symbian:{
  SUBDIRS = test
} else {
  SUBDIRS = test stdinprocess
}

!symbian:SUBDIRS += largefile

CONFIG += parallel_test
