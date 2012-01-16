TEMPLATE = subdirs
wince* {
  SUBDIRS = test
} else {
  SUBDIRS = test stdinprocess
}

CONFIG += parallel_test
