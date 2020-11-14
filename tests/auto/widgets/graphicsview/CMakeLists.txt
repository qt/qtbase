# Generated from graphicsview.pro.

if(QT_FEATURE_private_tests)
    add_subdirectory(qgraphicsanchorlayout)
    add_subdirectory(qgraphicsanchorlayout1)
    add_subdirectory(qgraphicsitem)
    add_subdirectory(qgraphicsscene)
    add_subdirectory(qgraphicssceneindex)
endif()
add_subdirectory(qgraphicseffectsource)
add_subdirectory(qgraphicsgridlayout)
add_subdirectory(qgraphicsitemanimation)
add_subdirectory(qgraphicslayout)
add_subdirectory(qgraphicslayoutitem)
add_subdirectory(qgraphicslinearlayout)
add_subdirectory(qgraphicsobject)
add_subdirectory(qgraphicspixmapitem)
add_subdirectory(qgraphicspolygonitem)
add_subdirectory(qgraphicstransform)
if(QT_FEATURE_style_fusion)
    # special case begin
    # These tests are never run in coin with qmake since the statement
    # !qtConfig(style-fusion): always passes causing the subdirs to be removed.
    # For these tests to be generated the .pro file needs to be patched and
    # the above line changed to !qtConfig(style-fusion). That being said,
    # one of the test fails and needs to be corrected by someone with know how in
    # that area. Until then we will disable them to mimic the behavior seen in
    # coin.
    #add_subdirectory(qgraphicsproxywidget)
    #add_subdirectory(qgraphicswidget)
    # special case end
endif()
# QTBUG-87671 # special case
if(NOT ANDROID)
    add_subdirectory(qgraphicsview)
endif()
