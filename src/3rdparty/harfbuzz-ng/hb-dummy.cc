// Work-around for issue with qmake: Since hb-common.cc has #include "hb-static.cc" in it,
// qmake will assume it is included and ignore it in the SOURCES list. But the #include
// is protected inside an #ifdef and will not be used, so hb-static.cc ends up not being
// linked at all and we get missing symbols. We work around this by including both in
// the same compilation unit.

#include "src/hb-common.cc"
#include "src/hb-static.cc"
