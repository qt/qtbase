SOURCES += futimens.cpp

# Block futimens() on Apple platforms unless it's available on ALL deployment
# targets. This simplifies the logic at the call site dramatically, as it isn't
# strictly needed compared to futimes().
darwin: QMAKE_CXXFLAGS += -Werror=unguarded-availability
