TEMPLATE=subdirs
SUBDIRS=\
   qaction \
   qactiongroup \
   qapplication \
   qboxlayout \
   qdesktopwidget \
   qformlayout \
   qgridlayout \
   qlayout \
   qstackedlayout \
   qtooltip \
   qwidget \
   qwidget_window \
   qwidgetaction \

SUBDIRS -= qsound

mac: qwidget.CONFIG += no_check_target # crashes, see QTBUG-23695
