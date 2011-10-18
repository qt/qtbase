TEMPLATE      = subdirs
SUBDIRS       = analogclock \
                applicationicon \
                calculator \
                calendarwidget \
                charactermap \
                codeeditor \
                digitalclock \
                elidedlabel \
                groupbox \
                icons \
                imageviewer \
                lineedits \
                movie \
                orientation \
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

contains(styles, motif): SUBDIRS += styles

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS widgets.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets
INSTALLS += target sources

QT += widgets
