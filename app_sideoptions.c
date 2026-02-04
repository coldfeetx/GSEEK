// SPDX-License-Identifier: GPL-2.0-only
/*
 * app_sideoptions.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "utils.h"
#include "gtk_common.h"
#include "app_externs.h"
#include "nftw.h"
#include "app_fileops.h"
#include "parse_contents.h"
#include "properties_page.h"
#include "filematches.h"


extern GtkWidget *g_appop_notebook;
extern GtkWidget *g_filetrash_menuitem, *g_filedel_menuitem;
extern FILE *g_app_history_fp;

extern void enable_content_entry (GtkWidget *, GtkWidget *, guint, gpointer);

GtkWidget *g_app_find_button, *g_app_stop_button, *g_app_clear_button,
  *g_app_saveas_button, *g_app_quit_button, *g_app_about_button;

search_settings_t g_search_settings;
char g_search_error_str[LINE_MAX];

char *g_sdirname = NULL;
unsigned long int g_file_search_count_tot, g_file_search_count,
  g_file_match_count;
int g_app_stopped;

filematches_t g_filematches_settings;


static int
verify_search_settings ()
{
  char *location_entry =
    (char *) gtk_entry_get_text (GTK_ENTRY (g_location_entry));

  if (STRING_IS_EMPTY (location_entry))
    {
      print_to_status_statusbar ("<Location Entry> cannot be Empty!");

      return APP_FAILURE;
    }

  char *location_path = realpath (location_entry, NULL);

  if (STRING_IS_NULL_OR_EMPTY (location_path))
    {
      print_to_status_statusbar ("<Location Entry %s> Invalid!",
				 (char *)
				 gtk_entry_get_text (GTK_ENTRY
						     (g_location_entry)));

      return APP_FAILURE;
    }

  location_entry = location_path;

  struct stat location_sbuf;
  errno = 0;
  if (stat (location_entry, &location_sbuf) != APP_SUCCESS)
    {
      print_to_status_statusbar
	("<Location Entry \"%s\"> could not be retrieved!: %s",
	 location_entry, strerror (errno));

      free (location_entry);
      return APP_FAILURE;
    }
  if (!S_ISDIR (location_sbuf.st_mode))
    {
      print_to_status_statusbar ("<Location Entry \"%s\"> not a Directory!",
				 location_entry);

      free (location_entry);
      return APP_FAILURE;
    }

  gtk_entry_set_text (GTK_ENTRY (g_location_entry), location_entry);
  free (location_entry);
  refresh_gtk ();

  char *content_desc =
    gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					(g_content_combo_box_text));

  if (content_desc_in_generic_content_list (content_desc) != APP_SUCCESS
      && content_table_get_content (content_desc) == NULL)
    {
      print_to_status_statusbar
	("<Content Type \"%s\"> Invalid!",
	 gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					     (g_content_combo_box_text)));

      return APP_FAILURE;
    }

  if (!strcmp
      (gtk_combo_box_text_get_active_text
       (GTK_COMBO_BOX_TEXT (g_size_cond_combo_box_text)), "Within"))
    {
      off_t size_lrange =
	(off_t) (gtk_spin_button_get_value_as_int
		 (GTK_SPIN_BUTTON (g_size_lrange_spin)) * (off_t) powl (1024,
									gtk_combo_box_get_active
									(GTK_COMBO_BOX
									 (g_size_lunit_combo_box_text))));

      off_t size_rrange =
	(off_t) (gtk_spin_button_get_value_as_int
		 (GTK_SPIN_BUTTON (g_size_rrange_spin)) * (off_t) powl (1024,
									gtk_combo_box_get_active
									(GTK_COMBO_BOX
									 (g_size_runit_combo_box_text))));

      if (size_lrange > size_rrange)
	{
	  print_to_status_statusbar
	    ("<Size Range %jd Bytes - %jd Bytes> invalid!",
	     (intmax_t) size_lrange, (intmax_t) size_rrange);

	  return APP_FAILURE;
	}
    }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_time_check)))
    {
      if (gtk_toggle_button_get_active
	  (GTK_TOGGLE_BUTTON (g_time_between_radio_button)))
	{
	  char *lclock_unflipped =
	    (char *) gtk_button_get_label (GTK_BUTTON (g_time_lclock_button)),
	    *rclock_unflipped =
	    (char *) gtk_button_get_label (GTK_BUTTON (g_time_rclock_button));
	  char *lclock = flip_date (lclock_unflipped), *rclock =
	    flip_date (rclock_unflipped);

	  int clock_diff = strcmp (lclock, rclock);
	  free (lclock);
	  free (rclock);

	  if (clock_diff > 0)
	    {
	      print_to_status_statusbar ("<Time Range \"%s - %s\"> invalid!",
					 lclock_unflipped, rclock_unflipped);

	      return APP_FAILURE;
	    }
	}

    }


  char *user =
    (char *)
    gtk_entry_get_text (GTK_ENTRY
			(gtk_bin_get_child
			 (GTK_BIN (g_user_combo_box_text))));
  if (!STRING_IS_EMPTY (user))
    {
      char *user_endptr;
      uid_t uid = (uid_t) strtol (user, &user_endptr, 10);

      if (!STRING_IS_EMPTY (user_endptr))
	{
	  if (getpwnam (user) == NULL)
	    {
	      print_to_status_statusbar ("<User %s Invalid!>", user);

	      return APP_FAILURE;
	    }
	}

    }


  char *group =
    (char *)
    gtk_entry_get_text (GTK_ENTRY
			(gtk_bin_get_child
			 (GTK_BIN (g_group_combo_box_text))));
  if (!STRING_IS_EMPTY (group))
    {
      char *group_endptr;
      gid_t gid = (gid_t) strtol (group, &group_endptr, 10);

      if (!STRING_IS_EMPTY (group_endptr))
	{
	  if (getgrnam (group) == NULL)
	    {
	      print_to_status_statusbar ("<Group %s Invalid!>", group);

	      return APP_FAILURE;
	    }
	}

    }

  return APP_SUCCESS;
}


static void
search_restore_default_settings ()
{
  set_widgets_sensitive (FALSE, 1, g_app_saveas_button);

  gtk_combo_box_set_active (GTK_COMBO_BOX (g_name_combo_box_text), 0);

  char *current_dir;
  if ((current_dir = (char *) get_current_dir_name ()) != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (g_location_entry), current_dir);
      free (current_dir);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (g_content_combo_box_text), 0);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_regexp_setting_check),
				FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_shexp_setting_check),
				FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_recurse_setting_check),
				TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_case_setting_check),
				FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_hidden_setting_check),
				FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_matchonce_setting_check),
				FALSE);

  gtk_entry_set_text (GTK_ENTRY (g_content_entry), "");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_ccase_setting_check),
				FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_cregexp_setting_check),
				FALSE);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (g_size_lrange_spin), 0);

  gtk_combo_box_set_active (GTK_COMBO_BOX (g_size_lunit_combo_box_text), 0);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (g_size_rrange_spin), 0);

  gtk_combo_box_set_active (GTK_COMBO_BOX (g_size_runit_combo_box_text), 0);

  gtk_combo_box_set_active (GTK_COMBO_BOX (g_size_cond_combo_box_text), 0);

  gtk_combo_box_set_active (GTK_COMBO_BOX (g_time_op_combo_box_text), 0);

  char *date_str = get_date ();
  gtk_button_set_label (GTK_BUTTON (g_time_lclock_button), date_str);

  gtk_button_set_label (GTK_BUTTON (g_time_rclock_button), date_str);
  app_free (date_str);

  gtk_combo_box_set_active (GTK_COMBO_BOX (g_time_stamp_combo_box_text), 0);
  switch_time_spin_range (g_time_stamp_combo_box_text, g_time_range_spin);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				(g_time_between_radio_button), TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_time_check), FALSE);

  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_user_read_permission_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_user_write_permission_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_user_exe_permission_combo_box_text), 0);

  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_group_read_permission_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_group_write_permission_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_group_exe_permission_combo_box_text), 0);

  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_others_read_permission_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_others_write_permission_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX
			    (g_others_exe_permission_combo_box_text), 0);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_permissions_check),
				FALSE);

  gtk_entry_set_text (GTK_ENTRY
		      (gtk_bin_get_child (GTK_BIN (g_user_combo_box_text))),
		      "");

  gtk_entry_set_text (GTK_ENTRY
		      (gtk_bin_get_child (GTK_BIN (g_group_combo_box_text))),
		      "");

  result_window_clear (g_result_store);

  statusbars_clear ();

  refresh_gtk ();
}

static void
create_search_settings ()
{
  g_search_settings.name =
    gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					(g_name_combo_box_text));

  g_search_settings.location =
    (char *) gtk_entry_get_text (GTK_ENTRY (g_location_entry));

  char *content_desc =
    gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					(g_content_combo_box_text));
  if (content_desc_in_generic_content_list (content_desc) == APP_SUCCESS)
    {
      g_search_settings.content_generic = 1;
      g_search_settings.content_type = strdup (content_desc);
    }
  else
    {
      g_search_settings.content_generic = 0;
      g_search_settings.content_type =
	strdup (content_table_get_content (content_desc));
    }

  g_search_settings.regexp =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_regexp_setting_check));

  g_search_settings.shexp =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_shexp_setting_check));

  g_search_settings.shexp_tofree = TRUE;

  g_search_settings.recurse =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				  (g_recurse_setting_check));

  g_search_settings.casei =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_case_setting_check));

  g_search_settings.hidden =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_hidden_setting_check));

  g_search_settings.matchonce =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				  (g_matchonce_setting_check));

  if (gtk_widget_get_sensitive (g_content_entry)
      && !STRING_IS_EMPTY ((char *)
			   gtk_entry_get_text (GTK_ENTRY (g_content_entry))))
    {
      g_filematches_settings.content = g_search_settings.content = 1;
      g_filematches_settings.content_text = g_search_settings.content_text =
	(char *) gtk_entry_get_text (GTK_ENTRY (g_content_entry));

      g_filematches_settings.ccasei = g_search_settings.ccasei =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				      (g_ccase_setting_check));

      g_filematches_settings.cregexp = g_search_settings.cregexp =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				      (g_cregexp_setting_check));
    }
  else
    g_filematches_settings.content = g_search_settings.content = 0;

  g_search_settings.size_cond =
    gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					(g_size_cond_combo_box_text));
  if (strcmp (g_search_settings.size_cond, "(None)"))
    {
      g_search_settings.size_lrange =
	(off_t) (gtk_spin_button_get_value_as_int
		 (GTK_SPIN_BUTTON (g_size_lrange_spin)) * (off_t) powl (1024,
									gtk_combo_box_get_active
									(GTK_COMBO_BOX
									 (g_size_lunit_combo_box_text))));

      if (!strcmp (g_search_settings.size_cond, "Within"))
	{
	  g_search_settings.size_rrange =
	    (off_t) (gtk_spin_button_get_value_as_int
		     (GTK_SPIN_BUTTON (g_size_rrange_spin)) *
		     (off_t) powl (1024,
				   gtk_combo_box_get_active (GTK_COMBO_BOX
							     (g_size_runit_combo_box_text))));
	}
    }

  if (g_search_settings.time =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_time_check)))
    {
      g_search_settings.time_op =
	gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					    (g_time_op_combo_box_text));

      if (g_search_settings.time_between =
	  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					(g_time_between_radio_button)))
	{
	  g_search_settings.time_between_lclock =
	    flip_date ((char *)
		       gtk_button_get_label (GTK_BUTTON
					     (g_time_lclock_button)));
	  g_search_settings.time_between_rclock =
	    flip_date ((char *)
		       gtk_button_get_label (GTK_BUTTON
					     (g_time_rclock_button)));
	  g_search_settings.time_during = FALSE;
	}
      else
	{
	  g_search_settings.time_during = TRUE;
	  g_search_settings.time_range =
	    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
					      (g_time_range_spin));
	  g_search_settings.time_stamp =
	    gtk_combo_box_get_active (GTK_COMBO_BOX
				      (g_time_stamp_combo_box_text));
	}

    }

  if (g_search_settings.permission =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_permissions_check)))
    {
      char *permissions;
      asprintf (&permissions, "%c%c%c%c%c%c%c%c%c",
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_user_read_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_user_write_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_user_exe_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_group_read_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_group_write_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_group_exe_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_others_read_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_others_write_permission_combo_box_text))
		[0],
		gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
						    (g_others_exe_permission_combo_box_text))
		[0]);

      g_search_settings.permissions = permissions;
    }


  char *user =
    (char *)
    gtk_entry_get_text (GTK_ENTRY
			(gtk_bin_get_child
			 (GTK_BIN (g_user_combo_box_text))));
  if (!STRING_IS_EMPTY (user))
    {
      char *user_endptr;
      g_search_settings.uid = (uid_t) strtol (user, &user_endptr, 10);

      if (!STRING_IS_EMPTY (user_endptr))
	{
	  g_search_settings.uid = getpwnam (user)->pw_uid;
	}

      g_search_settings.user = 1;
    }
  else
    g_search_settings.user = 0;


  char *group =
    (char *)
    gtk_entry_get_text (GTK_ENTRY
			(gtk_bin_get_child
			 (GTK_BIN (g_group_combo_box_text))));
  if (!STRING_IS_EMPTY (group))
    {
      char *group_endptr;
      g_search_settings.gid = (gid_t) strtol (group, &group_endptr, 10);

      if (!STRING_IS_EMPTY (group_endptr))
	{
	  g_search_settings.gid = getgrnam (group)->gr_gid;
	}

      g_search_settings.group = 1;
    }
  else
    g_search_settings.group = 0;

}


static void
print_search_settings ()
{
  fprintf (stderr, "Search Settings:\n");

  fprintf (stderr, "\t\tName:%s\n"
	   "\t\tLocation:%s\n"
	   "\t\tRegexp:%d\n"
	   "\t\tShexp:%d\n"
	   "\t\tRecurse:%d\n"
	   "\t\tCasei:%d\n"
	   "\t\tHidden:%d\n"
	   "\t\tMatch Once:%d\n"
	   "\t\tContent Type:%s\n"
	   "\t\tContent Generic:%d\n"
	   "\t\tContent:%d\n"
	   "\t\tContent Text:%s\n"
	   "\t\tContent Casei:%d\n"
	   "\t\tContent Regexp:%d\n"
	   "\t\tSize Condition:%s\n"
	   "\t\tSize Range: <%jd - %jd>\n"
	   "\t\tTime:%d\n"
	   "\t\tTime Operation:%s\n"
	   "\t\tTime_Between:%d\n"
	   "\t\tTime Range: <%s - %s>\n"
	   "\t\tTime_During:%d\n"
	   "\t\tTime_Range:%d\tTime_Stamp:%d\n"
	   "\t\tPermission:%d\n"
	   "\t\tPermissions:%s\n"
	   "\t\tUser:%d\n"
	   "\t\tUID:%jd\n"
	   "\t\tGroup:%d\n"
	   "\t\tGID:%jd\n",
	   g_search_settings.name,
	   g_search_settings.location,
	   g_search_settings.regexp,
	   g_search_settings.shexp,
	   g_search_settings.recurse,
	   g_search_settings.casei,
	   g_search_settings.hidden,
	   g_search_settings.matchonce,
	   g_search_settings.content_type,
	   g_search_settings.content_generic,
	   g_search_settings.content,
	   g_search_settings.content_text,
	   g_search_settings.ccasei,
	   g_search_settings.cregexp,
	   g_search_settings.size_cond,
	   (intmax_t) (g_search_settings.size_lrange),
	   (intmax_t) (g_search_settings.size_rrange), g_search_settings.time,
	   g_search_settings.time_op, g_search_settings.time_between,
	   g_search_settings.time_between_lclock,
	   g_search_settings.time_between_rclock,
	   g_search_settings.time_during, g_search_settings.time_range,
	   g_search_settings.time_stamp, g_search_settings.permission,
	   g_search_settings.permissions, g_search_settings.user,
	   (intmax_t) g_search_settings.uid, g_search_settings.group,
	   (intmax_t) g_search_settings.gid);

}


static void
print_error_tokens (char *file, char *search_error_str)
{
  char search_error_str_local[strlen (search_error_str)],
    *search_error_str_local_ptr = search_error_str_local;
  strcpy (search_error_str_local, search_error_str);

  char *search_error_msg;

  print_to_status_statusbar ("File Search for File_Pattern \"%s\" Failed:",
			     file);

  while ((search_error_msg =
	  strsep (&search_error_str_local_ptr, "\n")) != NULL)
    {
      print_to_status_statusbar ("%s\n", search_error_msg);
    }

}


void
name_history_combo_list_add (char *search_name)
{

  GList *name_history_list;
  int file_in_history = FALSE;
  if (g_name_history_list != NULL)
    {
      int name_history_list_index;
      for (name_history_list_index = 0, name_history_list =
	   g_name_history_list; name_history_list;
	   name_history_list =
	   g_list_next (name_history_list), name_history_list_index++)
	{
	  if (!strcmp (search_name, name_history_list->data))
	    {
	      g_name_history_list =
		g_list_remove (g_name_history_list, name_history_list->data);
	      gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT
					 (g_name_combo_box_text),
					 name_history_list_index);
	      file_in_history = TRUE;
	      break;
	    }

	}

      g_name_history_list =
	g_list_prepend (g_name_history_list, strdup (search_name));
    }
  else
    {
      g_name_history_list =
	g_list_append (g_name_history_list, strdup (search_name));
    }

  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (g_name_combo_box_text),
				  0, search_name);

  if (file_in_history == FALSE)
    {
      GtkTreeIter name_combo_box_text_entry_completion_list_iter;
      gtk_list_store_append (g_name_combo_box_text_entry_completion_store,
			     &name_combo_box_text_entry_completion_list_iter);
      gtk_list_store_set (g_name_combo_box_text_entry_completion_store,
			  &name_combo_box_text_entry_completion_list_iter, 0,
			  search_name, -1);
    }
}


void
history_write_line (gpointer name_history_line, gpointer g_app_history_fp)
{
  static char name_history_linen[LINE_MAX];
  sprintf (name_history_linen, "%s\n", (char *) name_history_line);

  fputs ((const char *) name_history_linen, (FILE *) g_app_history_fp);
}


static void
disable_search ()
{
  set_widgets_sensitive (FALSE, 7, g_appop_notebook, g_app_find_button,
			 g_app_clear_button, g_app_saveas_button,
			 g_app_quit_button, g_filetrash_menuitem,
			 g_filedel_menuitem);

  set_widgets_sensitive (TRUE, 1, g_app_stop_button);

  refresh_gtk ();
}


static void
enable_search ()
{
  set_widgets_sensitive (TRUE, 6, g_appop_notebook, g_app_find_button,
			 g_app_clear_button, g_app_quit_button,
			 g_filetrash_menuitem, g_filedel_menuitem);

  set_widgets_sensitive (FALSE, 1, g_app_stop_button);

  app_set_default_focus ();

  refresh_gtk ();
}


void
app_find (GtkWidget * app_find_button, gpointer data)
{
  disable_search ();

  g_file_search_count_tot = g_file_search_count = g_file_match_count =
    g_app_stopped = 0;

  enable_content_entry (g_appop_notebook, NULL, 1, g_content_combo_box_text);

  if (g_filematches_settings.content && g_filematches_settings.cregexp)
    regfree (&g_filematches_settings.cregexp_preg);

  result_window_clear (g_result_store);

  statusbars_clear ();
  refresh_gtk ();

  if (verify_search_settings () != APP_SUCCESS)
    {
      app_set_default_focus ();
      enable_search ();
      return;
    }

  print_to_status_statusbar ("File Search initiated!");


  create_search_settings ();

  print_search_settings ();


  char *search_file = g_search_settings.name;


  if (g_search_settings.regexp)
    {
      int regexp_comp_flags = REG_EXTENDED | REG_NEWLINE;
      if (!g_search_settings.casei)
	regexp_comp_flags |= REG_ICASE;

      char *regexp_comp_error = NULL;
      if ((regexp_comp_error =
	   regexp_comp (&g_search_settings.regexp_preg, search_file,
			regexp_comp_flags)) != NULL)
	{
	  print_to_status_statusbar ("File Search for File \"%s\" Failed: %s",
				     search_file, regexp_comp_error);

	  free (regexp_comp_error);
	  goto free_regex;
	}
    }


  if (g_search_settings.cregexp)
    {
      int cregexp_comp_flags = REG_EXTENDED | REG_NEWLINE;
      if (!g_search_settings.ccasei)
	cregexp_comp_flags |= REG_ICASE;

      char *cregexp_comp_error = NULL;
      if ((cregexp_comp_error =
	   regexp_comp (&g_search_settings.cregexp_preg,
			g_search_settings.content_text,
			cregexp_comp_flags)) != NULL)
	{
	  print_to_status_statusbar ("File Search for File \"%s\" Failed: %s",
				     search_file, cregexp_comp_error);

	  free (cregexp_comp_error);
	  goto free_cregex;
	}
      else
	{
	  regexp_comp (&g_filematches_settings.cregexp_preg,
		       g_filematches_settings.content_text,
		       cregexp_comp_flags);
	}
    }

  if (g_app_stopped)
    {
      app_free_all ();
      goto free_shexp;
    }

  char *dirpath = g_search_settings.location;

  if (g_app_stopped)
    {
      app_free_all ();
      goto free_shexp;
    }


  (void) nftw ((const char *) dirpath, Search, 100, FTW_ACTIONRETVAL);


  if (!STRING_IS_EMPTY (g_search_error_str))
    print_error_tokens (search_file, g_search_error_str);

free_shexp:
  if (g_search_settings.shexp && g_search_settings.shexp_tofree == TRUE)
    shexp_free (&g_search_settings.shexp_wordv);

free_regex:
  if (g_search_settings.regexp)
    regfree (&g_search_settings.regexp_preg);

free_cregex:
  if (g_search_settings.cregexp)
    regfree (&g_search_settings.cregexp_preg);

  if (g_search_settings.content_type != NULL)
    free (g_search_settings.content_type);

  if (g_search_settings.permission)
    free (g_search_settings.permissions);

  if (g_search_settings.time_between)
    {
      free (g_search_settings.time_between_lclock);
      free (g_search_settings.time_between_rclock);
    }

  name_history_combo_list_add (search_file);
  g_sdirname = NULL;

  progress_statusbar_clear ();

  if (!g_app_stopped)
    {
      if (g_file_search_count_tot)
	{
	  print_to_status_statusbar ("%lu/%lu Files Matched!",
				     g_file_match_count,
				     g_file_search_count_tot);
	}
      else
	{
	  print_to_status_statusbar ("File Search Completed!");
	}
    }
  else
    {
      print_to_status_statusbar
	("File Search Stopped! %lu/%lu Files Matched!", g_file_match_count,
	 g_file_search_count_tot);
    }

  if (g_file_match_count > 0)
    set_widgets_sensitive (TRUE, 1, g_app_saveas_button);

  enable_search ();
}


static void
app_stop (GtkWidget * app_stop_button, gpointer data)
{
  print_to_status_statusbar ("Stopping File Search...");

  g_app_stopped = 1;

  print_to_status_statusbar ("File Search Stopped!");

  app_set_default_focus ();
}


static void
app_clear (GtkWidget * app_clear_button, gpointer data)
{
  print_to_status_statusbar ("Clearing File Search Results...");

  search_restore_default_settings ();

  enable_content_entry (g_appop_notebook, NULL, 1, g_content_combo_box_text);

  print_to_status_statusbar ("Cleared File Search Results!");

  app_set_default_focus ();
}


static void
app_save_as (GtkWidget * app_saveas_button, gpointer data)
{
  GtkWidget *save_as_dialog =
    gtk_file_chooser_dialog_new ("Save Search Results As ...",
				 GTK_WINDOW (g_app_window),
				 GTK_FILE_CHOOSER_ACTION_SAVE,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER
						  (save_as_dialog), TRUE);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (save_as_dialog),
				       ".");

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save_as_dialog),
				     SAVE_AS_SEARCH_RESULT_FILE);


  gint save_as_result = gtk_dialog_run (GTK_DIALOG (save_as_dialog));

  gtk_widget_hide (save_as_dialog);

  if (save_as_result == GTK_RESPONSE_ACCEPT)
    {
      char *app_search_result_filename =
	gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (save_as_dialog));

      print_to_progress_statusbar ("Saving Search Results to: \"%s\"\n",
				   app_search_result_filename);

      FILE *app_search_result_fp;
      errno = 0;
      if ((app_search_result_fp =
	   fopen (app_search_result_filename, "w")) == NULL)
	{
	  progress_statusbar_clear ();
	  print_to_status_statusbar
	    ("Could not Save Search Results: fopen(\"w\") on SAVE_AS_SEARCH_RESULT_FILE \"%s\" FAILED: %s",
	     SAVE_AS_SEARCH_RESULT_FILE, strerror (errno));
	}
      else
	{
	  char search_result_linen[LINE_MAX];

	  GtkTreeModel *result_model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (g_result_treeview));
	  GtkTreeIter result_iter;
	  gtk_tree_model_get_iter_from_string (result_model, &result_iter,
					       "0");

	  sprintf (search_result_linen,
		   "File%sPath%sSize%sModified%sMembership%sPermissions\n",
		   CSV_FSP, CSV_FSP, CSV_FSP, CSV_FSP, CSV_FSP);
	  fputs (search_result_linen, app_search_result_fp);

	  do
	    {
	      char *file_name, *file_subfolder, *file_size, *file_modified,
		*file_membership, *file_perm;

	      gtk_tree_model_get (result_model, &result_iter, FILE_NAME,
				  &file_name, FILE_SUBFOLDER, &file_subfolder,
				  FILE_SIZE, &file_size, FILE_MODIFIED,
				  &file_modified, FILE_MEMBERSHIP,
				  &file_membership, FILE_PERM, &file_perm,
				  -1);

	      sprintf (search_result_linen, "%s%s%s%s%s%s%s%s%s%s%s\n",
		       file_name, CSV_FSP, file_subfolder, CSV_FSP, file_size,
		       CSV_FSP, file_modified, CSV_FSP, file_membership,
		       CSV_FSP, file_perm);

	      fputs (search_result_linen, app_search_result_fp);

	    }
	  while (gtk_tree_model_iter_next (result_model, &result_iter));

	  fclose (app_search_result_fp);

	  progress_statusbar_clear ();
	  print_to_status_statusbar ("Saved to: \"%s\"",
				     app_search_result_filename);
	}

    }

  gtk_widget_destroy (save_as_dialog);
}


static void
app_quit (GtkWidget * app_quit_button, gpointer data)
{
  print_to_status_statusbar ("Closing App...");

  rewind (g_app_history_fp);
  g_list_foreach (g_name_history_list, history_write_line, g_app_history_fp);

  fclose (g_app_history_fp);

  gtk_main_quit ();

  printf ("App Closed!\n");
}


static void
app_about (GtkWidget * app_about_button, gpointer data)
{
  const char *app_authors[] = {
    "soumikbanerjee68@yahoo.com",
    NULL,
  };


  const char *app_documenters[] = {
    "soumikbanerjee68@yahoo.com",
    NULL,
  };


  GtkWidget *app_about_dialog = gtk_about_dialog_new ();

  GError *logo_load_error = NULL;
  GdkPixbuf *app_logo =
    gdk_pixbuf_new_from_file (LOGO_ICON, &logo_load_error);
  if (logo_load_error == NULL)
    gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (app_about_dialog), app_logo);
  else
    {
      if (logo_load_error->domain == GDK_PIXBUF_ERROR)
	fprintf (stderr, "GdkPixbufError: %s\n", logo_load_error->message);
      else if (logo_load_error->domain == G_FILE_ERROR)
	fprintf (stderr, "GFileError: %s\n", logo_load_error->message);
      else
	fprintf (stderr, "An Error occurred in the domain:%d\n",
		 logo_load_error->domain);

      g_error_free (logo_load_error);
    }

  gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (app_about_dialog),
				     APP_NAME);

  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (app_about_dialog), "1.0");

  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (app_about_dialog),
				  "(C) 2012 soumikbanerjee68@yahoo.com");

  gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (app_about_dialog),
				 "A GTK-based Powerful and Light-weight GUI File Searcher built from scratch and inspired from kfind!");

  gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (app_about_dialog),
				"SPDX-License-Identifier: GPL-2.0-only");
  gtk_about_dialog_set_wrap_license (GTK_ABOUT_DIALOG (app_about_dialog),
				     TRUE);

  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (app_about_dialog),
				app_authors);

  gtk_about_dialog_set_documenters (GTK_ABOUT_DIALOG (app_about_dialog),
				    app_documenters);


  gtk_dialog_run (GTK_DIALOG (app_about_dialog));

  gtk_widget_destroy (app_about_dialog);
}


void
setup_app_sideoptions (GtkWidget * app_sideoptions_table)
{
  gtk_table_set_row_spacings (GTK_TABLE (app_sideoptions_table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (app_sideoptions_table), 10);

  g_app_find_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
  gtk_widget_set_can_default (g_app_find_button, TRUE);
  g_signal_connect (G_OBJECT (g_app_find_button), "clicked",
		    G_CALLBACK (app_find), NULL);

  g_app_stop_button = gtk_button_new_from_stock (GTK_STOCK_STOP);
  g_signal_connect (G_OBJECT (g_app_stop_button), "clicked",
		    G_CALLBACK (app_stop), NULL);
  set_widgets_sensitive (FALSE, 1, g_app_stop_button);

  g_app_clear_button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
  gtk_widget_set_tooltip_text (g_app_clear_button,
			       "Reset Search Settings and Results");
  g_signal_connect (G_OBJECT (g_app_clear_button), "clicked",
		    G_CALLBACK (app_clear), NULL);

  g_app_saveas_button = gtk_button_new_from_stock (GTK_STOCK_SAVE_AS);
  gtk_widget_set_tooltip_text (g_app_saveas_button,
			       "Save Search Results in CSV Format");
  g_signal_connect (G_OBJECT (g_app_saveas_button), "clicked",
		    G_CALLBACK (app_save_as), NULL);
  set_widgets_sensitive (FALSE, 1, g_app_saveas_button);

  g_app_quit_button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
  gtk_widget_set_tooltip_text (g_app_quit_button,
			       "Save Search Item History and Quit");
  g_signal_connect (G_OBJECT (g_app_quit_button), "clicked",
		    G_CALLBACK (app_quit), NULL);

  g_app_about_button = gtk_button_new_from_stock (GTK_STOCK_ABOUT);
  g_signal_connect (G_OBJECT (g_app_about_button), "clicked",
		    G_CALLBACK (app_about), NULL);


  gtk_table_attach (GTK_TABLE (app_sideoptions_table), g_app_find_button, 0,
		    1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (app_sideoptions_table), g_app_stop_button, 0,
		    1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (app_sideoptions_table), g_app_clear_button, 0,
		    1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (app_sideoptions_table), g_app_saveas_button, 0,
		    1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (app_sideoptions_table), g_app_quit_button, 0,
		    1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (app_sideoptions_table), g_app_about_button, 0,
		    1, 6, 7, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
}
