TEMPLATE      = subdirs
SUBDIRS       = analogclock \
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
                styles \
                stylesheet \
                tablet \
                tetrix \
                tooltips \
                validators \
                wiggly \
                windowflags

!emscripten: SUBDIRS +=  \
               calculator \
