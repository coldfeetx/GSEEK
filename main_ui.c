// SPDX-License-Identifier: GPL-2.0-only
/*
 * main_ui.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "app_sideoptions.h"
#include "basic_page.h"
#include "content_page.h"
#include "properties_page.h"
#include "result_window.h"


GtkWidget *g_app_window;
GtkWidget *g_app_status_statusbar, *g_app_progress_statusbar;
GtkWidget *g_result_scrolled_window;
GtkWidget *g_appop_notebook;
FILE *g_app_history_fp;


void
app_conf_init ()
{
  uid_t app_uid = getuid ();
  struct passwd *pwd;
  errno = 0;
  if ((pwd = getpwuid (app_uid)) == NULL)
    errprint_exit ("Could not Fetch Password DataBase for User %jd: %s\n",
		   (intmax_t) app_uid, strerror (errno));


  char *home_dir = pwd->pw_dir;

  char *app_dir;
  asprintf (&app_dir, "%s/%s", home_dir, APP_DIR);

  errno = 0;
  if (mkdir (app_dir, S_IRWXU) != APP_SUCCESS && errno != EEXIST)
    errprint_exit ("Could not Access Directory \"%s\": %s\n", app_dir,
		   strerror (errno));


  char *app_history_file;
  asprintf (&app_history_file, "%s/%s", app_dir, APP_HISTORY_FILE);
  free (app_dir);

  int app_history_fd;
  errno = 0;
  if ((app_history_fd =
       open (app_history_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
    errprint_exit ("Could not Access File \"%s\": %s\n", app_history_file,
		   strerror (errno));


  errno = 0;
  if ((g_app_history_fp = fdopen (app_history_fd, "r+")) == NULL)
    errprint_exit ("Could not Retrieve File Stream for File \"%s\": %s\n",
		   app_history_file, strerror (errno));


  free (app_history_file);
}


void
app_init ()
{
  app_conf_init ();
}


void
gtk_settings_set_preferences ()
{
  GtkSettings *default_settings = gtk_settings_get_default ();
  g_object_set (default_settings, "gtk-button-images", TRUE, NULL);
  g_object_unref (default_settings);
}


void
setup_appop_ui (GtkWidget * appop_notebook)
{
  GtkWidget *basic_label = gtk_label_new_with_mnemonic ("_Basic");
  GtkWidget *basic_page_table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (basic_page_table), 10);

  GtkWidget *content_label = gtk_label_new_with_mnemonic ("Cont_ent");
  GtkWidget *content_page_table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (content_page_table), 10);

  GtkWidget *properties_label = gtk_label_new_with_mnemonic ("_Properties");
  GtkWidget *properties_page_table = gtk_table_new (5, 4, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (properties_page_table), 10);

  gtk_notebook_append_page (GTK_NOTEBOOK (appop_notebook), basic_page_table,
			    basic_label);
  gtk_notebook_append_page (GTK_NOTEBOOK (appop_notebook), content_page_table,
			    content_label);
  gtk_notebook_append_page (GTK_NOTEBOOK (appop_notebook),
			    properties_page_table, properties_label);


  gtk_notebook_set_scrollable (GTK_NOTEBOOK (appop_notebook), TRUE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (appop_notebook), GTK_POS_TOP);


  setup_basic_page (basic_page_table);
  setup_content_page (content_page_table);
  setup_properties_page (properties_page_table);
}


static void
setup_app_status_progress (GtkWidget * app_status_progress_hbox)
{
  g_app_progress_statusbar = gtk_statusbar_new ();
  gtk_widget_set_halign (g_app_progress_statusbar, GTK_ALIGN_START);

  g_app_status_statusbar = gtk_statusbar_new ();
  gtk_widget_set_halign (g_app_status_statusbar, GTK_ALIGN_START);

  gtk_box_pack_start (GTK_BOX (app_status_progress_hbox),
		      g_app_progress_statusbar, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (app_status_progress_hbox),
		      g_app_status_statusbar, TRUE, TRUE, 0);
}


static void
setup_app_main (GtkWidget * app_main_table)
{
  gtk_table_set_row_spacings (GTK_TABLE (app_main_table), 10);


  g_appop_notebook = gtk_notebook_new ();
  gtk_widget_set_valign (g_appop_notebook, GTK_ALIGN_START);
  setup_appop_ui (g_appop_notebook);


  g_result_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
				  (g_result_scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW
				       (g_result_scrolled_window),
				       GTK_SHADOW_IN);
  setup_result_window (g_result_scrolled_window);


  GtkWidget *app_status_progress_hbox =
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_set_homogeneous (GTK_BOX (app_status_progress_hbox), TRUE);
  setup_app_status_progress (app_status_progress_hbox);


  gtk_table_attach (GTK_TABLE (app_main_table), g_appop_notebook, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (app_main_table), g_result_scrolled_window, 0,
		    1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0,
		    0);
  gtk_table_attach (GTK_TABLE (app_main_table), app_status_progress_hbox, 0,
		    1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
}


static void
setup_app_core (GtkWidget * app_core_hbox)
{
  GtkWidget *app_main_table = gtk_table_new (3, 1, FALSE);
  setup_app_main (app_main_table);

  GtkWidget *app_sideoptions_table = gtk_table_new (7, 1, TRUE);
  setup_app_sideoptions (app_sideoptions_table);

  gtk_box_pack_start (GTK_BOX (app_core_hbox), app_main_table, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (app_core_hbox), app_sideoptions_table, FALSE,
		      FALSE, 0);
}



static void
app_quit (GtkWidget * window)
{
  gtk_main_quit ();

  exit (EXIT_SUCCESS);
}


static gboolean
app_key_pressed (GtkWidget * app_eventbox, GdkEventKey * app_key_event)
{
  guint app_key = app_key_event->keyval;

  switch (app_key)
    {
    case GDK_KEY_Escape:
      app_quit (g_app_window);
      break;
    }

  return FALSE;
}


static void
setup_app_eventbox (GtkWidget * app_eventbox)
{
  GtkWidget *app_core_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  setup_app_core (app_core_hbox);

  gtk_container_add (GTK_CONTAINER (app_eventbox), app_core_hbox);
  g_signal_connect (G_OBJECT (app_eventbox), "key-press-event",
		    G_CALLBACK (app_key_pressed), NULL);
}


void
app_parse_args (int argc, char **argv)
{
  if (argc > 1)
    errprint_exit ("Usage: %s\n"
		   "An Efficient and LightWeight File Search Utility\n",
		   argv[0]);
}


int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  app_init ();

  app_parse_args (argc, argv);

  gtk_settings_set_preferences ();

  g_app_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (g_app_window), APP_NAME);
  gtk_container_set_border_width (GTK_CONTAINER (g_app_window), 15);
  gtk_window_set_icon_from_file (GTK_WINDOW (g_app_window), WINDOW_ICON,
				 NULL);
  g_signal_connect (G_OBJECT (g_app_window), "destroy", G_CALLBACK (app_quit),
		    NULL);

  GtkWidget *app_eventbox = gtk_event_box_new ();
  gtk_event_box_set_above_child (GTK_EVENT_BOX (app_eventbox), FALSE);
  setup_app_eventbox (app_eventbox);

  gtk_container_add (GTK_CONTAINER (g_app_window), app_eventbox);

  gtk_widget_set_events (app_eventbox, GDK_KEY_PRESS_MASK);
  gtk_widget_realize (app_eventbox);

  gtk_widget_show_all (g_app_window);

  gtk_widget_set_size_request (g_result_scrolled_window,
			       gtk_widget_get_allocated_width
			       (g_appop_notebook),
			       gtk_widget_get_allocated_height
			       (g_appop_notebook));

  app_set_default_focus ();

  gtk_main ();

  return 0;
}
