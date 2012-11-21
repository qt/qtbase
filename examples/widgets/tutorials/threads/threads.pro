TEMPLATE = subdirs

SUBDIRS = hellothread \
          hellothreadpool \
          clock \
          movedobject 

contains(QT_CONFIG, concurrent): SUBDIRS += helloconcurrent

QT += widgets

# install
sources.files = threads.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/threads
INSTALLS += sources
