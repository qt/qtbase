V8DIR = $$(V8DIR)
isEmpty(V8DIR) {
    V8DIR = $$PWD/../3rdparty/v8
} else {
    message(using external V8 from $$V8DIR)
}

*-g++*: {
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

    # mksnapshot hangs if gcc 4.5 is used
    # for reference look at http://code.google.com/p/v8/issues/detail?id=884
    equals(QT_GCC_MAJOR_VERSION, 4): equals(QT_GCC_MINOR_VERSION, 5) {
      message(because of a bug in gcc / v8 we need to add -fno-strict-aliasing)
      QMAKE_CFLAGS += -fno-strict-aliasing
      QMAKE_CXXFLAGS += -fno-strict-aliasing
    }
}
