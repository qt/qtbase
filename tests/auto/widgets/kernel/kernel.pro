TEMPLATE=subdirs
SUBDIRS=\
   qaction \
   qactiongroup \
   qapplication \
   qboxlayout \
   qdesktopwidget \
   qformlayout \
   qgesturerecognizer \
   qgridlayout \
   qlayout \
   qstackedlayout \
   qtooltip \
   qwidget \
   qwidget_window \
   qwidgetaction \
   qwidgetmetatype \
   qwidgetsvariant \
   qwindowcontainer \
   qshortcut \
   qsizepolicy

darwin:SUBDIRS -= \ # Uses native recognizers
   qgesturerecognizer \
