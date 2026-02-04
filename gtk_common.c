#include "common.h"
#include "utils.h"


extern GtkWidget *g_app_status_statusbar, *g_app_progress_statusbar;


void
set_widgets_sensitive (int setting, int widget_count, ...)
{
  va_list widgets;
  va_start (widgets, widget_count);

  int widget_no;
  for (widget_no = 0; widget_no < widget_count; widget_no++)
    {
      gtk_widget_set_sensitive (va_arg (widgets, GtkWidget *), setting);
    }

  va_end (widgets);
}


GtkWindow *
app_get_parent_window (GtkWidget * widget)
{
  GdkWindow *parent_gdk_window = gtk_widget_get_parent_window (widget);
  GtkWindow *parent_gtk_window = NULL;
  gdk_window_get_user_data (parent_gdk_window,
			    (gpointer *) & parent_gtk_window);

  return parent_gtk_window;
}


void
print_to_status_statusbar (const char *format, ...)
{

  char *print_msg;
  va_list print_args;

  va_start (print_args, format);

  vasprintf (&print_msg, format, print_args);

  gtk_statusbar_push (GTK_STATUSBAR (g_app_status_statusbar),
		      gtk_statusbar_get_context_id (GTK_STATUSBAR
						    (g_app_status_statusbar),
						    print_msg), print_msg);

  va_end (print_args);

  free (print_msg);
}


void
print_to_progress_statusbar (const char *format, ...)
{
  char *print_msg;
  va_list print_args;

  va_start (print_args, format);

  vasprintf (&print_msg, format, print_args);

  gtk_statusbar_push (GTK_STATUSBAR (g_app_progress_statusbar),
		      gtk_statusbar_get_context_id (GTK_STATUSBAR
						    (g_app_progress_statusbar),
						    print_msg), print_msg);

  va_end (print_args);

  free (print_msg);
}


void
status_statusbar_clear ()
{
  print_to_status_statusbar ("");
}


void
progress_statusbar_clear ()
{
  print_to_progress_statusbar ("");
}


void
statusbars_clear ()
{
  print_to_progress_statusbar ("");
  print_to_status_statusbar ("");
}


void
refresh_gtk ()
{
  while (gtk_events_pending ())
    gtk_main_iteration ();
}


void
app_set_default_focus ()
{
  extern GtkWidget *g_name_combo_box_text, *g_app_find_button;

  gtk_widget_grab_focus (g_name_combo_box_text);
  gtk_widget_grab_default (g_app_find_button);
}


char *
get_content_type (char *filename)
{
  GFile *file = NULL;
  GFileInfo *fileinfo = NULL;
  GError *g_error = NULL;
  char *content_type = NULL, *filename_content_type = NULL;

  file = g_file_new_for_path (filename);

  fileinfo =
    g_file_query_info (file, "standard::content-type",
		       G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &g_error);

  if (g_error == NULL)
    {
      content_type =
	(char *) g_file_info_get_attribute_string (fileinfo,
						   G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
      filename_content_type = app_strdup (content_type);
    }

  if (fileinfo != NULL)
    g_object_unref (fileinfo);
  if (file != NULL)
    g_object_unref (file);
  if (g_error != NULL)
    g_error_free (g_error);

  return filename_content_type;
}


char *
get_file_icon (char *filename, char *filetype)
{
  GFile *file = NULL;
  GFileInfo *fileinfo = NULL;
  GIcon *file_icon;
  char *file_icon_string;
  int unref_file_icon = 0;

  file = g_file_new_for_path (filename);

  if ((fileinfo =
       g_file_query_info (file, "preview::icon",
			  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL,
			  NULL)) != NULL)
    {
      if ((file_icon =
	   G_ICON (g_file_info_get_attribute_object
		   (fileinfo, "preview::icon"))) != NULL)
	{
	  goto free_return;
	}
    }

  if (fileinfo != NULL)
    g_object_unref (fileinfo);


  if ((fileinfo =
       g_file_query_info (file, "thumbnail::path",
			  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL,
			  NULL)) != NULL)
    {
      char *file_icon_path;
      if ((file_icon_path =
	   (char *) g_file_info_get_attribute_byte_string (fileinfo,
							   "thumbnail::path"))
	  != NULL)
	{
	  if ((file_icon =
	       g_icon_new_for_string (file_icon_path, NULL)) != NULL)
	    {
	      unref_file_icon = 1;
	      goto free_return;
	    }
	}
    }

  if (fileinfo != NULL)
    g_object_unref (fileinfo);


  if ((fileinfo =
       g_file_query_info (file, "standard::icon,standard::symbolic-icon",
			  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL,
			  NULL)) != NULL)
    {
      if ((file_icon =
	   G_ICON (g_file_info_get_attribute_object
		   (fileinfo, "standard::icon"))) != NULL)
	{
	  goto free_return;
	}

      if ((file_icon =
	   G_ICON (g_file_info_get_attribute_object
		   (fileinfo, "standard::symbolic-icon"))) != NULL)
	{
	  goto free_return;
	}
    }


  file_icon = g_content_type_get_icon (filetype);
  unref_file_icon = 1;


free_return:
  file_icon_string = g_icon_to_string (file_icon);
  if (unref_file_icon)
    g_object_unref (file_icon);

  if (fileinfo != NULL)
    g_object_unref (fileinfo);
  if (file != NULL)
    g_object_unref (file);

  return file_icon_string;
}
