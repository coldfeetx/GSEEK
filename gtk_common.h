// SPDX-License-Identifier: GPL-2.0-only
/*
 * gtk_common.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

void set_widgets_sensitive (int, int, ...);

GtkWindow *app_get_parent_window (GtkWidget *);

void print_to_status_statusbar (const char *, ...);

void print_to_progress_statusbar (const char *, ...);

void status_statusbar_clear (void);

void progress_statusbar_clear (void);

void statusbars_clear (void);

void refresh_gtk (void);

void app_set_default_focus (void);

char *get_content_type (char *);

char *get_file_icon (char *, char *);
