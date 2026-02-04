// SPDX-License-Identifier: GPL-2.0-only
/*
 * basic_page.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "shexp.h"
#include "parse_contents.h"


extern FILE *g_app_history_fp;
extern GtkWidget *g_app_find_button;
void app_find (GtkWidget *, gpointer data);


GtkWidget *g_name_combo_box_text;
GtkListStore *g_name_combo_box_text_entry_completion_store;
GtkWidget *g_location_entry;
GtkWidget *g_regexp_setting_check, *g_shexp_setting_check,
  *g_recurse_setting_check, *g_case_setting_check, *g_hidden_setting_check,
  *g_matchonce_setting_check;
GtkWidget *g_content_combo_box_text;

GList *g_name_history_list = NULL;


static GtkWidget *
setup_name_combo_box_text ()
{
  GtkWidget *name_combo_box_text = gtk_combo_box_text_new_with_entry ();

  GtkEntryCompletion *name_combo_box_text_entry_completion =
    gtk_entry_completion_new ();
  gtk_entry_completion_set_text_column (name_combo_box_text_entry_completion,
					0);

  g_name_combo_box_text_entry_completion_store =
    gtk_list_store_new (1, G_TYPE_STRING);
  gtk_entry_completion_set_model (name_combo_box_text_entry_completion,
				  GTK_TREE_MODEL
				  (g_name_combo_box_text_entry_completion_store));


  GtkTreeIter name_combo_box_text_entry_completion_list_iter;

  rewind (g_app_history_fp);
  char history_buf[PATH_MAX + 1];
  int file_count = 0;
  while (fgets (history_buf, PATH_MAX, g_app_history_fp) != NULL)
    {
      history_buf[strlen (history_buf) - 1] = '\0';

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (name_combo_box_text), history_buf);
      char *history_bufstr = strdup (history_buf);
      g_name_history_list =
	g_list_append (g_name_history_list, strdup (history_bufstr));

      gtk_list_store_append (g_name_combo_box_text_entry_completion_store,
			     &name_combo_box_text_entry_completion_list_iter);
      gtk_list_store_set (g_name_combo_box_text_entry_completion_store,
			  &name_combo_box_text_entry_completion_list_iter, 0,
			  history_bufstr, -1);

      if (file_count == 0)
	{
	  gtk_combo_box_set_active (GTK_COMBO_BOX (name_combo_box_text),
				    file_count);
	}

      file_count++;
    }

  gtk_entry_set_completion (GTK_ENTRY
			    (gtk_bin_get_child
			     (GTK_BIN (name_combo_box_text))),
			    name_combo_box_text_entry_completion);

  return name_combo_box_text;
}


static void
browse (GtkWidget * browse_button, GtkWidget * location_entry)
{
  GtkWidget *browse_dialog =
    gtk_file_chooser_dialog_new ("Select Folder to Search In",
				 GTK_WINDOW (gtk_widget_get_toplevel
					     (browse_button)),
				 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

  gint browse_result = gtk_dialog_run (GTK_DIALOG (browse_dialog));
  if (browse_result == GTK_RESPONSE_ACCEPT)
    {
      char *filename =
	gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (browse_dialog));
      if (!STRING_IS_NULL_OR_EMPTY (filename))
	{
	  gtk_entry_set_text (GTK_ENTRY (location_entry), filename);
	  g_free (filename);
	}
    }

  gtk_widget_destroy (browse_dialog);
}


static void
dir_complete (GtkEditable * location_entry,
	      GtkListStore * location_entry_completion_store)
{
  GtkTreeIter location_entry_completion_list_iter;

  gtk_list_store_clear (location_entry_completion_store);

  wordexp_t dir_completion_wordexp;
  char **dir_completion_wordv;

  char *file_to_complete;
  asprintf (&file_to_complete, "'%s'*",
	    (char *) gtk_entry_get_text (GTK_ENTRY (location_entry)));

  if (wordexp
      (file_to_complete, &dir_completion_wordexp,
       WRDE_NOCMD | WRDE_UNDEF) != APP_SUCCESS)
    goto free_return;

  dir_completion_wordv = dir_completion_wordexp.we_wordv;

  struct stat file_to_complete_sbuf;
  char *dir_completed;
  int dirs = 0;
  int dir_completion_index;
  for (dir_completion_index = 0;
       dir_completion_index < dir_completion_wordexp.we_wordc;
       dir_completion_index++)
    {
      if (strcmp
	  (dir_completion_wordv[dir_completion_index], file_to_complete)
	  && stat (dir_completion_wordv[dir_completion_index],
		   &file_to_complete_sbuf) == 0
	  && S_ISDIR (file_to_complete_sbuf.st_mode))
	{
	  asprintf (&dir_completed, "%s/",
		    dir_completion_wordv[dir_completion_index]);
	  gtk_list_store_append (location_entry_completion_store,
				 &location_entry_completion_list_iter);
	  gtk_list_store_set (location_entry_completion_store,
			      &location_entry_completion_list_iter, 0,
			      dir_completed, -1);
	  free (dir_completed);
	}
    }

  wordfree (&dir_completion_wordexp);

free_return:
  free (file_to_complete);
}


static void
setup_location_entry (GtkWidget * location_entry)
{
  char *current_dir = (char *) get_current_dir_name ();

  if (current_dir != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (location_entry), current_dir);
      free (current_dir);
    }
}


static void
toggle_check (GtkToggleButton * check1_toggle, GtkWidget * check2_widget)
{
  if (gtk_toggle_button_get_active (check1_toggle))
    gtk_widget_set_sensitive (check2_widget, FALSE);
  else
    gtk_widget_set_sensitive (check2_widget, TRUE);
}


void
setup_basic_page (GtkWidget * basic_page_table)
{
  gtk_table_set_row_spacings (GTK_TABLE (basic_page_table), 8);
  gtk_table_set_col_spacings (GTK_TABLE (basic_page_table), 8);

  GtkWidget *name_label = gtk_label_new_with_mnemonic ("_Named:");
  gtk_misc_set_alignment (GTK_MISC (name_label), 0, 0);


  g_name_combo_box_text = setup_name_combo_box_text ();
  g_signal_connect_swapped (G_OBJECT
			    (gtk_bin_get_child
			     (GTK_BIN (g_name_combo_box_text))), "activate",
			    G_CALLBACK (app_find), g_app_find_button);


  GtkWidget *location_label = gtk_label_new_with_mnemonic ("_Look In:");
  gtk_misc_set_alignment (GTK_MISC (location_label), 0, 0);


  GtkWidget *location_ip_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

  g_location_entry = gtk_entry_new ();

  GtkEntryCompletion *location_entry_completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_text_column (location_entry_completion, 0);
  GtkListStore *location_entry_completion_store =
    gtk_list_store_new (1, G_TYPE_STRING);
  gtk_entry_completion_set_model (location_entry_completion,
				  GTK_TREE_MODEL
				  (location_entry_completion_store));
  gtk_entry_set_completion (GTK_ENTRY (g_location_entry),
			    location_entry_completion);
  g_signal_connect (G_OBJECT (g_location_entry), "changed",
		    G_CALLBACK (dir_complete),
		    location_entry_completion_store);
  g_signal_connect_swapped (G_OBJECT (g_location_entry), "activate",
			    G_CALLBACK (app_find), g_app_find_button);

  setup_location_entry (g_location_entry);

  GtkWidget *browse_button = gtk_button_new_with_mnemonic ("Bro_wse");
  g_signal_connect (G_OBJECT (browse_button), "clicked", G_CALLBACK (browse),
		    (gpointer) g_location_entry);
  gtk_box_pack_start (GTK_BOX (location_ip_hbox), g_location_entry, TRUE,
		      TRUE, 0);
  gtk_box_pack_start (GTK_BOX (location_ip_hbox), browse_button, FALSE, FALSE,
		      0);


  GtkWidget *content_label = gtk_label_new_with_mnemonic ("_Type:");
  gtk_misc_set_alignment (GTK_MISC (content_label), 0, 0);


  g_content_combo_box_text = setup_content_combo_box_text ();
  g_signal_connect_swapped (G_OBJECT
			    (gtk_bin_get_child
			     (GTK_BIN (g_content_combo_box_text))),
			    "activate", G_CALLBACK (app_find),
			    g_app_find_button);

  gtk_label_set_mnemonic_widget (GTK_LABEL (name_label),
				 g_name_combo_box_text);
  gtk_label_set_mnemonic_widget (GTK_LABEL (location_label),
				 g_location_entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (content_label),
				 g_content_combo_box_text);


  GtkWidget *check_nsetting_table = gtk_table_new (3, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (check_nsetting_table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (check_nsetting_table), 5);

  g_regexp_setting_check =
    gtk_check_button_new_with_mnemonic ("Re_gular Expression");
  g_shexp_setting_check =
    gtk_check_button_new_with_mnemonic ("Shell-T_ype Expansion");
  g_recurse_setting_check = gtk_check_button_new_with_mnemonic ("_Recurse");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (g_recurse_setting_check),
				TRUE);

  g_case_setting_check =
    gtk_check_button_new_with_mnemonic ("Case-Sensiti_ve");
  g_hidden_setting_check =
    gtk_check_button_new_with_mnemonic ("Show _Hidden Files");
  g_matchonce_setting_check =
    gtk_check_button_new_with_mnemonic ("_Match Once");

  g_signal_connect (G_OBJECT (g_regexp_setting_check), "toggled",
		    G_CALLBACK (toggle_check), g_shexp_setting_check);
  g_signal_connect (G_OBJECT (g_shexp_setting_check), "toggled",
		    G_CALLBACK (toggle_check), g_regexp_setting_check);
  g_signal_connect (G_OBJECT (g_shexp_setting_check), "toggled",
		    G_CALLBACK (toggle_check), g_case_setting_check);

  gtk_table_attach (GTK_TABLE (check_nsetting_table), g_regexp_setting_check,
		    0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (check_nsetting_table), g_shexp_setting_check,
		    1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (check_nsetting_table), g_recurse_setting_check,
		    0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (check_nsetting_table), g_case_setting_check, 1,
		    2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (check_nsetting_table), g_hidden_setting_check,
		    0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (check_nsetting_table),
		    g_matchonce_setting_check, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);


  gtk_table_attach (GTK_TABLE (basic_page_table), name_label, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (basic_page_table), g_name_combo_box_text, 1, 2,
		    0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (basic_page_table), location_label, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (basic_page_table), location_ip_hbox, 1, 2, 1,
		    2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (basic_page_table), content_label, 0, 1, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (basic_page_table), g_content_combo_box_text, 1,
		    2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (basic_page_table), check_nsetting_table, 1, 2,
		    3, 4, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
}
