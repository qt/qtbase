TEMPLATE=subdirs
SUBDIRS=\
   selftest \
   access \
   kernel \
   ssl \
   socket \

win32:    socket.CONFIG += no_check_target      # QTBUG-24451 - all socket tests require waitForX
