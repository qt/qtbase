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
                mousebuttons \
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

contains(styles, windows): SUBDIRS += styles

# install
sources.files = widgets.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets
INSTALLS += sources
