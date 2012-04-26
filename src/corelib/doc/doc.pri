qtPrepareTool(QDOC, qdoc)
docs.commands += $$QDOC $$QT.core.sources/doc/qtcore.qdocconf
QMAKE_EXTRA_TARGETS += docs
