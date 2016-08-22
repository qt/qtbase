TEMPLATE=subdirs
SUBDIRS=\
   qabstracttextdocumentlayout \
   qcssparser \
   qfont \
   qfontcache \
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
   qzip \
   qtextodfwriter

win32:SUBDIRS -= qtextpiecetable

!qtConfig(private_tests): SUBDIRS -= \
           qfontcache \
           qcssparser \
           qtextlayout \
           qtextpiecetable \
           qzip \
           qtextodfwriter
