TEMPLATE = subdirs

SUBDIRS = hellothread \
          hellothreadpool \
          clock \
          movedobject 

contains(QT_CONFIG, concurrent): SUBDIRS += helloconcurrent
