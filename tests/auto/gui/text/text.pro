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
   qtextodfwriter \
   qinputcontrol

win32:SUBDIRS -= qtextpiecetable

qtConfig(textmarkdownreader): SUBDIRS += qtextmarkdownimporter
qtConfig(textmarkdownwriter): SUBDIRS += qtextmarkdownwriter

!qtConfig(private_tests): SUBDIRS -= \
           qfontcache \
           qcssparser \
           qtextlayout \
           qtextpiecetable \
           qzip \
           qtextmarkdownwriter \
           qtextodfwriter

!qtHaveModule(xml): SUBDIRS -= \
           qcssparser \
           qtextdocument
