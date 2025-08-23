# Create an empty interface library for WrapCPDB when CPDB is not available
add_library(WrapCPDB::WrapCPDB INTERFACE IMPORTED GLOBAL)
set(WrapCPDB_FOUND TRUE)
