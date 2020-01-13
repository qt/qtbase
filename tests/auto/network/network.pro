TEMPLATE=subdirs
SUBDIRS=\
   selftest \
   access \
   bearer \
   kernel \
   ssl \
   socket \

win32:    socket.CONFIG += no_check_target      # QTBUG-24451 - all socket tests require waitForX
win32|mac:bearer.CONFIG += no_check_target      # QTBUG-24503 - these tests fail if machine has a WLAN adaptor
