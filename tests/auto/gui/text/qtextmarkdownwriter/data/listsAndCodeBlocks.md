- something happens in the debugger like this:

  ```
  1   QQuickEventPoint::setGrabberItem                 qquickevents.cpp     869 0x7ffff7a963f2 
  2   QQuickItem::grabMouse                            qquickitem.cpp      7599 0x7ffff7abea29 
  3   QQuickWindowPrivate::deliverMatchingPointsToItem qquickwindow.cpp    2738 0x7ffff7aea34c 
  4   QQuickWindowPrivate::deliverPressOrReleaseEvent  qquickwindow.cpp    2692 0x7ffff7ae9e57 
  5   QQuickWindowPrivate::deliverMouseEvent           qquickwindow.cpp    1911 0x7ffff7ae561b 
  6   QQuickWindowPrivate::deliverPointerEvent         qquickwindow.cpp    2454 0x7ffff7ae888c 
  7   QQuickWindowPrivate::handleMouseEvent            qquickwindow.cpp    2282 0x7ffff7ae7f1a 
  8   QQuickWindow::mousePressEvent                    qquickwindow.cpp    2249 0x7ffff7ae7bf5 
  9   QQuickView::mousePressEvent                      qquickview.cpp       626 0x7ffff7bd6bad 
  10  QWindow::event                                   qwindow.cpp         2258 0x7ffff70b2c54 
  ```
  and then I want to explain something about it.

- something I tried to fix it:

  ```c++
  item->ungrab();
  ```
- still didn't fix it, expecting a breakthrough any day now
- some sort of miracle
- profit!
