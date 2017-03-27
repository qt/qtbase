TEMPLATE=subdirs

# Run this test first
SUBDIRS=\
           qdbusconnection_delayed

SUBDIRS+=\
           qdbusabstractadaptor \
           qdbusabstractinterface \
           qdbusconnection \
           qdbusconnection_no_app \
           qdbusconnection_no_bus \
           qdbusconnection_no_libdbus \
           qdbusconnection_spyhook \
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
           qdbusxmlparser

!qtConfig(private_tests): SUBDIRS -= \
           qdbusmarshall \

!qtConfig(process): SUBDIRS -= \
           qdbusabstractadaptor \
           qdbusabstractinterface \
           qdbusinterface \
           qdbusmarshall

!qtHaveModule(xml): SUBDIRS -= \
           qdbusxmlparser
