TEMPLATE = subdirs
SUBDIRS = qmimedatabase-xml
unix:!mac:!qnx: SUBDIRS += qmimedatabase-cache
