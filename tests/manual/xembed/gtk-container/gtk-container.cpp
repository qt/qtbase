// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <gtk/gtk.h>
#include <gtk/gtkx.h>

gchar *cmd;

void launch_app(GtkButton */* btn */, guint32 *id)
{
    gchar *command = g_strdup_printf("%s %u", cmd, *id);
    g_spawn_command_line_async(command, NULL);
}

gint main(gint argc, gchar **argv)
{
    if (argc <=1 || argc > 2) {
        g_print("No client application defined.\n");
        return 0;
    }

    cmd = g_strdup_printf("%s", argv[1]);

    gtk_init(&argc, &argv);

    GtkWidget *win  = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *sock = gtk_socket_new();
    GtkWidget *btn  = gtk_button_new_with_label("Hello, World!");
    g_signal_connect(sock, "plug-removed", gtk_main_quit, NULL);
    g_signal_connect(win,  "delete-event", gtk_main_quit, NULL);
    gtk_widget_set_size_request(sock, 200, 200);
    gtk_box_pack_start(GTK_BOX(vbox), btn,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sock,  TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);
    gtk_widget_show_all(win);

    guint32 id = gtk_socket_get_id(GTK_SOCKET(sock));
    g_signal_connect(btn, "clicked", G_CALLBACK(launch_app), &id);

    gtk_main();
    return 0;
}
