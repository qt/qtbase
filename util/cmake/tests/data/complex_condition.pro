!system("dbus-send --session --type=signal / local.AutotestCheck.Hello >$$QMAKE_SYSTEM_NULL_DEVICE 2>&1") {
    SOURCES = dbus.cpp
}

