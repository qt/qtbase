# Generated from platforminputcontexts.pro.

if(QT_FEATURE_xkbcommon)
    add_subdirectory(compose)
endif()
if(QT_FEATURE_xkbcommon AND TARGET Qt::DBus AND UNIX AND NOT MACOS)
    add_subdirectory(ibus)
endif()
