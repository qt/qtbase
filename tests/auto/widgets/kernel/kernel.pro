TEMPLATE=subdirs
SUBDIRS=\
   qapplication \
   qboxlayout \
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

darwin:SUBDIRS -= \
   qgesturerecognizer \

qtConfig(action):SUBDIRS += \
   qaction \
   qactiongroup \
   qwidgetaction

!qtConfig(shortcut): SUBDIRS -= \
   qshortcut
