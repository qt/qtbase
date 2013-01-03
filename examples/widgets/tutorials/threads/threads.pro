TEMPLATE = subdirs

SUBDIRS = hellothread \
          hellothreadpool \
          clock \
          movedobject 

qtHaveModule(concurrent): SUBDIRS += helloconcurrent
