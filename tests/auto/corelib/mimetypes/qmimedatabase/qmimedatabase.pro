TEMPLATE = subdirs
SUBDIRS = qmimedatabase-xml
unix:!mac: SUBDIRS += qmimedatabase-cache
