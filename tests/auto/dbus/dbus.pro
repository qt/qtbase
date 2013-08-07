TEMPLATE=subdirs
SUBDIRS=\
           qdbusabstractadaptor \
           qdbusabstractinterface \
           qdbusconnection \
           qdbusconnection_no_bus \
           qdbuscontext \
           qdbusinterface \
           qdbuslocalcalls \
           qdbusmarshall \
           qdbusmetaobject \
           qdbusmetatype \
           qdbuspendingcall \
           qdbuspendingreply \
           qdbusreply \
           qdbusservicewatcher \
           qdbustype \
           qdbusthreading \
           qdbusxmlparser \

!contains(QT_CONFIG,private_tests): SUBDIRS -= \
           qdbusmarshall \

