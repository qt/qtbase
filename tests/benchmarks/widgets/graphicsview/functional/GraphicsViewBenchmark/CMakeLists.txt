# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_GraphicsViewBenchmark Binary:
#####################################################################

qt_internal_add_benchmark(tst_GraphicsViewBenchmark
    SOURCES
        main.cpp
        widgets/abstractitemcontainer.cpp widgets/abstractitemcontainer.h
        widgets/abstractitemview.cpp widgets/abstractitemview.h
        widgets/abstractscrollarea.cpp widgets/abstractscrollarea.h
        widgets/abstractviewitem.cpp widgets/abstractviewitem.h
        widgets/backgrounditem.cpp widgets/backgrounditem.h
        widgets/button.cpp widgets/button.h
        widgets/commandline.cpp widgets/commandline.h
        widgets/dummydatagen.cpp widgets/dummydatagen.h
        widgets/gvbwidget.cpp widgets/gvbwidget.h
        widgets/iconitem.cpp widgets/iconitem.h
        widgets/itemrecyclinglist.cpp widgets/itemrecyclinglist.h
        widgets/itemrecyclinglistview.cpp widgets/itemrecyclinglistview.h
        widgets/label.cpp widgets/label.h
        widgets/listitem.cpp widgets/listitem.h
        widgets/listitemcache.cpp widgets/listitemcache.h
        widgets/listitemcontainer.cpp widgets/listitemcontainer.h
        widgets/listmodel.cpp widgets/listmodel.h
        widgets/listwidget.cpp widgets/listwidget.h
        widgets/mainview.cpp widgets/mainview.h
        widgets/menu.cpp widgets/menu.h
        widgets/recycledlistitem.cpp widgets/recycledlistitem.h
        widgets/scrollbar.cpp widgets/scrollbar.h
        widgets/scroller.cpp widgets/scroller.h widgets/scroller_p.h
        widgets/settings.cpp widgets/settings.h
        widgets/simplelist.cpp widgets/simplelist.h
        widgets/simplelistview.cpp widgets/simplelistview.h
        widgets/theme.cpp widgets/theme.h
        widgets/themeevent.cpp widgets/themeevent.h
        widgets/topbar.cpp widgets/topbar.h
    INCLUDE_DIRECTORIES
        widgets
    LIBRARIES
        Qt::Gui
        Qt::Svg
        Qt::Test
)

# Resources:
set_source_files_properties("resources/blue_SVG/blue_background_360x640px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "background.svg"
)
set_source_files_properties("resources/blue_SVG/blue_background_horisontal_640x360px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "background_horisontal.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_default_icon_52x52px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_default_icon.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_list_divider_360x76px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_list_divider.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_list_highlighter_360x76px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_list_highlighter.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_status_idle_33x33px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_status_idle.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_status_offline_33x33px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_status_offline.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_status_online_33x33px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_status_online.svg"
)
set_source_files_properties("resources/blue_SVG/blue_scroll_16x80px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "scroll.svg"
)
set_source_files_properties("resources/blue_SVG/blue_scrollbar_7x14px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "scrollbar.svg"
)
set_source_files_properties("resources/blue_SVG/blue_status_field_left_14x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "status_field_left.svg"
)
set_source_files_properties("resources/blue_SVG/blue_status_field_middle_14x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "status_field_middle.svg"
)
set_source_files_properties("resources/blue_SVG/blue_status_field_right_14x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "status_field_right.svg"
)
set_source_files_properties("resources/blue_SVG/blue_topbar_356x96px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "topbar.svg"
)
set_source_files_properties("resources/blue_SVG/blue_topbar_horisontal_636x96px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "topbar_horisontal.svg"
)
set_source_files_properties("resources/blue_SVG/blue_user_default_icon_68x68px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_default_icon.svg"
)
set_source_files_properties("resources/blue_SVG/blue_user_status_idle_38x38px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_status_idle.svg"
)
set_source_files_properties("resources/blue_SVG/blue_user_status_offline_38x38px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_status_offline.svg"
)
set_source_files_properties("resources/blue_SVG/blue_user_status_online_38x38px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_status_online.svg"
)
set(GraphicsViewBenchmark_resource_files
    "resources/blue_SVG/blue_background_360x640px.svg"
    "resources/blue_SVG/blue_background_horisontal_640x360px.svg"
    "resources/blue_SVG/blue_contact_default_icon_52x52px.svg"
    "resources/blue_SVG/blue_contact_list_divider_360x76px.svg"
    "resources/blue_SVG/blue_contact_list_highlighter_360x76px.svg"
    "resources/blue_SVG/blue_contact_status_idle_33x33px.svg"
    "resources/blue_SVG/blue_contact_status_offline_33x33px.svg"
    "resources/blue_SVG/blue_contact_status_online_33x33px.svg"
    "resources/blue_SVG/blue_scroll_16x80px.svg"
    "resources/blue_SVG/blue_scrollbar_7x14px.svg"
    "resources/blue_SVG/blue_status_field_left_14x24px.svg"
    "resources/blue_SVG/blue_status_field_middle_14x24px.svg"
    "resources/blue_SVG/blue_status_field_right_14x24px.svg"
    "resources/blue_SVG/blue_topbar_356x96px.svg"
    "resources/blue_SVG/blue_topbar_horisontal_636x96px.svg"
    "resources/blue_SVG/blue_user_default_icon_68x68px.svg"
    "resources/blue_SVG/blue_user_status_idle_38x38px.svg"
    "resources/blue_SVG/blue_user_status_offline_38x38px.svg"
    "resources/blue_SVG/blue_user_status_online_38x38px.svg"
)

