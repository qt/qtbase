TEMPLATE      = subdirs
SUBDIRS       = analogclock \
                calculator \
                calendarwidget \
                charactermap \
                codeeditor \
                digitalclock \
                groupbox \
                icons \
                imageviewer \
                lineedits \
                movie \
                scribble \
                shapedclock \
                sliders \
                spinboxes \
                stylesheet \
                tablet \
                tetrix \
                tooltips \
                validators \
                wiggly \
                windowflags

symbian: SUBDIRS = \
                analogclock \
                calculator \
                calendarwidget \
                lineedits \
                shapedclock \
                tetrix \
                wiggly \
                softkeys

contains(styles, motif): SUBDIRS += styles

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS widgets.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
