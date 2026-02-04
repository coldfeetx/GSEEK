// SPDX-License-Identifier: GPL-2.0-only
/*
 * result_window.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "utils.h"
#include "nftw.h"
#include "gtk_common.h"
#include "result_db.h"
#include "regexp.h"
#include "filematches.h"
#include "parse_contents.h"
#include "content_search.h"


extern GtkWidget *g_app_window;
extern char *g_date_pattern_rev, *g_date_sort_pattern;
extern char *g_search_location;
extern filematches_t g_filematches_settings;

GtkWidget *g_result_treeview;
GtkListStore *g_result_store;
GtkTreeSortable *g_result_sortable;

int g_modified;

GtkWidget *g_filematches_menuitem, *g_filetrash_menuitem, *g_filedel_menuitem;


void
append_result_window (GtkListStore * result_store,
		      search_result_t * search_result,
		      GtkTreeSortable * result_sortable, int column_id,
		      int sort_order)
{
  char *file;
  asprintf (&file, "%s/%s%s", g_search_location,
	    search_result->file_subfolder, search_result->file_name);

  char *file_icon_string = get_file_icon (file, search_result->file_content);
  GIcon *file_icon = g_icon_new_for_string (file_icon_string, NULL);
  free (file_icon_string);
  free (file);

  GtkTreeIter result_iter;

  gtk_list_store_append (result_store, &result_iter);
  gtk_list_store_set (result_store, &result_iter, FILE_ICON, file_icon,
		      FILE_NAME, search_result->file_name, FILE_SUBFOLDER,
		      search_result->file_subfolder, FILE_SIZE,
		      search_result->file_size, FILE_MODIFIED,
		      search_result->file_modified, FILE_MEMBERSHIP,
		      search_result->file_membership, FILE_PERM,
		      search_result->file_perm, -1);

  gtk_tree_sortable_set_sort_column_id (result_sortable,
					GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					sort_order);

  g_object_unref (file_icon);
}


void
result_window_clear (GtkListStore * result_store)
{
  gtk_list_store_clear (result_store);
}


static int
get_sizeu (char size)
{
  switch (size)
    {
    case 'B':
      return 0;
    case 'K':
      return 1;
    case 'M':
      return 2;
    case 'G':
      return 3;
    case 'T':
      return 4;
    }
}


static int
get_perm (char perm)
{
  switch (perm)
    {
    case '-':
      return 0;
    case 'r':
      return 1;
    case 'w':
      return 2;
    case 'x':
      return 3;
    }
}


static int
get_timediff (char *filetimestr_a, char *filetimestr_b)
{
  struct tm tm_a, tm_b;
  strptime (filetimestr_a, g_date_pattern_rev, &tm_a);
  strptime (filetimestr_b, g_date_pattern_rev, &tm_b);

  char *filetime_str_a = get_tm_to_time (&tm_a, g_date_sort_pattern);
  char *filetime_str_b = get_tm_to_time (&tm_b, g_date_sort_pattern);

  int filetimestr_cmp = strcmp (filetime_str_a, filetime_str_b);
  app_free (filetime_str_a);
  app_free (filetime_str_b);

  return filetimestr_cmp;
}


static int
sort_filepathstr (char *filepathstr_a, char *filepathstr_b)
{
  char *filepath_a, *filepath_b;

  while (1)
    {
      filepath_b = strsep (&filepathstr_a, "/");
      filepath_a = strsep (&filepathstr_b, "/");

      if (filepath_a == NULL && filepath_b == NULL)
	return 0;
      else if (filepath_a == NULL)
	return 1;
      else if (filepath_b == NULL)
	return -1;
      else
	{
	  int cmp_result = strcmp (filepath_a, filepath_b);
	  if (cmp_result != 0)
	    return cmp_result;
	}
    }

  return 0;
}


static int
sort_filenames_default (GtkTreeModel * result_model, GtkTreeIter * fileiter_a,
			GtkTreeIter * fileiter_b, gpointer user_data)
{
  char *filenamestr_a, *filenamestr_b, *filepathstr_a, *filepathstr_b;

  gtk_tree_model_get (result_model, fileiter_a, FILE_NAME, &filenamestr_a,
		      FILE_SUBFOLDER, &filepathstr_a, -1);
  gtk_tree_model_get (result_model, fileiter_b, FILE_NAME, &filenamestr_b,
		      FILE_SUBFOLDER, &filepathstr_b, -1);


  int cmp_result = sort_filepathstr (filepathstr_a, filepathstr_b);
  if (cmp_result != 0)
    return -cmp_result;
  else
    return (strcasecmp (filenamestr_a, filenamestr_b));
}


static int
sort_filenames (GtkTreeModel * result_model, GtkTreeIter * fileiter_a,
		GtkTreeIter * fileiter_b, gpointer user_data)
{
  char *filenamestr_a, *filenamestr_b;

  gtk_tree_model_get (result_model, fileiter_a, FILE_NAME, &filenamestr_a,
		      -1);
  gtk_tree_model_get (result_model, fileiter_b, FILE_NAME, &filenamestr_b,
		      -1);

  return strcasecmp (filenamestr_a, filenamestr_b);
}


static int
sort_filepaths (GtkTreeModel * result_model, GtkTreeIter * fileiter_a,
		GtkTreeIter * fileiter_b, gpointer user_data)
{
  char *filepathstr_a, *filepathstr_b;

  gtk_tree_model_get (result_model, fileiter_a, FILE_SUBFOLDER,
		      &filepathstr_a, -1);
  gtk_tree_model_get (result_model, fileiter_b, FILE_SUBFOLDER,
		      &filepathstr_b, -1);

  return sort_filepathstr (filepathstr_a, filepathstr_b);
}


static int
sort_filesizes (GtkTreeModel * result_model, GtkTreeIter * fileiter_a,
		GtkTreeIter * fileiter_b, gpointer user_data)
{
  char *filesizestr_a, *filesizestr_b;

  gtk_tree_model_get (result_model, fileiter_a, FILE_SIZE, &filesizestr_a,
		      -1);
  gtk_tree_model_get (result_model, fileiter_b, FILE_SIZE, &filesizestr_b,
		      -1);

  int sizeu_a = get_sizeu (*(strrchr (filesizestr_a, ' ') + 1));
  int sizeu_b = get_sizeu (*(strrchr (filesizestr_b, ' ') + 1));

  if (sizeu_a != sizeu_b)
    {
      return sizeu_a - sizeu_b;
    }
  else
    {
      double sized_a = strtod (filesizestr_a, NULL);
      double sized_b = strtod (filesizestr_b, NULL);

      if (sized_a > sized_b)
	return 1;
      else if (sized_a < sized_b)
	return -1;
      else
	return 0;
    }

}


static int
sort_filetimes (GtkTreeModel * result_model, GtkTreeIter * fileiter_a,
		GtkTreeIter * fileiter_b, gpointer const_time_flag)
{
  char *filetimestr_a, *filetimestr_b;

  int time_flag = *(int *) const_time_flag;

  gtk_tree_model_get (result_model, fileiter_a, time_flag, &filetimestr_a,
		      -1);
  gtk_tree_model_get (result_model, fileiter_b, time_flag, &filetimestr_b,
		      -1);

  return get_timediff (filetimestr_a, filetimestr_b);
}


static int
sort_filememberships (GtkTreeModel * result_model, GtkTreeIter * fileiter_a,
		      GtkTreeIter * fileiter_b, gpointer user_data)
{
  char *filememsstr_a, *filememsstr_b;

  gtk_tree_model_get (result_model, fileiter_a, FILE_MEMBERSHIP,
		      &filememsstr_a, -1);
  gtk_tree_model_get (result_model, fileiter_b, FILE_MEMBERSHIP,
		      &filememsstr_b, -1);

  char filememstr_a[strlen (filememsstr_a) + 2], *filemem_a =
    filememstr_a, filememstr_b[strlen (filememsstr_b) + 2], *filemem_b =
    filememstr_b;
  sprintf (filememstr_a, "%s/", filememsstr_a);
  sprintf (filememstr_b, "%s/", filememsstr_b);

  int i = 0;
  char *member1, *member2;
  while (i++ < 2)
    {
      member1 = strsep (&filemem_a, "/");
      member2 = strsep (&filemem_b, "/");

      int cmp_result = strcmp (member1, member2);
      if (cmp_result != 0)
	return cmp_result;
    }

  return 0;
}


static int
sort_fileperms (GtkTreeModel * result_model, GtkTreeIter * fileiter_a,
		GtkTreeIter * fileiter_b, gpointer user_data)
{
  char *filepermstr_a, *filepermstr_b;

  gtk_tree_model_get (result_model, fileiter_a, FILE_PERM, &filepermstr_a,
		      -1);
  gtk_tree_model_get (result_model, fileiter_b, FILE_PERM, &filepermstr_b,
		      -1);

  while (*filepermstr_a)
    {
      int diff_perm = get_perm (*filepermstr_a) - get_perm (*filepermstr_b);
      if (diff_perm != 0)
	{
	  return diff_perm;
	}
      filepermstr_a++;
      filepermstr_b++;
    }

  return 0;
}


static void
launch_file_with_appinfo (char *file, GAppInfo * launch_appinfo)
{
  GList *file_list = NULL;
  GFile *gfile = g_file_new_for_path (file);
  file_list = g_list_append (file_list, gfile);

  GError *launch_error = NULL;
  if ((g_app_info_launch (launch_appinfo, file_list, NULL, &launch_error))
      != TRUE)
    {
      print_to_progress_statusbar
	("Application %s Failed to launch FILE:%s : %s",
	 g_app_info_get_display_name (launch_appinfo), file,
	 launch_error->message);
    }
  else if (launch_error != NULL)
    {
      print_to_progress_statusbar
	("Application %s Launch for FILE:%s Warning: %s",
	 g_app_info_get_display_name (launch_appinfo), file,
	 launch_error->message);
    }

  if (launch_error != NULL)
    g_clear_error (&launch_error);

  g_list_free_full (file_list, g_object_unref);
}


static void
launch_openwith_app (char *file, char *filename)
{
  GFile *gfile;
  if ((gfile = g_file_new_for_path (file)) == NULL)
    {
      print_to_progress_statusbar ("GIO Handle Retrieval Failed for File:%s!",
				   file);
      return;
    }

  GtkWidget *openwith_dialog =
    gtk_app_chooser_dialog_new (GTK_WINDOW (g_app_window),
				GTK_DIALOG_MODAL |
				GTK_DIALOG_DESTROY_WITH_PARENT, gfile);

  char *openwith_heading;
  asprintf (&openwith_heading, "Open File \"%s\" With", filename);

  gtk_app_chooser_dialog_set_heading (GTK_APP_CHOOSER_DIALOG
				      (openwith_dialog), openwith_heading);
  free (openwith_heading);

  GtkWidget *openwith_chooser =
    gtk_app_chooser_dialog_get_widget (GTK_APP_CHOOSER_DIALOG
				       (openwith_dialog));

  gtk_app_chooser_widget_set_show_default (GTK_APP_CHOOSER_WIDGET
					   (openwith_chooser), TRUE);
  gtk_app_chooser_widget_set_show_recommended (GTK_APP_CHOOSER_WIDGET
					       (openwith_chooser), TRUE);

//  set_widgets_sensitive (FALSE, 1, g_app_window);
  if (gtk_dialog_run (GTK_DIALOG (openwith_dialog)) == GTK_RESPONSE_OK)
    {
      GAppInfo *launch_appinfo =
	gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (openwith_chooser));
      launch_file_with_appinfo (file, launch_appinfo);
    }

  gtk_widget_destroy (openwith_dialog);
//  set_widgets_sensitive (TRUE, 1, g_app_window);
}


static GtkWidget *
setup_matched_window (GtkWidget * matches_scrolled_window)
{
  GtkWidget *matches_textview = gtk_text_view_new ();

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (matches_textview),
			       GTK_WRAP_WORD);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (matches_textview), FALSE);

  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (matches_textview), FALSE);


  gtk_container_add (GTK_CONTAINER (matches_scrolled_window),
		     matches_textview);

  return matches_textview;
}


static gboolean
matches_key_pressed (GtkWidget * matches_eventbox,
		     GdkEventKey * matches_key_event,
		     GtkWidget * matches_window)
{
  guint matches_key = matches_key_event->keyval;

  switch (matches_key)
    {
    case GDK_KEY_Escape:
      gtk_widget_destroy (matches_window);
      break;
    }

  return FALSE;
}


static GtkWidget *
setup_matches_eventbox (GtkWidget * matches_window,
			GtkWidget * matches_eventbox)
{
  GtkWidget *matches_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
				  (matches_scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW
				       (matches_scrolled_window),
				       GTK_SHADOW_IN);

  GtkWidget *matches_textview =
    setup_matched_window (matches_scrolled_window);

  gtk_container_add (GTK_CONTAINER (matches_eventbox),
		     matches_scrolled_window);
  g_signal_connect (G_OBJECT (matches_eventbox), "key-press-event",
		    G_CALLBACK (matches_key_pressed), matches_window);

  return matches_textview;
}


static GtkWidget *
setup_matches_window (GtkWidget * matches_window, char *filename)
{
  gtk_window_set_position (GTK_WINDOW (matches_window), GTK_WIN_POS_CENTER);
  gtk_widget_set_size_request (matches_window,
			       gtk_widget_get_allocated_width (g_app_window) *
			       0.75,
			       gtk_widget_get_allocated_height (g_app_window)
			       * 0.75);

  char *matches_window_title;
  asprintf (&matches_window_title, "File \"%s\" Matches", filename);
  gtk_window_set_title (GTK_WINDOW (matches_window), matches_window_title);
  free (matches_window_title);

  gtk_container_set_border_width (GTK_CONTAINER (matches_window), 10);

  g_signal_connect (G_OBJECT (matches_window), "destroy",
		    G_CALLBACK (gtk_widget_destroy), NULL);

  GtkWidget *matches_eventbox = gtk_event_box_new ();
  gtk_event_box_set_above_child (GTK_EVENT_BOX (matches_eventbox), FALSE);

  GtkWidget *matches_textview =
    setup_matches_eventbox (matches_window, matches_eventbox);

  gtk_container_add (GTK_CONTAINER (matches_window), matches_eventbox);

  gtk_widget_set_events (matches_eventbox, GDK_KEY_PRESS_MASK);
  gtk_widget_realize (matches_eventbox);

  return matches_textview;
}


void
show_matches (char *file, char *filename, char *subfolder, char *size,
	      char *modified)
{
  search_stream_obj *stream_obj = search_stream_init (file);
  if (strlen (stream_obj->stream_error) > 0)
    {
      print_to_progress_statusbar (stream_obj->stream_error);
      return;
    }

  GtkWidget *matches_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *matches_textview =
    setup_matches_window (matches_window, filename);


  GtkTextBuffer *matches_textbuffer =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (matches_textview));

  gtk_text_buffer_create_tag (matches_textbuffer, "italic", "style",
			      PANGO_STYLE_ITALIC, NULL);

  gtk_text_buffer_create_tag (matches_textbuffer, "normal", "style",
			      PANGO_STYLE_NORMAL, NULL);

  gtk_text_buffer_create_tag (matches_textbuffer, "bold", "weight",
			      PANGO_WEIGHT_BOLD, NULL);

  gtk_text_buffer_create_tag (matches_textbuffer, "blue_fg", "foreground",
			      "blue", NULL);

  gtk_text_buffer_create_tag (matches_textbuffer, "red_fg", "foreground",
			      "red", NULL);

  gtk_text_buffer_create_tag (matches_textbuffer, "green_fg", "foreground",
			      "green", NULL);

  GtkTextIter matches_textiter;
  gtk_text_buffer_get_iter_at_offset (matches_textbuffer, &matches_textiter,
				      0);
  char *matches_textheader_val;

  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    "FILE: ", -1, "bold", NULL);

  asprintf (&matches_textheader_val, "%s\n", filename);
  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    matches_textheader_val, -1,
					    "blue_fg", "italic", "bold",
					    NULL);
  free (matches_textheader_val);

  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    "PATH: ", -1, "bold", NULL);

  asprintf (&matches_textheader_val, "%s/%s\n", g_search_location, subfolder);
  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    matches_textheader_val, -1,
					    "blue_fg", "italic", "bold",
					    NULL);
  free (matches_textheader_val);

  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    "TYPE: ", -1, "bold", NULL);

  char *content_type = get_content_type (file);
  asprintf (&matches_textheader_val, "%s\n",
	    content_table_get_description (content_type));
  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    matches_textheader_val, -1,
					    "blue_fg", "italic", "bold",
					    NULL);
  app_free (content_type);
  free (matches_textheader_val);

  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    "SIZE: ", -1, "bold", NULL);

  asprintf (&matches_textheader_val, "%s\n", size);
  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    matches_textheader_val, -1,
					    "blue_fg", "italic", "bold",
					    NULL);
  free (matches_textheader_val);

  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    "MODIFIED: ", -1, "bold", NULL);

  asprintf (&matches_textheader_val, "%s\n", modified);
  gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
					    &matches_textiter,
					    matches_textheader_val, -1,
					    "blue_fg", "italic", "bold",
					    NULL);
  free (matches_textheader_val);


  char *(*matches_strstr) (const char *, const char *);
  if (!g_filematches_settings.cregexp)
    {
      if (g_filematches_settings.ccasei)
	matches_strstr = strstr;
      else
	matches_strstr = strcasestr;
    }


  int matches_linestart, line_matched;
  unsigned long matches_textline_no = 1, matches_matches = 0;
  char *filebuf, *fileptr;
  while ((filebuf = search_stream_get_content_next (stream_obj)) != NULL)
    {
      line_matched = 0;
      matches_linestart = 1;

      fileptr = filebuf;

      size_t match_start, match_end;
      int matched;

      do
	{
	  matched = 0;

	  if (g_filematches_settings.cregexp)
	    {
	      regexp_list_t *regexp_matches_list =
		regexp_exec (&g_filematches_settings.cregexp_preg, fileptr, 0,
			     REGEXP_FAST);
	      if (regexp_matches_list->regexp_error == APP_SUCCESS)
		{
		  line_matched = matched = 1;
		  regexp_node_t *regexp_matches_node =
		    regexp_matches_list->regexp_nodes;

		  match_start = regexp_matches_node[0].regexp_start_index;
		  match_end = regexp_matches_node[0].regexp_end_index;

		  regexp_free_all (regexp_matches_list);
		}
	    }
	  else
	    {
	      char *match_ptr =
		matches_strstr (fileptr, g_filematches_settings.content_text);
	      if (match_ptr != NULL)
		{
		  line_matched = matched = 1;
		  match_start = match_ptr - fileptr;
		  match_end =
		    match_ptr - fileptr +
		    strlen (g_filematches_settings.content_text) - 1;
		}
	    }

	  if (matched)
	    {
	      if (matches_linestart)
		{
		  char *matches_textline =
		    search_stream_get_pos_formatted (stream_obj);

		  gtk_text_buffer_insert_with_tags_by_name
		    (matches_textbuffer, &matches_textiter, matches_textline,
		     -1, "italic", NULL);

		  free (matches_textline);
		  matches_linestart = 0;
		}

	      if (match_start > 0)
		{
		  char *line_plain = strndup (fileptr, match_start);
		  asprintf (&line_plain, "%.*s", (int) match_start, fileptr);

		  gtk_text_buffer_insert (matches_textbuffer,
					  &matches_textiter, line_plain, -1);

		  free (line_plain);
		}

	      char *line_match;
	      asprintf (&line_match, "%.*s",
			(int) (match_end - match_start + 1),
			fileptr + match_start);

	      gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
							&matches_textiter,
							line_match, -1,
							"green_fg", NULL);

	      free (line_match);

	      fileptr += match_end + 1;
	    }
	  else if (line_matched && strlen (fileptr) > 0)
	    {
	      char *line_end;
	      asprintf (&line_end, "%s", fileptr);

	      gtk_text_buffer_insert (matches_textbuffer,
				      &matches_textiter, line_end, -1);

	      free (line_end);
	    }

	}
      while (matched && strlen (fileptr) > 0);

      if (line_matched)
	matches_matches += 1;

      matches_textline_no += 1;
    }

  search_stream_cleanup (stream_obj);

  if (matches_textline_no > 1)
    {
      gtk_text_buffer_get_iter_at_line (matches_textbuffer,
					&matches_textiter, 5);

      gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
						&matches_textiter,
						"MATCHES: ", -1, "normal",
						"bold", NULL);

      asprintf (&matches_textheader_val, "%lu", matches_matches);
      gtk_text_buffer_insert_with_tags_by_name (matches_textbuffer,
						&matches_textiter,
						matches_textheader_val, -1,
						"red_fg", "italic", "bold",
						NULL);
      free (matches_textheader_val);


      gtk_widget_show_all (matches_window);
    }
  else
    gtk_widget_destroy (matches_window);
}


static void
launch_app (char *file, char *filename)
{
  char *filetype;
  if ((filetype = get_content_type (file)) == NULL)
    {
      print_to_progress_statusbar
	("File Content Retrieval Failed for File:%s!", file);
      return;
    }

  if (executable (file, filetype) == APP_SUCCESS)
    {
      char *file_cmd;
      asprintf (&file_cmd, "'%s'", file);

      int file_cmd_spawn = g_spawn_command_line_async (file_cmd, NULL);
      free (file_cmd);

      if (file_cmd_spawn == TRUE)
	goto free_return;
    }

  GList *file_list = NULL;
  GFile *gfile = g_file_new_for_path (file);
  file_list = g_list_append (file_list, gfile);

  GAppInfo *launch_appinfo;
  if ((launch_appinfo =
       g_app_info_get_default_for_type (filetype, FALSE)) == NULL)
    {
      print_to_progress_statusbar
	("AppInfo Retrieval Failed for File:%s with content:%s!", file,
	 filetype);
      launch_openwith_app (file, filename);
      goto freeall_return;
    }

  GError *launch_error = NULL;
  if ((g_app_info_launch (launch_appinfo, file_list, NULL, &launch_error))
      != TRUE)
    {
      print_to_progress_statusbar
	("Application %s Failed to launch FILE:%s : %s",
	 g_app_info_get_display_name (launch_appinfo), file,
	 launch_error->message);
      launch_openwith_app (file, filename);
    }
  else if (launch_error != NULL)
    {
      print_to_progress_statusbar
	("Application %s Launch for FILE:%s Warning: %s",
	 g_app_info_get_display_name (launch_appinfo), file,
	 launch_error->message);
    }

  if (launch_error != NULL)
    g_clear_error (&launch_error);


freeall_return:
  g_list_free_full (file_list, g_object_unref);
free_return:
  app_free (filetype);
}


static void
launch_file_in_app (GtkTreeView * result_treeview, GtkTreePath * result_path,
		    GtkTreeViewColumn * result_column, gpointer data)
{
  progress_statusbar_clear ();

  GtkTreeModel *result_model;
  GtkTreeIter result_iter;

  result_model = gtk_tree_view_get_model (result_treeview);
  if (gtk_tree_model_get_iter (result_model, &result_iter, result_path))
    {
      char *filename, *subfolder;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, -1);

      char *file;
      asprintf (&file, "%s/%s%s", g_search_location, subfolder, filename);
      launch_app (file, filename);

      free (file);
    }
  else
    {
      print_to_progress_statusbar
	("File and Folder Tree Attributes could not be retrieved!");
    }
}


static void
launch_app_with (char *file, char *filename, char *filetype)
{

  GList *file_all_appinfo_list;
  if ((file_all_appinfo_list =
       g_app_info_get_all_for_type (filetype)) == NULL)
    return launch_openwith_app (file, filename);

  GList *file_list = NULL;
  GFile *gfile = g_file_new_for_path (file);
  file_list = g_list_append (file_list, gfile);

  int launch_failed = 1;
  GAppInfo *launch_appinfo;
  for (launch_appinfo = G_APP_INFO (file_all_appinfo_list->data);
       launch_appinfo;
       file_all_appinfo_list = g_list_next (file_all_appinfo_list))
    {
      GError *launch_error = NULL;
      if ((g_app_info_launch (launch_appinfo, file_list, NULL, &launch_error))
	  != TRUE)
	{
	  print_to_progress_statusbar
	    ("Application %s Failed to launch FILE:%s : %s",
	     g_app_info_get_display_name (launch_appinfo), file,
	     launch_error->message);
	}
      else
	{
	  if (launch_error != NULL)
	    {
	      print_to_progress_statusbar
		("Application %s Launch for FILE:%s Warning: %s",
		 g_app_info_get_display_name (launch_appinfo), file,
		 launch_error->message);

	      g_clear_error (&launch_error);
	    }

	  launch_failed = 0;
	  break;
	}

      if (launch_error != NULL)
	g_clear_error (&launch_error);

    }

  if (launch_failed)
    launch_openwith_app (file, filename);

freeall_return:
  g_list_free_full (file_list, g_object_unref);
}


void
remove_directory_from_tree (char *parent_file, GtkTreeModel * result_model,
			    GtkTreeView * result_treestore,
			    GtkListStore * result_store,
			    GtkTreeIter * parent_iter_ptr)
{
  gtk_list_store_remove (result_store, parent_iter_ptr);
  GtkTreeIter child_iter;
  gtk_tree_model_get_iter_from_string (result_model, &child_iter, "0");

  int matched;
  do
    {
      if (gtk_list_store_iter_is_valid (result_store, &child_iter) != TRUE)
	return;

      matched = 0;

      char *subfolder;
      gtk_tree_model_get (result_model, &child_iter, FILE_SUBFOLDER,
			  &subfolder, -1);

      char *path;
      asprintf (&path, "%s/%s", g_search_location, subfolder);

      if (!strncmp (path, parent_file, strlen (parent_file)))
	{
	  gtk_list_store_remove (result_store, &child_iter);
	  matched = 1;
	}

      free (path);
    }
  while (matched || gtk_tree_model_iter_next (result_model, &child_iter));
}


static void
move_file_to_trash (char *file, GtkTreeModel * result_model,
		    GtkWidget * result_treeview, GtkTreeIter * result_iter)
{

  GtkWidget *file_trash_question_dialog =
    gtk_message_dialog_new (GTK_WINDOW (g_app_window),
			    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			    GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
			    "Are you sure to Move \"%s\" to Trash?",
			    file);

  gtk_dialog_set_default_response (GTK_DIALOG
				   (file_trash_question_dialog),
				   GTK_RESPONSE_CANCEL);

  int trash_file_result =
    gtk_dialog_run (GTK_DIALOG (file_trash_question_dialog));
  gtk_widget_destroy (file_trash_question_dialog);
  if (trash_file_result == GTK_RESPONSE_YES)
    {
      char *filetype;

      GFile *file_trash;
      if ((file_trash = g_file_new_for_path (file)) != NULL)
	{
	  filetype = get_content_type (file);

	  GError *file_trash_error = NULL;
	  if (g_file_trash (file_trash, NULL, &file_trash_error) != TRUE)
	    {
	      print_to_progress_statusbar
		("File %s could not be Moved to Trash!: %s", file_trash,
		 file_trash_error->message);
	    }
	  else if (filetype != NULL && !strcmp (filetype, "inode/directory"))
	    {
	      char *filed;
	      asprintf (&filed, "%s/", file);

	      remove_directory_from_tree (filed, result_model,
					  GTK_TREE_VIEW
					  (result_treeview),
					  GTK_LIST_STORE
					  (result_model), result_iter);

	      free (filed);
	    }
	  else
	    {
	      gtk_list_store_remove (GTK_LIST_STORE (result_model),
				     result_iter);
	    }

	  if (filetype != NULL)
	    free (filetype);

	  if (file_trash_error != NULL)
	    g_clear_error (&file_trash_error);
	  g_object_unref (file_trash);
	}

    }

}


int
Remove_Directory (const char *pathname, const struct stat *sbuf, int type,
		  struct FTW *ftwbuf)
{
  errno = 0;
  if (remove (pathname) != APP_SUCCESS)
    {
      print_to_progress_statusbar ("Could not Remove %s!: %s", pathname,
				   strerror (errno));
    }

  return 0;
}


static void
delete_file (char *file, GtkTreeModel * result_model,
	     GtkWidget * result_treeview, GtkTreeIter * result_iter)
{

  GtkWidget *file_delete_question_dialog =
    gtk_message_dialog_new (GTK_WINDOW (g_app_window),
			    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			    GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
			    "Are you sure you want to PERMANENTLY DELETE \"%s\"?",
			    file);

  gtk_dialog_set_default_response (GTK_DIALOG
				   (file_delete_question_dialog),
				   GTK_RESPONSE_CANCEL);

  int delete_file_result =
    gtk_dialog_run (GTK_DIALOG (file_delete_question_dialog));
  gtk_widget_destroy (file_delete_question_dialog);
  if (delete_file_result == GTK_RESPONSE_YES)
    {
      char *filetype;
      if ((filetype = get_content_type (file)) != NULL
	  && !strcmp (filetype, "inode/directory"))
	{
	  if (nftw
	      ((const char *) file, Remove_Directory, 100,
	       FTW_DEPTH) != APP_SUCCESS)
	    {
	      print_to_progress_statusbar
		("Error in Removing Directory %s!", file);
	    }
	  else
	    {
	      char *filed;
	      asprintf (&filed, "%s/", file);

	      remove_directory_from_tree (filed, result_model,
					  GTK_TREE_VIEW
					  (result_treeview),
					  GTK_LIST_STORE
					  (result_model), result_iter);

	      free (filed);
	    }
	}
      else
	{
	  errno = 0;
	  if (unlink (file) != APP_SUCCESS)
	    {
	      print_to_progress_statusbar
		("Failed to Remove File %s: %s!", file, strerror (errno));
	    }
	  else
	    {
	      gtk_list_store_remove (GTK_LIST_STORE (result_model),
				     result_iter);
	    }
	}

      if (filetype != NULL)
	free (filetype);
    }

}


static void
file_showmatches (GtkWidget * file_menu, GtkWidget * result_treeview)
{
  GtkTreeSelection *result_selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (result_treeview));

  GtkTreeModel *result_model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (result_treeview));

  GtkTreeIter result_iter;
  if (gtk_tree_selection_get_selected
      (result_selection, &result_model, &result_iter))
    {
      char *filename, *subfolder, *size, *modified;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, FILE_SIZE, &size,
			  FILE_MODIFIED, &modified, -1);
      char *file;
      asprintf (&file, "%s/%s%s", g_search_location, subfolder, filename);

      show_matches (file, filename, subfolder, size, modified);

      free (file);
    }

}


static void
file_open (GtkWidget * file_menu, GtkWidget * result_treeview)
{
  GtkTreeSelection *result_selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (result_treeview));

  GtkTreeModel *result_model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (result_treeview));

  GtkTreeIter result_iter;
  if (gtk_tree_selection_get_selected
      (result_selection, &result_model, &result_iter))
    {
      char *filename, *subfolder;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, -1);
      char *file;
      asprintf (&file, "%s/%s%s", g_search_location, subfolder, filename);

      launch_app (file, filename);

      free (file);
    }

}


static void
file_openwith (GtkWidget * file_menu, GtkWidget * result_treeview)
{
  progress_statusbar_clear ();

  GtkTreeSelection *result_selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (result_treeview));

  GtkTreeModel *result_model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (result_treeview));

  GtkTreeIter result_iter;
  if (gtk_tree_selection_get_selected
      (result_selection, &result_model, &result_iter))
    {
      char *filename, *subfolder;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, -1);
      char *file;
      asprintf (&file, "%s/%s%s", g_search_location, subfolder, filename);

      launch_openwith_app (file, filename);

      free (file);
    }

}


static void
file_opencontaining (GtkWidget * file_menu, GtkWidget * result_treeview)
{
  progress_statusbar_clear ();

  GtkTreeSelection *result_selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (result_treeview));

  GtkTreeModel *result_model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (result_treeview));

  GtkTreeIter result_iter;
  if (gtk_tree_selection_get_selected
      (result_selection, &result_model, &result_iter))
    {
      char *filename, *subfolder;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, -1);
      char *file;
      asprintf (&file, "%s/%s", g_search_location, subfolder);

      launch_app_with (file, filename, "inode/directory");

      free (file);
    }

}


static void
file_movetotrash (GtkWidget * file_menu, GtkWidget * result_treeview)
{
  progress_statusbar_clear ();

  GtkTreeSelection *result_selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (result_treeview));

  GtkTreeModel *result_model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (result_treeview));

  GtkTreeIter result_iter;
  if (gtk_tree_selection_get_selected
      (result_selection, &result_model, &result_iter))
    {
      char *filename, *subfolder;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, -1);
      char *file;
      asprintf (&file, "%s/%s%s", g_search_location, subfolder, filename);

      move_file_to_trash (file, result_model, result_treeview, &result_iter);

      free (file);
    }

}


static void
file_delete (GtkWidget * file_menu, GtkWidget * result_treeview)
{
  progress_statusbar_clear ();

  GtkTreeSelection *result_selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (result_treeview));

  GtkTreeModel *result_model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (result_treeview));

  GtkTreeIter result_iter;
  if (gtk_tree_selection_get_selected
      (result_selection, &result_model, &result_iter))
    {
      char *filename, *subfolder;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, -1);
      char *file;
      asprintf (&file, "%s/%s%s", g_search_location, subfolder, filename);

      delete_file (file, result_model, result_treeview, &result_iter);

      free (file);
    }

}


static void
file_showmatches_toggle_setting (GtkTreeView * result_treeview,
				 GtkTreeSelection * result_selection)
{
  int file_showmatches_setting = FALSE;

  GtkTreeModel *result_model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (result_treeview));

  GtkTreeIter result_iter;
  if (gtk_tree_selection_get_selected
      (result_selection, &result_model, &result_iter))
    {
      char *filename, *subfolder;
      gtk_tree_model_get (result_model, &result_iter, FILE_NAME, &filename,
			  FILE_SUBFOLDER, &subfolder, -1);
      char *file;
      asprintf (&file, "%s/%s%s", g_search_location, subfolder, filename);

      if (g_filematches_settings.content)
	{
	  char *content_type = get_content_type (file);

	  int content_is_searchable =
	    content_desc_searchable (content_table_get_description
				     (content_type));
	  app_free (content_type);

	  if (content_is_searchable == APP_SUCCESS)
	    file_showmatches_setting = TRUE;
	}

      free (file);
    }

  set_widgets_sensitive (file_showmatches_setting, 1, g_filematches_menuitem);
}


static void
popup_file_menu (GtkWidget * file_menu, GdkEventButton * event,
		 GtkTreeView * result_treeview,
		 GtkTreeSelection * result_selection)
{
  file_showmatches_toggle_setting (result_treeview, result_selection);

  gtk_menu_popup (GTK_MENU (file_menu), NULL, NULL, NULL, NULL,
		  (event != NULL) ? event->button : 0,
		  gdk_event_get_time ((GdkEvent *) event));
}


static gboolean
file_menu_on_button (GtkWidget * result_treeview, GdkEventButton * event,
		     GtkWidget * file_menu)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      GtkTreeSelection *result_selection =
	gtk_tree_view_get_selection (GTK_TREE_VIEW (result_treeview));
      if (gtk_tree_selection_count_selected_rows (result_selection) <= 1)
	{
	  GtkTreePath *result_path;
	  if (gtk_tree_view_get_path_at_pos
	      (GTK_TREE_VIEW (result_treeview), (gint) event->x,
	       (gint) event->y, &result_path, NULL, NULL, NULL))
	    {
	      gtk_tree_selection_unselect_all (result_selection);
	      gtk_tree_selection_select_path (result_selection, result_path);
	      gtk_tree_path_free (result_path);
	    }
	}

      popup_file_menu (file_menu, event, GTK_TREE_VIEW (result_treeview),
		       result_selection);

      return TRUE;
    }

  return FALSE;
}


static gboolean
file_menu_on_popup (GtkWidget * result_treeview, GtkWidget * file_menu)
{
  popup_file_menu (file_menu, NULL, GTK_TREE_VIEW (g_result_treeview),
		   gtk_tree_view_get_selection (GTK_TREE_VIEW
						(g_result_treeview)));

  return TRUE;
}


static GtkWidget *
create_file_menu (GtkWidget * result_treeview)
{
  GtkWidget *file_menu = gtk_menu_new ();

  GtkAccelGroup *file_accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (g_app_window), file_accel_group);
  gtk_menu_set_accel_group (GTK_MENU (file_menu), file_accel_group);


  g_filematches_menuitem = gtk_menu_item_new_with_label ("Show Matches");
  gtk_widget_add_accelerator (g_filematches_menuitem, "activate",
			      file_accel_group, GDK_KEY_M, GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE);
  g_signal_connect (G_OBJECT (g_filematches_menuitem), "activate",
		    G_CALLBACK (file_showmatches), result_treeview);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), g_filematches_menuitem);

  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu),
			 gtk_separator_menu_item_new ());

  GtkWidget *fileopen_menuitem = gtk_menu_item_new_with_label ("Open");
  gtk_widget_add_accelerator (fileopen_menuitem, "activate",
			      file_accel_group, GDK_KEY_O, GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE);
  g_signal_connect (G_OBJECT (fileopen_menuitem), "activate",
		    G_CALLBACK (file_open), result_treeview);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), fileopen_menuitem);

  GtkWidget *fileopenwith_menuitem =
    gtk_menu_item_new_with_label ("Open With");
  gtk_widget_add_accelerator (fileopenwith_menuitem, "activate",
			      file_accel_group, GDK_KEY_W, GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE);
  g_signal_connect (G_OBJECT (fileopenwith_menuitem), "activate",
		    G_CALLBACK (file_openwith), result_treeview);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), fileopenwith_menuitem);

  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu),
			 gtk_separator_menu_item_new ());

  GtkWidget *fileopencont_menuitem =
    gtk_menu_item_new_with_label ("Open Containing Folder");
  gtk_widget_add_accelerator (fileopencont_menuitem, "activate",
			      file_accel_group, GDK_KEY_F, GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE);
  g_signal_connect (G_OBJECT (fileopencont_menuitem), "activate",
		    G_CALLBACK (file_opencontaining), result_treeview);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), fileopencont_menuitem);

  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu),
			 gtk_separator_menu_item_new ());

  g_filetrash_menuitem = gtk_menu_item_new_with_label ("Move to Trash");
  gtk_widget_add_accelerator (g_filetrash_menuitem, "activate",
			      file_accel_group, GDK_KEY_Delete, 0,
			      GTK_ACCEL_VISIBLE);
  g_signal_connect (G_OBJECT (g_filetrash_menuitem), "activate",
		    G_CALLBACK (file_movetotrash), result_treeview);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), g_filetrash_menuitem);

  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu),
			 gtk_separator_menu_item_new ());

  g_filedel_menuitem = gtk_menu_item_new_with_label ("Delete");
  gtk_widget_add_accelerator (g_filedel_menuitem, "activate",
			      file_accel_group, GDK_KEY_Delete,
			      GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
  g_signal_connect (G_OBJECT (g_filedel_menuitem), "activate",
		    G_CALLBACK (file_delete), result_treeview);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), g_filedel_menuitem);


  gtk_widget_show_all (file_menu);

  return file_menu;
}


static void
setup_result_treeview (GtkWidget * result_treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, "File");
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer, "gicon", FILE_ICON,
				       NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer, "text", FILE_NAME,
				       NULL);
  gtk_tree_view_column_set_sort_column_id (column, FILE_NAME);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (result_treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes ("Path", renderer, "text",
					      FILE_SUBFOLDER, NULL);
  gtk_tree_view_column_set_sort_column_id (column, FILE_SUBFOLDER);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (result_treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes ("Size", renderer, "text",
					      FILE_SIZE, NULL);
  gtk_tree_view_column_set_sort_column_id (column, FILE_SIZE);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (result_treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes ("Modified", renderer, "text",
					      FILE_MODIFIED, NULL);
  gtk_tree_view_column_set_sort_column_id (column, FILE_MODIFIED);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (result_treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes ("Membership", renderer,
					      "text", FILE_MEMBERSHIP, NULL);
  gtk_tree_view_column_set_sort_column_id (column, FILE_MEMBERSHIP);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (result_treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes ("Permissions", renderer,
					      "text", FILE_PERM, NULL);
  gtk_tree_view_column_set_sort_column_id (column, FILE_PERM);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (result_treeview), column);

  GtkWidget *file_menu = create_file_menu (result_treeview);
  g_signal_connect (G_OBJECT (result_treeview), "row-activated",
		    G_CALLBACK (launch_file_in_app), NULL);
  g_signal_connect (G_OBJECT (result_treeview), "button-press-event",
		    G_CALLBACK (file_menu_on_button), file_menu);
  g_signal_connect (G_OBJECT (result_treeview), "popup-menu",
		    G_CALLBACK (file_menu_on_popup), file_menu);
}


void
setup_result_window (GtkWidget * result_scrolled_window)
{
  g_result_treeview = gtk_tree_view_new ();
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (g_result_treeview), TRUE);
  setup_result_treeview (g_result_treeview);

  g_result_store =
    gtk_list_store_new (COLUMNS, G_TYPE_ICON, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING);

  g_result_sortable = GTK_TREE_SORTABLE (g_result_store);


  gtk_tree_sortable_set_sort_func (g_result_sortable, FILE_NAME,
				   sort_filenames, NULL, NULL);

  gtk_tree_sortable_set_sort_func (g_result_sortable, FILE_SUBFOLDER,
				   sort_filepaths, NULL, NULL);

  gtk_tree_sortable_set_sort_func (g_result_sortable, FILE_SIZE,
				   sort_filesizes, NULL, NULL);

  g_modified = FILE_MODIFIED;
  gtk_tree_sortable_set_sort_func (g_result_sortable, FILE_MODIFIED,
				   sort_filetimes, &g_modified, NULL);

  gtk_tree_sortable_set_sort_func (g_result_sortable, FILE_MEMBERSHIP,
				   sort_filememberships, NULL, NULL);

  gtk_tree_sortable_set_sort_func (g_result_sortable, FILE_PERM,
				   sort_fileperms, NULL, NULL);


  gtk_tree_sortable_set_default_sort_func (g_result_sortable,
					   sort_filenames_default, NULL,
					   NULL);

  gtk_tree_sortable_set_sort_column_id (g_result_sortable,
					GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					GTK_SORT_ASCENDING);


  gtk_tree_view_set_model (GTK_TREE_VIEW (g_result_treeview),
			   GTK_TREE_MODEL (g_result_store));

  gtk_container_add (GTK_CONTAINER (result_scrolled_window),
		     g_result_treeview);
}
