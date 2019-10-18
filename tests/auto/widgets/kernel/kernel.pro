TEMPLATE=subdirs
SUBDIRS=\
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
   qwidgetmetatype \
   qwidgetsvariant \
   qwindowcontainer \
   qshortcut \
   qsizepolicy

darwin:SUBDIRS -= \ # Uses native recognizers
   qgesturerecognizer \

!qtConfig(action):SUBDIRS -= \
   qaction \
   qactiongroup \
   qwidgetaction

!qtConfig(shortcut): SUBDIRS -= \
   qshortcut
