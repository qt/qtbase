TEMPLATE      = subdirs
SUBDIRS       = analogclock \
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