qt_internal_add_resource(tst_GraphicsViewBenchmark "GraphicsViewBenchmark"
    PREFIX
        "/themes/blue"
    FILES
        ${GraphicsViewBenchmark_resource_files}
)
set_source_files_properties("resources/blue_SVG/blue_contact_status_idle_33x33px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_status_idle.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_status_offline_33x33px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_status_offline.svg"
)
set_source_files_properties("resources/blue_SVG/blue_contact_status_online_33x33px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_status_online.svg"
)
set_source_files_properties("resources/lime_SVG/lime_background_360x640px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "background.svg"
)
set_source_files_properties("resources/lime_SVG/lime_background_horisontal_640x360px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "background_horisontal.svg"
)
set_source_files_properties("resources/lime_SVG/lime_contact_default_icon_53x53px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_default_icon.svg"
)
set_source_files_properties("resources/lime_SVG/lime_contact_default_icon_highlight_53x53px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_default_icon_highlight.svg"
)
set_source_files_properties("resources/lime_SVG/lime_contact_list_divider_360x76px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_list_divider.svg"
)
set_source_files_properties("resources/lime_SVG/lime_contact_list_highlighter_357x80px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "contact_list_highlighter.svg"
)
set_source_files_properties("resources/lime_SVG/lime_scroll_5x80px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "scroll.svg"
)
set_source_files_properties("resources/lime_SVG/lime_scrollbar_5x14px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "scrollbar.svg"
)
set_source_files_properties("resources/lime_SVG/lime_status_field_left_14x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "status_field_left.svg"
)
set_source_files_properties("resources/lime_SVG/lime_status_field_middle_10x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "status_field_middle.svg"
)
set_source_files_properties("resources/lime_SVG/lime_status_field_right_14x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "status_field_right.svg"
)
set_source_files_properties("resources/lime_SVG/lime_topbar_356x96px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "topbar.svg"
)
set_source_files_properties("resources/lime_SVG/lime_topbar_horisontal_636x96px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "topbar_horisontal.svg"
)
set_source_files_properties("resources/lime_SVG/lime_user_default_icon_84x68px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_default_icon.svg"
)
set_source_files_properties("resources/lime_SVG/lime_user_status_idle_24x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_status_idle.svg"
)
set_source_files_properties("resources/lime_SVG/lime_user_status_offline_24x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_status_offline.svg"
)
set_source_files_properties("resources/lime_SVG/lime_user_status_online_24x24px.svg"
    PROPERTIES QT_RESOURCE_ALIAS "user_status_online.svg"
)
set(GraphicsViewBenchmark1_resource_files
    "resources/blue_SVG/blue_contact_status_idle_33x33px.svg"
    "resources/blue_SVG/blue_contact_status_offline_33x33px.svg"
    "resources/blue_SVG/blue_contact_status_online_33x33px.svg"
    "resources/lime_SVG/lime_background_360x640px.svg"
    "resources/lime_SVG/lime_background_horisontal_640x360px.svg"
    "resources/lime_SVG/lime_contact_default_icon_53x53px.svg"
    "resources/lime_SVG/lime_contact_default_icon_highlight_53x53px.svg"
    "resources/lime_SVG/lime_contact_list_divider_360x76px.svg"
    "resources/lime_SVG/lime_contact_list_highlighter_357x80px.svg"
    "resources/lime_SVG/lime_scroll_5x80px.svg"
    "resources/lime_SVG/lime_scrollbar_5x14px.svg"
    "resources/lime_SVG/lime_status_field_left_14x24px.svg"
    "resources/lime_SVG/lime_status_field_middle_10x24px.svg"
    "resources/lime_SVG/lime_status_field_right_14x24px.svg"
    "resources/lime_SVG/lime_topbar_356x96px.svg"
    "resources/lime_SVG/lime_topbar_horisontal_636x96px.svg"
    "resources/lime_SVG/lime_user_default_icon_84x68px.svg"
    "resources/lime_SVG/lime_user_status_idle_24x24px.svg"
    "resources/lime_SVG/lime_user_status_offline_24x24px.svg"
    "resources/lime_SVG/lime_user_status_online_24x24px.svg"
)

