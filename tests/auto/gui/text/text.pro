TEMPLATE=subdirs
SUBDIRS=\
   qabstracttextdocumentlayout \
   qcssparser \
   qfont \
   qfontdatabase \
   qfontmetrics \
   qglyphrun \
   qrawfont \
   qstatictext \
   qsyntaxhighlighter \
   qtextblock \
   qtextcursor \
   qtextdocument \
   qtextdocumentfragment \
   qtextdocumentlayout \
   qtextformat \
   qtextlayout \
   qtextlist \
   qtextobject \
   qtextpiecetable \
   qtextscriptengine \
   qtexttable \

contains(QT_CONFIG, OdfWriter):SUBDIRS += qzip qtextodfwriter

win32:SUBDIRS -= qtextpiecetable

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qcssparser \
           qstatictext \
           qtextlayout \
           qtextpiecetable \

mac {
    qtextlayout.CONFIG = no_check_target # QTBUG-23050
}
