load(qttest_p4)
SOURCES  += ../tst_qclipboard.cpp
TARGET = ../tst_qclipboard

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qclipboard
} else {
    TARGET = ../../release/tst_qclipboard
  }
}

wince*|symbian: {
  copier.files = ../copier/copier.exe
  copier.path = copier
  paster.files = ../paster/paster.exe  
  paster.path = paster
  
  symbian: {
    LIBS += -lbafl -lestor -letext

    load(data_caging_paths)
    rsc.files = $${EPOCROOT}$$HW_ZDIR$$APP_RESOURCE_DIR/copier.rsc
    rsc.files += $${EPOCROOT}$$HW_ZDIR$$APP_RESOURCE_DIR/paster.rsc
    rsc.path = $$APP_RESOURCE_DIR
    reg_resource.files = $${EPOCROOT}$$HW_ZDIR$$REG_RESOURCE_IMPORT_DIR/copier_reg.rsc
    reg_resource.files += $${EPOCROOT}$$HW_ZDIR$$REG_RESOURCE_IMPORT_DIR/paster_reg.rsc
    reg_resource.path = $$REG_RESOURCE_IMPORT_DIR
  }
  
  DEPLOYMENT += copier paster rsc reg_resource
}