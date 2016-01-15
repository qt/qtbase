#!/usr/bin/python
#############################################################################
##
## Copyright (C) 2013 Canonical Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

from gi.repository import Gtk
from subprocess import Popen

window = Gtk.Window()

box = Gtk.VBox(False, 0)
window.add(box)

child = None
def on_button_clicked(button, socket):
    global child
    child = Popen(['./rasterwindow', str(socket.get_id())])

button = Gtk.Button("Press me to embed a Qt client")
box.pack_start(button, False, False, 0)

socket = Gtk.Socket()
socket.set_size_request(200, 200)
box.add(socket)

button.connect("clicked", on_button_clicked, socket)
window.connect("destroy", Gtk.main_quit)

def plugged_event(widget):
    print("A window was embedded!")

socket.connect("plug-added", plugged_event)

window.show_all()
Gtk.main()
if child:
    child.terminate()
