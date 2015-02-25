#! [0]
# Comments usually start at the beginning of a line, but they
# can also follow other content on the same line.
#! [0]

#! [1]
# To include a literal hash character, use the $$LITERAL_HASH variable:
urlPieces = http://doc.qt.io/qt-5/qtextdocument.html pageCount
message($$join(urlPieces, $$LITERAL_HASH))
#! [1]
