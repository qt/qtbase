load(qttest_p4)
SOURCES  += tst_qtablewidget.cpp

# This prevents the GCCE compile failure: "elf2e32: Error 1063: Fatal Error in
# PostLinker." The paged statement is documented in the S60 docs.
symbian {
    MMP_RULES -= PAGED

    custom_paged_rule = "$${LITERAL_HASH}ifndef GCCE"\
                        "PAGED" \
                        "$${LITERAL_HASH}endif"
    MMP_RULES += custom_paged_rule
}

symbian:MMP_RULES += "OPTION   GCCE -mlong-calls"

