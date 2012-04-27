qtPrepareTool(QDOC, qdoc)
docs.commands += $$QDOC $$QT.sql.sources/doc/qtsql.qdocconf
QMAKE_EXTRA_TARGETS += docs