qt_internal_add_resource(tst_GraphicsViewBenchmark "GraphicsViewBenchmark1"
    PREFIX
        "/themes/lime"
    FILES
        ${GraphicsViewBenchmark1_resource_files}
)
set_source_files_properties("resources/avatars/avatar_man_001_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_001.png"
)
set_source_files_properties("resources/avatars/avatar_man_002_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_002.png"
)
set_source_files_properties("resources/avatars/avatar_man_003_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_003.png"
)
set_source_files_properties("resources/avatars/avatar_man_004_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_004.png"
)
set_source_files_properties("resources/avatars/avatar_man_005_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_005.png"
)
set_source_files_properties("resources/avatars/avatar_man_006_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_006.png"
)
set_source_files_properties("resources/avatars/avatar_man_007_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_007.png"
)
set_source_files_properties("resources/avatars/avatar_man_008_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_008.png"
)
set_source_files_properties("resources/avatars/avatar_man_009_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_009.png"
)
set_source_files_properties("resources/avatars/avatar_man_010_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_010.png"
)
set_source_files_properties("resources/avatars/avatar_picture_001_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_011.png"
)
set_source_files_properties("resources/avatars/avatar_picture_002_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_012.png"
)
set_source_files_properties("resources/avatars/avatar_picture_003_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_013.png"
)
set_source_files_properties("resources/avatars/avatar_picture_004_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_014.png"
)
set_source_files_properties("resources/avatars/avatar_picture_005_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_015.png"
)
set_source_files_properties("resources/avatars/avatar_woman_001_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_016.png"
)
set_source_files_properties("resources/avatars/avatar_woman_002_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_017.png"
)
set_source_files_properties("resources/avatars/avatar_woman_003_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_018.png"
)
set_source_files_properties("resources/avatars/avatar_woman_004_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_019.png"
)
set_source_files_properties("resources/avatars/avatar_woman_005_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_020.png"
)
set_source_files_properties("resources/avatars/avatar_woman_006_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_021.png"
)
set_source_files_properties("resources/avatars/avatar_woman_007_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_022.png"
)
set_source_files_properties("resources/avatars/avatar_woman_008_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_023.png"
)
set_source_files_properties("resources/avatars/avatar_woman_009_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_024.png"
)
set_source_files_properties("resources/avatars/avatar_woman_010_58x58.png"
    PROPERTIES QT_RESOURCE_ALIAS "avatar_025.png"
)
set(GraphicsViewBenchmark2_resource_files
    "resources/avatars/avatar_man_001_58x58.png"
    "resources/avatars/avatar_man_002_58x58.png"
    "resources/avatars/avatar_man_003_58x58.png"
    "resources/avatars/avatar_man_004_58x58.png"
    "resources/avatars/avatar_man_005_58x58.png"
    "resources/avatars/avatar_man_006_58x58.png"
    "resources/avatars/avatar_man_007_58x58.png"
    "resources/avatars/avatar_man_008_58x58.png"
    "resources/avatars/avatar_man_009_58x58.png"
    "resources/avatars/avatar_man_010_58x58.png"
    "resources/avatars/avatar_picture_001_58x58.png"
    "resources/avatars/avatar_picture_002_58x58.png"
    "resources/avatars/avatar_picture_003_58x58.png"
    "resources/avatars/avatar_picture_004_58x58.png"
    "resources/avatars/avatar_picture_005_58x58.png"
    "resources/avatars/avatar_woman_001_58x58.png"
    "resources/avatars/avatar_woman_002_58x58.png"
    "resources/avatars/avatar_woman_003_58x58.png"
    "resources/avatars/avatar_woman_004_58x58.png"
    "resources/avatars/avatar_woman_005_58x58.png"
    "resources/avatars/avatar_woman_006_58x58.png"
    "resources/avatars/avatar_woman_007_58x58.png"
    "resources/avatars/avatar_woman_008_58x58.png"
    "resources/avatars/avatar_woman_009_58x58.png"
    "resources/avatars/avatar_woman_010_58x58.png"
)

qt_internal_add_resource(tst_GraphicsViewBenchmark "GraphicsViewBenchmark2"
    PREFIX
        "/avatars"
    FILES
        ${GraphicsViewBenchmark2_resource_files}
)
set_source_files_properties("resources/contacts/areacodes.txt"
    PROPERTIES QT_RESOURCE_ALIAS "areacodes.txt"
)
set_source_files_properties("resources/contacts/firstnamesF.txt"
    PROPERTIES QT_RESOURCE_ALIAS "firstnamesF.txt"
)
set_source_files_properties("resources/contacts/firstnamesM.txt"
    PROPERTIES QT_RESOURCE_ALIAS "firstnamesM.txt"
)
set_source_files_properties("resources/contacts/lastnames.txt"
    PROPERTIES QT_RESOURCE_ALIAS "lastnames.txt"
)
set(GraphicsViewBenchmark3_resource_files
    "resources/contacts/areacodes.txt"
    "resources/contacts/firstnamesF.txt"
    "resources/contacts/firstnamesM.txt"
    "resources/contacts/lastnames.txt"
)

qt_internal_add_resource(tst_GraphicsViewBenchmark "GraphicsViewBenchmark3"
    PREFIX
        "/contact"
    FILES
        ${GraphicsViewBenchmark3_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_GraphicsViewBenchmark CONDITION TARGET Qt::OpenGL
    LIBRARIES
        Qt::OpenGL
        Qt::OpenGLWidgets
)
