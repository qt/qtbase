# Use subdirs template to suppress generation of unnecessary files
TEMPLATE = subdirs

# We just want to export the sqlite3 binaries for Symbian for platforms that do not have them.
symbian-abld|symbian-sbsv2 {
    !symbian_no_export_sqlite:!exists($${EPOCROOT}epoc32/release/armv5/lib/sqlite3.dso) {
        BLD_INF_RULES.prj_exports +=  ":zip SQLite3_v9.2.zip"
    }
}
