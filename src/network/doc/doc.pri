qtPrepareTool(QDOC, qdoc)
docs.commands += $$QDOC $$QT.network.sources/doc/qtnetwork.qdocconf
QMAKE_EXTRA_TARGETS += docs
