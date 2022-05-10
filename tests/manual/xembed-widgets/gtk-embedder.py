#!/usr/bin/python
# Copyright (C) 2013 Canonical Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from gi.repository import Gtk
from subprocess import Popen

window = Gtk.Window()

box = Gtk.VBox(False, 0)
window.add(box)

child = None
def on_button_clicked(button, socket):
    global child
    child = Popen(['./lineedits', str(socket.get_id())])

button = Gtk.Button("Press me to embed a Qt client")
box.pack_start(button, False, False, 0)

entry = Gtk.Entry()
box.pack_start(entry, False, False, 0)

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
