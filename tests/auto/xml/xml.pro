TEMPLATE=subdirs
SUBDIRS=\
   dom \

# QTBUG-87671
android: SUBDIRS -= \
            dom
