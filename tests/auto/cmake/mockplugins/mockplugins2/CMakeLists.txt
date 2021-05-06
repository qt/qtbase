qt_internal_add_module(MockPlugins2
    SOURCES
        fake.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)
