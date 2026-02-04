// SPDX-License-Identifier: GPL-2.0-only
/*
 * properties_page.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "utils.h"
#include "gtk_common.h"
#include "gtk_time.h"


extern GtkWidget *g_app_window;
extern GtkWidget *g_app_find_button;
void app_find (GtkWidget *, gpointer data);


clock_scale_t g_clock_scales[6] = {
  {.scale = "Minutes",.range = 60,.secs = 60}
  ,
  {.scale = "Hours",.range = 24,.secs = 60 * 60}
  ,
  {.scale = "Days",.range = 6,.secs = 24 * 60 * 60}
  ,
  {.scale = "Weeks",.range = 5,.secs = 7 * 24 * 60 * 60}
  ,
  {.scale = "Months",.range = 12,.secs = 30 * 24 * 60 * 60}
  ,
  {.scale = "Years",.range = 100,.secs = 12 * 30 * 24 * 60 * 60}
};

int g_clock_scales_len = 6;


GtkWidget *g_time_check;
GtkWidget *g_time_op_combo_box_text;
GtkWidget *g_time_between_radio_button, *g_time_during_radio_button;
GtkWidget *g_time_lclock_button, *g_time_rclock_button;
GtkWidget *g_time_range_spin;
GtkWidget *g_time_stamp_combo_box_text;
GtkWidget *g_permissions_check;
GtkWidget *g_user_permission_hbox, *g_group_permission_hbox,
  *g_others_permission_hbox;
GtkWidget *g_user_read_permission_combo_box_text,
  *g_group_read_permission_combo_box_text,
  *g_others_read_permission_combo_box_text;
GtkWidget *g_user_write_permission_combo_box_text,
  *g_group_write_permission_combo_box_text,
  *g_others_write_permission_combo_box_text;
GtkWidget *g_user_exe_permission_combo_box_text,
  *g_group_exe_permission_combo_box_text,
  *g_others_exe_permission_combo_box_text;
GtkWidget *g_user_combo_box_text, *g_group_combo_box_text;


GtkWidget *
setup_time_op_combo_box_text ()
{
  GtkWidget *time_op_combo_box_text = gtk_combo_box_text_new ();

  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (time_op_combo_box_text),
				  "Accessed");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (time_op_combo_box_text),
				  "Modified");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (time_op_combo_box_text),
				  "Changed");

  gtk_combo_box_set_active (GTK_COMBO_BOX (time_op_combo_box_text), 0);

  return time_op_combo_box_text;
}


static GtkAdjustment *
setup_time_clock_adjustment (int clock_scale, int ceil)
{
  return
    GTK_ADJUSTMENT (gtk_adjustment_new
		    (1.0, 0.0,
		     (float) (g_clock_scales[clock_scale].range - ceil), 1.0,
		     g_clock_scales[clock_scale].range / 8, 0.0));
}


static GtkWidget *
setup_time_clock_range_spin (int clock_scale, int ceil)
{
  GtkAdjustment *time_clock_integer_range =
    setup_time_clock_adjustment (clock_scale, ceil);

  GtkWidget *time_clock_range_spin =
    gtk_spin_button_new (time_clock_integer_range, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (time_clock_range_spin), TRUE);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (time_clock_range_spin),
				     GTK_UPDATE_IF_VALID);

  return time_clock_range_spin;
}


static void
calendar_dialog_set_time (GtkWidget * time_clock_button, gpointer data)
{
  GtkWidget *calendar_dialog = gtk_dialog_new_with_buttons ("Select Date",
							    GTK_WINDOW
							    (g_app_window),
							    GTK_DIALOG_MODAL |
							    GTK_DIALOG_DESTROY_WITH_PARENT,
							    GTK_STOCK_OK,
							    GTK_RESPONSE_ACCEPT,
							    GTK_STOCK_CANCEL,
							    GTK_RESPONSE_REJECT,
							    NULL);

  GtkWidget *calendar = gtk_calendar_new ();

  char *prev_time_str =
    (char *) gtk_button_get_label (GTK_BUTTON (time_clock_button));
  guint prev_day, prev_month, prev_year;
  sscanf (prev_time_str, "%d/%d/%d", &prev_day, &prev_month, &prev_year);

  gtk_calendar_select_month (GTK_CALENDAR (calendar), --prev_month,
			     prev_year);
  gtk_calendar_select_day (GTK_CALENDAR (calendar), prev_day);

  gtk_widget_show (calendar);


  GtkWidget *dialog_content_area =
    gtk_dialog_get_content_area (GTK_DIALOG (calendar_dialog));

  gtk_container_add (GTK_CONTAINER (dialog_content_area), calendar);

  gint calendar_dialog_response =
    gtk_dialog_run (GTK_DIALOG (calendar_dialog));

  guint new_year, new_month, new_day;

  switch (calendar_dialog_response)
    {
    case GTK_RESPONSE_ACCEPT:

      gtk_calendar_get_date (GTK_CALENDAR (calendar), &new_year, &new_month,
			     &new_day);

      if (new_year > 1900)
	new_year -= 1900;
      struct tm new_timem = {.tm_mday = new_day,.tm_mon = new_month,.tm_year =
	  new_year
      };
      char *new_time_str = get_tm_to_time (&new_timem, "%d/%m/%g");

      gtk_button_set_label (GTK_BUTTON (time_clock_button), new_time_str);
      app_free (new_time_str);
      break;

    }

  gtk_widget_destroy (calendar_dialog);
}


static GtkWidget *
setup_time_clock_button_with_calendar ()
{

  char *time_str = get_date ();
  GtkWidget *time_clock_button = gtk_button_new_with_label (time_str);
  app_free (time_str);

  g_signal_connect (G_OBJECT (time_clock_button), "clicked",
		    G_CALLBACK (calendar_dialog_set_time), NULL);

  return time_clock_button;
}


static GtkWidget *
setup_time_stamp_combo_box_text ()
{
  GtkWidget *time_stamp_combo_box_text = gtk_combo_box_text_new ();

  int clock_index;
  for (clock_index = 0; clock_index < g_clock_scales_len; clock_index++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (time_stamp_combo_box_text),
				      g_clock_scales[clock_index].scale);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (time_stamp_combo_box_text), 0);

  return time_stamp_combo_box_text;
}


static GtkWidget *
setup_permission_hbox (int usergroup_or_others,
		       GtkWidget ** perm_read_combo_box_text_ptr,
		       GtkWidget ** perm_write_combo_box_text_ptr,
		       GtkWidget ** perm_exe_combo_box_text_ptr)
{

  GtkWidget *perm_read_combo_box_text = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_read_combo_box_text), " ");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_read_combo_box_text), "-");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_read_combo_box_text), "r");

  GtkWidget *perm_write_combo_box_text = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_write_combo_box_text), " ");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_write_combo_box_text), "-");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_write_combo_box_text), "w");

  GtkWidget *perm_exe_combo_box_text = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_exe_combo_box_text), " ");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_exe_combo_box_text), "-");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (perm_exe_combo_box_text), "x");
  if (usergroup_or_others != OTHERS)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (perm_exe_combo_box_text), "s");
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (perm_exe_combo_box_text), "S");
    }
  else
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (perm_exe_combo_box_text), "t");
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (perm_exe_combo_box_text), "T");
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (perm_read_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX (perm_write_combo_box_text), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX (perm_exe_combo_box_text), 0);

  GtkWidget *permission_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (permission_hbox), perm_read_combo_box_text,
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (permission_hbox), perm_write_combo_box_text,
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (permission_hbox), perm_exe_combo_box_text,
		      TRUE, TRUE, 0);

  *perm_read_combo_box_text_ptr = perm_read_combo_box_text;
  *perm_write_combo_box_text_ptr = perm_write_combo_box_text;
  *perm_exe_combo_box_text_ptr = perm_exe_combo_box_text;

  return permission_hbox;
}


static int
usergroup_list_add_sorted (gconstpointer usergroup1, gconstpointer usergroup2)
{
  return strcasecmp (usergroup1, usergroup2);
}


static GtkWidget *
setup_usergroup_combo_box_text (int user_or_group)
{
  GtkWidget *usergroup_combo_box_text = gtk_combo_box_text_new_with_entry ();

  GList *usergroup_sorted_list = NULL;
  if (user_or_group == USER)
    {
      setpwent ();

      struct passwd *pass;
      while ((pass = getpwent ()) != NULL)
	{
	  if (pass->pw_name != NULL)
	    {
	      usergroup_sorted_list =
		g_list_insert_sorted (usergroup_sorted_list,
				      strdup (pass->pw_name),
				      usergroup_list_add_sorted);

	    }
	}

      endpwent ();
    }
  else
    {
      setgrent ();

      struct group *grp;
      while ((grp = getgrent ()) != NULL)
	{
	  if (grp->gr_name != NULL)
	    {
	      usergroup_sorted_list =
		g_list_insert_sorted (usergroup_sorted_list,
				      strdup (grp->gr_name),
				      usergroup_list_add_sorted);
	    }
	}

      endgrent ();
    }


  GtkEntryCompletion *usergroup_combo_box_text_entry_completion =
    gtk_entry_completion_new ();
  gtk_entry_completion_set_text_column
    (usergroup_combo_box_text_entry_completion, 0);
  gtk_entry_completion_set_inline_selection
    (usergroup_combo_box_text_entry_completion, TRUE);

  GtkListStore *usergroup_combo_box_text_entry_completion_store =
    gtk_list_store_new (1, G_TYPE_STRING);

  GtkTreeIter usergroup_combo_box_text_entry_completion_list_iter;
  GList *usergroup_sorted_list_curr;
  for (usergroup_sorted_list_curr = usergroup_sorted_list;
       usergroup_sorted_list_curr;
       usergroup_sorted_list_curr = g_list_next (usergroup_sorted_list_curr))
    {

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (usergroup_combo_box_text),
				      (char *)
				      usergroup_sorted_list_curr->data);

      gtk_list_store_append
	(usergroup_combo_box_text_entry_completion_store,
	 &usergroup_combo_box_text_entry_completion_list_iter);
      gtk_list_store_set
	(usergroup_combo_box_text_entry_completion_store,
	 &usergroup_combo_box_text_entry_completion_list_iter, 0,
	 (char *) usergroup_sorted_list_curr->data, -1);

    }

  g_list_free_full (usergroup_sorted_list, free);

  gtk_entry_completion_set_model (usergroup_combo_box_text_entry_completion,
				  GTK_TREE_MODEL
				  (usergroup_combo_box_text_entry_completion_store));

  g_object_unref (usergroup_combo_box_text_entry_completion_store);

  gtk_entry_set_completion (GTK_ENTRY
			    (gtk_bin_get_child
			     (GTK_BIN (usergroup_combo_box_text))),
			    usergroup_combo_box_text_entry_completion);

  return usergroup_combo_box_text;
}


static void
enable_time_between (GtkToggleButton * time_between_radio_button,
		     gpointer data)
{
  int time_between_setting = TRUE, time_during_setting = FALSE;

  if (!gtk_toggle_button_get_active (time_between_radio_button))
    {
      time_between_setting = FALSE;
      time_during_setting = TRUE;
    }

  set_widgets_sensitive (time_between_setting, 2, g_time_lclock_button,
			 g_time_rclock_button);
  set_widgets_sensitive (time_during_setting, 2, g_time_range_spin,
			 g_time_stamp_combo_box_text);
}


static void
enable_time_between_during (GtkToggleButton * time_check, gpointer data)
{
  if (!gtk_toggle_button_get_active (time_check))
    {
      set_widgets_sensitive (FALSE, 7, g_time_op_combo_box_text,
			     g_time_between_radio_button,
			     g_time_lclock_button, g_time_rclock_button,
			     g_time_during_radio_button, g_time_range_spin,
			     g_time_stamp_combo_box_text);
    }
  else
    {
      set_widgets_sensitive (TRUE, 3, g_time_op_combo_box_text,
			     g_time_between_radio_button,
			     g_time_during_radio_button);
      enable_time_between (GTK_TOGGLE_BUTTON (g_time_between_radio_button),
			   NULL);
    }
}


void
switch_time_spin_range (GtkWidget * time_stamp_combo_box_text,
			GtkWidget * time_range_spin)
{
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (time_range_spin),
				  setup_time_clock_adjustment
				  (gtk_combo_box_get_active
				   (GTK_COMBO_BOX
				    (time_stamp_combo_box_text)), 1));
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (time_range_spin), 1.0);
}


static void
enable_permissions_hbox (GtkToggleButton * permissions_check, gpointer data)
{
  int permissions_hbox_setting = FALSE;

  if (gtk_toggle_button_get_active (permissions_check))
    permissions_hbox_setting = TRUE;

  set_widgets_sensitive (permissions_hbox_setting, 3, g_user_permission_hbox,
			 g_group_permission_hbox, g_others_permission_hbox);
}


void
setup_properties_page (GtkWidget * properties_page_table)
{
  gtk_table_set_row_spacings (GTK_TABLE (properties_page_table), 8);
  gtk_table_set_col_spacings (GTK_TABLE (properties_page_table), 8);

  g_time_check = gtk_check_button_new_with_mnemonic ("_Time Last Operated:");
  gtk_widget_set_halign (g_time_check, GTK_ALIGN_START);


  g_time_op_combo_box_text = setup_time_op_combo_box_text ();


  g_time_between_radio_button =
    gtk_radio_button_new_with_label (NULL, "Time between:");
  gtk_widget_set_margin_left (g_time_between_radio_button, 20);


  g_time_lclock_button = setup_time_clock_button_with_calendar ();


  GtkWidget *time_and_label = gtk_label_new_with_mnemonic ("and");


  g_time_rclock_button = setup_time_clock_button_with_calendar ();


  g_time_during_radio_button =
    gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON
						    (g_time_between_radio_button),
						    "_During previous:");
  gtk_widget_set_margin_left (g_time_during_radio_button, 20);


  g_time_range_spin = setup_time_clock_range_spin (MINUTES, 1);
  g_signal_connect_swapped (G_OBJECT (g_time_range_spin), "activate",
			    G_CALLBACK (app_find), g_app_find_button);


  g_time_stamp_combo_box_text = setup_time_stamp_combo_box_text ();

  set_widgets_sensitive (FALSE, 7, g_time_op_combo_box_text,
			 g_time_between_radio_button, g_time_lclock_button,
			 g_time_rclock_button, g_time_during_radio_button,
			 g_time_range_spin, g_time_stamp_combo_box_text);

  g_signal_connect (G_OBJECT (g_time_check), "toggled",
		    G_CALLBACK (enable_time_between_during), NULL);

  g_signal_connect (G_OBJECT (g_time_between_radio_button), "toggled",
		    G_CALLBACK (enable_time_between), NULL);

  g_signal_connect (G_OBJECT (g_time_stamp_combo_box_text), "changed",
		    G_CALLBACK (switch_time_spin_range), g_time_range_spin);


  g_permissions_check = gtk_check_button_new_with_mnemonic ("Per_missions:");
  gtk_widget_set_halign (g_permissions_check, GTK_ALIGN_START);


  g_user_permission_hbox =
    setup_permission_hbox (USER, &g_user_read_permission_combo_box_text,
			   &g_user_write_permission_combo_box_text,
			   &g_user_exe_permission_combo_box_text);


  g_group_permission_hbox =
    setup_permission_hbox (GROUP, &g_group_read_permission_combo_box_text,
			   &g_group_write_permission_combo_box_text,
			   &g_group_exe_permission_combo_box_text);


  g_others_permission_hbox =
    setup_permission_hbox (OTHERS, &g_others_read_permission_combo_box_text,
			   &g_others_write_permission_combo_box_text,
			   &g_others_exe_permission_combo_box_text);

  set_widgets_sensitive (FALSE, 3, g_user_permission_hbox,
			 g_group_permission_hbox, g_others_permission_hbox);

  g_signal_connect (G_OBJECT (g_permissions_check), "toggled",
		    G_CALLBACK (enable_permissions_hbox), NULL);


  GtkWidget *user_label = gtk_label_new_with_mnemonic ("_Owned by:");
  gtk_misc_set_alignment (GTK_MISC (user_label), 0, 0);


  g_user_combo_box_text = setup_usergroup_combo_box_text (USER);
  g_signal_connect_swapped (G_OBJECT
			    (gtk_bin_get_child
			     (GTK_BIN (g_user_combo_box_text))), "activate",
			    G_CALLBACK (app_find), g_app_find_button);


  GtkWidget *group_label =
    gtk_label_new_with_mnemonic ("Belonging to _Group:");
  gtk_misc_set_alignment (GTK_MISC (group_label), 0, 0);


  g_group_combo_box_text = setup_usergroup_combo_box_text (GROUP);
  g_signal_connect_swapped (G_OBJECT
			    (gtk_bin_get_child
			     (GTK_BIN (g_group_combo_box_text))), "activate",
			    G_CALLBACK (app_find), g_app_find_button);

  gtk_label_set_mnemonic_widget (GTK_LABEL (user_label),
				 g_user_combo_box_text);
  gtk_label_set_mnemonic_widget (GTK_LABEL (group_label),
				 g_group_combo_box_text);


  gtk_table_attach (GTK_TABLE (properties_page_table), g_time_check, 0, 1, 0,
		    1, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table),
		    g_time_op_combo_box_text, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table),
		    g_time_between_radio_button, 0, 1, 1, 2, GTK_FILL,
		    GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), g_time_lclock_button,
		    1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), time_and_label, 2, 3,
		    1, 2, GTK_EXPAND, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), g_time_rclock_button,
		    3, 4, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table),
		    g_time_during_radio_button, 0, 1, 2, 3, GTK_FILL,
		    GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), g_time_range_spin, 1,
		    3, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table),
		    g_time_stamp_combo_box_text, 3, 4, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), g_permissions_check, 0,
		    1, 3, 4, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), g_user_permission_hbox,
		    1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table),
		    g_group_permission_hbox, 2, 3, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table),
		    g_others_permission_hbox, 3, 4, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), user_label, 0, 1, 4, 5,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), g_user_combo_box_text,
		    1, 2, 4, 5, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), group_label, 2, 3, 4,
		    5, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (properties_page_table), g_group_combo_box_text,
		    3, 4, 4, 5, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
}
