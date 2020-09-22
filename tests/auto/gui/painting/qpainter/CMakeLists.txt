# Generated from qpainter.pro.

#####################################################################
## tst_qpainter Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawEllipse/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawLine_rop_bitmap/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawPixmap_rop/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawPixmap_rop_bitmap/*)
list(APPEND test_data ${test_data_glob})
list(APPEND test_data "task217400.png")

qt_internal_add_test(tst_qpainter
    SOURCES
        tst_qpainter.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpainter CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
        Qt::WidgetsPrivate
)

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(testdata_resource_files
        "drawEllipse/10x10SizeAt0x0.png"
        "drawEllipse/10x10SizeAt100x100.png"
        "drawEllipse/10x10SizeAt200x200.png"
        "drawEllipse/13x100SizeAt0x0.png"
        "drawEllipse/13x100SizeAt100x100.png"
        "drawEllipse/13x100SizeAt200x200.png"
        "drawEllipse/200x200SizeAt0x0.png"
        "drawEllipse/200x200SizeAt100x100.png"
        "drawEllipse/200x200SizeAt200x200.png"
        "drawLine_rop_bitmap/dst.xbm"
        "drawLine_rop_bitmap/res/res_AndNotROP.xbm"
        "drawLine_rop_bitmap/res/res_AndROP.xbm"
        "drawLine_rop_bitmap/res/res_ClearROP.xbm"
        "drawLine_rop_bitmap/res/res_CopyROP.xbm"
        "drawLine_rop_bitmap/res/res_NandROP.xbm"
        "drawLine_rop_bitmap/res/res_NopROP.xbm"
        "drawLine_rop_bitmap/res/res_NorROP.xbm"
        "drawLine_rop_bitmap/res/res_NotAndROP.xbm"
        "drawLine_rop_bitmap/res/res_NotCopyROP.xbm"
        "drawLine_rop_bitmap/res/res_NotOrROP.xbm"
        "drawLine_rop_bitmap/res/res_NotROP.xbm"
        "drawLine_rop_bitmap/res/res_NotXorROP.xbm"
        "drawLine_rop_bitmap/res/res_OrNotROP.xbm"
        "drawLine_rop_bitmap/res/res_OrROP.xbm"
        "drawLine_rop_bitmap/res/res_SetROP.xbm"
        "drawLine_rop_bitmap/res/res_XorROP.xbm"
        "drawPixmap_rop/dst1.png"
        "drawPixmap_rop/dst2.png"
        "drawPixmap_rop/dst3.png"
        "drawPixmap_rop/res/res_AndNotROP0.png"
        "drawPixmap_rop/res/res_AndNotROP1.png"
        "drawPixmap_rop/res/res_AndNotROP2.png"
        "drawPixmap_rop/res/res_AndNotROP3.png"
        "drawPixmap_rop/res/res_AndNotROP4.png"
        "drawPixmap_rop/res/res_AndNotROP5.png"
        "drawPixmap_rop/res/res_AndNotROP6.png"
        "drawPixmap_rop/res/res_AndNotROP7.png"
        "drawPixmap_rop/res/res_AndROP0.png"
        "drawPixmap_rop/res/res_AndROP1.png"
        "drawPixmap_rop/res/res_AndROP2.png"
        "drawPixmap_rop/res/res_AndROP3.png"
        "drawPixmap_rop/res/res_AndROP4.png"
        "drawPixmap_rop/res/res_AndROP5.png"
        "drawPixmap_rop/res/res_AndROP6.png"
        "drawPixmap_rop/res/res_AndROP7.png"
        "drawPixmap_rop/res/res_ClearROP0.png"
        "drawPixmap_rop/res/res_ClearROP1.png"
        "drawPixmap_rop/res/res_ClearROP2.png"
        "drawPixmap_rop/res/res_ClearROP3.png"
        "drawPixmap_rop/res/res_ClearROP4.png"
        "drawPixmap_rop/res/res_ClearROP5.png"
        "drawPixmap_rop/res/res_ClearROP6.png"
        "drawPixmap_rop/res/res_ClearROP7.png"
        "drawPixmap_rop/res/res_CopyROP0.png"
        "drawPixmap_rop/res/res_CopyROP1.png"
        "drawPixmap_rop/res/res_CopyROP2.png"
        "drawPixmap_rop/res/res_CopyROP3.png"
        "drawPixmap_rop/res/res_CopyROP4.png"
        "drawPixmap_rop/res/res_CopyROP5.png"
        "drawPixmap_rop/res/res_CopyROP6.png"
        "drawPixmap_rop/res/res_CopyROP7.png"
        "drawPixmap_rop/res/res_NandROP0.png"
        "drawPixmap_rop/res/res_NandROP1.png"
        "drawPixmap_rop/res/res_NandROP2.png"
        "drawPixmap_rop/res/res_NandROP3.png"
        "drawPixmap_rop/res/res_NandROP4.png"
        "drawPixmap_rop/res/res_NandROP5.png"
        "drawPixmap_rop/res/res_NandROP6.png"
        "drawPixmap_rop/res/res_NandROP7.png"
        "drawPixmap_rop/res/res_NopROP0.png"
        "drawPixmap_rop/res/res_NopROP1.png"
        "drawPixmap_rop/res/res_NopROP2.png"
        "drawPixmap_rop/res/res_NopROP3.png"
        "drawPixmap_rop/res/res_NopROP4.png"
        "drawPixmap_rop/res/res_NopROP5.png"
        "drawPixmap_rop/res/res_NopROP6.png"
        "drawPixmap_rop/res/res_NopROP7.png"
        "drawPixmap_rop/res/res_NorROP0.png"
        "drawPixmap_rop/res/res_NorROP1.png"
        "drawPixmap_rop/res/res_NorROP2.png"
        "drawPixmap_rop/res/res_NorROP3.png"
        "drawPixmap_rop/res/res_NorROP4.png"
        "drawPixmap_rop/res/res_NorROP5.png"
        "drawPixmap_rop/res/res_NorROP6.png"
        "drawPixmap_rop/res/res_NorROP7.png"
        "drawPixmap_rop/res/res_NotAndROP0.png"
        "drawPixmap_rop/res/res_NotAndROP1.png"
        "drawPixmap_rop/res/res_NotAndROP2.png"
        "drawPixmap_rop/res/res_NotAndROP3.png"
        "drawPixmap_rop/res/res_NotAndROP4.png"
        "drawPixmap_rop/res/res_NotAndROP5.png"
        "drawPixmap_rop/res/res_NotAndROP6.png"
        "drawPixmap_rop/res/res_NotAndROP7.png"
        "drawPixmap_rop/res/res_NotCopyROP0.png"
        "drawPixmap_rop/res/res_NotCopyROP1.png"
        "drawPixmap_rop/res/res_NotCopyROP2.png"
        "drawPixmap_rop/res/res_NotCopyROP3.png"
        "drawPixmap_rop/res/res_NotCopyROP4.png"
        "drawPixmap_rop/res/res_NotCopyROP5.png"
        "drawPixmap_rop/res/res_NotCopyROP6.png"
        "drawPixmap_rop/res/res_NotCopyROP7.png"
        "drawPixmap_rop/res/res_NotOrROP0.png"
        "drawPixmap_rop/res/res_NotOrROP1.png"
        "drawPixmap_rop/res/res_NotOrROP2.png"
        "drawPixmap_rop/res/res_NotOrROP3.png"
        "drawPixmap_rop/res/res_NotOrROP4.png"
        "drawPixmap_rop/res/res_NotOrROP5.png"
        "drawPixmap_rop/res/res_NotOrROP6.png"
        "drawPixmap_rop/res/res_NotOrROP7.png"
        "drawPixmap_rop/res/res_NotROP0.png"
        "drawPixmap_rop/res/res_NotROP1.png"
        "drawPixmap_rop/res/res_NotROP2.png"
        "drawPixmap_rop/res/res_NotROP3.png"
        "drawPixmap_rop/res/res_NotROP4.png"
        "drawPixmap_rop/res/res_NotROP5.png"
        "drawPixmap_rop/res/res_NotROP6.png"
        "drawPixmap_rop/res/res_NotROP7.png"
        "drawPixmap_rop/res/res_NotXorROP0.png"
        "drawPixmap_rop/res/res_NotXorROP1.png"
        "drawPixmap_rop/res/res_NotXorROP2.png"
        "drawPixmap_rop/res/res_NotXorROP3.png"
        "drawPixmap_rop/res/res_NotXorROP4.png"
        "drawPixmap_rop/res/res_NotXorROP5.png"
        "drawPixmap_rop/res/res_NotXorROP6.png"
        "drawPixmap_rop/res/res_NotXorROP7.png"
        "drawPixmap_rop/res/res_OrNotROP0.png"
        "drawPixmap_rop/res/res_OrNotROP1.png"
        "drawPixmap_rop/res/res_OrNotROP2.png"
        "drawPixmap_rop/res/res_OrNotROP3.png"
        "drawPixmap_rop/res/res_OrNotROP4.png"
        "drawPixmap_rop/res/res_OrNotROP5.png"
        "drawPixmap_rop/res/res_OrNotROP6.png"
        "drawPixmap_rop/res/res_OrNotROP7.png"
        "drawPixmap_rop/res/res_OrROP0.png"
        "drawPixmap_rop/res/res_OrROP1.png"
        "drawPixmap_rop/res/res_OrROP2.png"
        "drawPixmap_rop/res/res_OrROP3.png"
        "drawPixmap_rop/res/res_OrROP4.png"
        "drawPixmap_rop/res/res_OrROP5.png"
        "drawPixmap_rop/res/res_OrROP6.png"
        "drawPixmap_rop/res/res_OrROP7.png"
        "drawPixmap_rop/res/res_SetROP0.png"
        "drawPixmap_rop/res/res_SetROP1.png"
        "drawPixmap_rop/res/res_SetROP2.png"
        "drawPixmap_rop/res/res_SetROP3.png"
        "drawPixmap_rop/res/res_SetROP4.png"
        "drawPixmap_rop/res/res_SetROP5.png"
        "drawPixmap_rop/res/res_SetROP6.png"
        "drawPixmap_rop/res/res_SetROP7.png"
        "drawPixmap_rop/res/res_XorROP0.png"
        "drawPixmap_rop/res/res_XorROP1.png"
        "drawPixmap_rop/res/res_XorROP2.png"
        "drawPixmap_rop/res/res_XorROP3.png"
        "drawPixmap_rop/res/res_XorROP4.png"
        "drawPixmap_rop/res/res_XorROP5.png"
        "drawPixmap_rop/res/res_XorROP6.png"
        "drawPixmap_rop/res/res_XorROP7.png"
        "drawPixmap_rop/src1.xbm"
        "drawPixmap_rop/src2-mask.xbm"
        "drawPixmap_rop/src2.xbm"
        "drawPixmap_rop/src3.xbm"
        "drawPixmap_rop_bitmap/dst.xbm"
        "drawPixmap_rop_bitmap/res/res_AndNotROP.xbm"
        "drawPixmap_rop_bitmap/res/res_AndROP.xbm"
        "drawPixmap_rop_bitmap/res/res_ClearROP.xbm"
        "drawPixmap_rop_bitmap/res/res_CopyROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NandROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NopROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NorROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NotAndROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NotCopyROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NotOrROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NotROP.xbm"
        "drawPixmap_rop_bitmap/res/res_NotXorROP.xbm"
        "drawPixmap_rop_bitmap/res/res_OrNotROP.xbm"
        "drawPixmap_rop_bitmap/res/res_OrROP.xbm"
        "drawPixmap_rop_bitmap/res/res_SetROP.xbm"
        "drawPixmap_rop_bitmap/res/res_XorROP.xbm"
        "drawPixmap_rop_bitmap/src1-mask.xbm"
        "drawPixmap_rop_bitmap/src1.xbm"
        "drawPixmap_rop_bitmap/src2.xbm"
        "task217400.png"
    )

    qt_internal_add_resource(tst_qpainter "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()
