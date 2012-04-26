qtPrepareTool(QDOC, qdoc)
docs.commands += $$QDOC $$QT.xml.sources/doc/qtxml.qdocconf
QMAKE_EXTRA_TARGETS += docs
