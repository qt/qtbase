# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## DBus Module:
#####################################################################

qt_internal_add_module(DBus
    QMAKE_MODULE_CONFIG dbusadaptors dbusinterfaces
    SOURCES
        dbus_minimal_p.h
        qdbus_symbols.cpp qdbus_symbols_p.h
        qdbusabstractadaptor.cpp qdbusabstractadaptor.h qdbusabstractadaptor_p.h
        qdbusabstractinterface.cpp qdbusabstractinterface.h qdbusabstractinterface_p.h
        qdbusargument.cpp qdbusargument.h qdbusargument_p.h
        qdbusconnection.cpp qdbusconnection.h qdbusconnection_p.h
        qdbusconnectioninterface.cpp qdbusconnectioninterface.h
        qdbusconnectionmanager.cpp qdbusconnectionmanager_p.h
        qdbuscontext.cpp qdbuscontext.h qdbuscontext_p.h
        qdbuserror.cpp qdbuserror.h
        qdbusextratypes.cpp qdbusextratypes.h
        qdbusintegrator.cpp qdbusintegrator_p.h
        qdbusinterface.cpp qdbusinterface.h qdbusinterface_p.h
        qdbusinternalfilters.cpp
        qdbusintrospection.cpp qdbusintrospection_p.h
        qdbusmarshaller.cpp
        qdbusmessage.cpp qdbusmessage.h qdbusmessage_p.h
        qdbusmetaobject.cpp qdbusmetaobject_p.h
        qdbusmetatype.cpp qdbusmetatype.h qdbusmetatype_p.h
        qdbusmisc.cpp
        qdbuspendingcall.cpp qdbuspendingcall.h qdbuspendingcall_p.h
        qdbuspendingreply.cpp qdbuspendingreply.h
        qdbusreply.cpp qdbusreply.h
        qdbusserver.cpp qdbusserver.h
        qdbusservicewatcher.cpp qdbusservicewatcher.h
        qdbusthreaddebug_p.h
        qdbusunixfiledescriptor.cpp qdbusunixfiledescriptor.h
        qdbusutil.cpp qdbusutil_p.h
        qdbusvirtualobject.cpp qdbusvirtualobject.h
        qdbusxmlgenerator.cpp
        qdbusxmlparser.cpp qdbusxmlparser_p.h
        qtdbusglobal.h qtdbusglobal_p.h
    NO_UNITY_BUILD_SOURCES
        qdbusconnectionmanager.cpp # qt_windows.h clashing with "interface"
    DEFINES
        DBUS_API_SUBJECT_TO_CHANGE
        QT_NO_CONTEXTLESS_CONNECT
        QT_NO_FOREACH
        QT_NO_QPAIR
        QT_USE_NODISCARD_FILE_OPEN
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

# This file is included by qdbusargument.cpp
set_source_files_properties(qdbusmarshaller.cpp
    PROPERTIES HEADER_FILE_ONLY ON)

## Scopes:
#####################################################################

qt_internal_extend_target(DBus CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    LIBRARIES
        dbus-1
)

qt_internal_extend_target(DBus CONDITION WIN32
    LIBRARIES
        advapi32
        netapi32
        user32
        ws2_32
)
qt_internal_add_docs(DBus
    doc/qtdbus.qdocconf
)
