// SPDX-License-Identifier: GPL-2.0-only
/*
 * content_page.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "gtk_common.h"
#include "parse_contents.h"


extern GtkWidget *g_appop_notebook;
extern GtkWidget *g_content_combo_box_text;
extern GtkWidget *g_app_find_button;
void app_find (GtkWidget *, gpointer data);


GtkWidget *g_content_entry;
GtkWidget *g_ccase_setting_check, *g_cregexp_setting_check;
GtkWidget *g_size_cond_combo_box_text;
GtkWidget *g_size_lrange_hbox, *g_size_rrange_hbox;
GtkWidget *g_size_lrange_spin, *g_size_rrange_spin;
GtkWidget *g_size_lunit_combo_box_text, *g_size_runit_combo_box_text;



static GtkWidget *
setup_size_cond_combo_box_text ()
{
  GtkWidget *size_cond_combo_box_text = gtk_combo_box_text_new ();

  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_cond_combo_box_text), "(None)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_cond_combo_box_text), "Less Than");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_cond_combo_box_text), "Equals");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_cond_combo_box_text), "Greater Than");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_cond_combo_box_text), "Within");

  gtk_combo_box_set_active (GTK_COMBO_BOX (size_cond_combo_box_text), 0);

  return size_cond_combo_box_text;
}


static GtkWidget *
setup_size_range_spin ()
{
  GtkAdjustment *size_integer_range =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 999, 1.0, 10.0, 0.0));

  GtkWidget *size_range_spin =
    gtk_spin_button_new (size_integer_range, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (size_range_spin), TRUE);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (size_range_spin),
				     GTK_UPDATE_IF_VALID);

  return size_range_spin;
}


static GtkWidget *
setup_size_unit_combo_box_text ()
{
  GtkWidget *size_unit_combo_box_text = gtk_combo_box_text_new ();

  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_unit_combo_box_text), "Bytes");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_unit_combo_box_text), "KB");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_unit_combo_box_text), "MB");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				  (size_unit_combo_box_text), "GB");

  gtk_combo_box_set_active (GTK_COMBO_BOX (size_unit_combo_box_text), 0);

  return size_unit_combo_box_text;
}


static void
size_cond_changed (GtkWidget * size_cond_combo_box_text, gpointer data)
{
  int left_setting = FALSE, right_setting = FALSE;

  char *size_cond_text =
    gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					(size_cond_combo_box_text));
  if (strcmp (size_cond_text, "(None)"))
    left_setting = TRUE;
  if (!strcmp (size_cond_text, "Within"))
    right_setting = TRUE;

  set_widgets_sensitive (left_setting, 1, g_size_lrange_hbox);
  set_widgets_sensitive (right_setting, 1, g_size_rrange_hbox);
}


void
enable_content_entry (GtkWidget * content_label, GtkWidget * notebook_page,
		      guint notebook_page_index,
		      GtkWidget * content_combo_box_text)
{

  if (notebook_page_index == 1)
    {
      int content_setting = TRUE;

      char *content_desc =
	gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
					    (content_combo_box_text));

      if (content_desc_searchable (content_desc) != APP_SUCCESS)
	content_setting = FALSE;

      set_widgets_sensitive (content_setting, 3, g_content_entry,
			     g_ccase_setting_check, g_cregexp_setting_check);
    }
}


static void
enable_csetting_setting (GtkWidget * content_entry,
			 GtkWidget * check_csetting_table)
{
  int csetting_setting = FALSE;

  if (strlen ((char *) gtk_entry_get_text (GTK_ENTRY (content_entry))) > 0)
    csetting_setting = TRUE;

  set_widgets_sensitive (csetting_setting, 1, check_csetting_table);
}


void
setup_content_page (GtkWidget * content_page_table)
{
  gtk_table_set_row_spacings (GTK_TABLE (content_page_table), 8);
  gtk_table_set_col_spacings (GTK_TABLE (content_page_table), 8);

  GtkWidget *content_label =
    gtk_label_new_with_mnemonic ("C_ontaining Text:");
  gtk_misc_set_alignment (GTK_MISC (content_label), 0, 0);


  g_content_entry = gtk_entry_new ();
  gtk_widget_set_tooltip_text (g_content_entry,
			       "Supported Searchable Types:\n"
			       "\tText File Types (Sources, Headers, general Text Files)\n"
			       "\tScripts (Shell, Python, Perl, TCL/Expect, Ruby, etc.)\n"
			       "\tPDF Documents\n");

  g_signal_connect_swapped (G_OBJECT (g_content_entry), "activate",
			    G_CALLBACK (app_find), g_app_find_button);


  GtkWidget *check_csetting_table = gtk_table_new (2, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (check_csetting_table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (check_csetting_table), 4);

  g_ccase_setting_check =
    gtk_check_button_new_with_mnemonic ("Case-Sensiti_ve");

  g_cregexp_setting_check =
    gtk_check_button_new_with_mnemonic ("_Regular Expression");

  gtk_table_attach (GTK_TABLE (check_csetting_table), g_ccase_setting_check,
		    0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (check_csetting_table), g_cregexp_setting_check,
		    0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

  set_widgets_sensitive (FALSE, 1, check_csetting_table);

  g_signal_connect (G_OBJECT (g_appop_notebook), "switch-page",
		    G_CALLBACK (enable_content_entry),
		    g_content_combo_box_text);

  g_signal_connect (G_OBJECT (g_content_entry), "changed",
		    G_CALLBACK (enable_csetting_setting),
		    check_csetting_table);


  GtkWidget *size_label = gtk_label_new_with_mnemonic ("S_ize:");
  gtk_misc_set_alignment (GTK_MISC (size_label), 0, 0);


  GtkWidget *size_table = gtk_table_new (1, 3, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (size_table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (size_table), 5);

  g_size_lrange_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  g_size_lrange_spin = setup_size_range_spin ();
  g_signal_connect_swapped (G_OBJECT (g_size_lrange_spin), "activate",
			    G_CALLBACK (app_find), g_app_find_button);

  g_size_lunit_combo_box_text = setup_size_unit_combo_box_text ();
  gtk_box_pack_start (GTK_BOX (g_size_lrange_hbox), g_size_lrange_spin, TRUE,
		      TRUE, 0);
  gtk_box_pack_start (GTK_BOX (g_size_lrange_hbox),
		      g_size_lunit_combo_box_text, TRUE, TRUE, 0);

  g_size_rrange_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  g_size_rrange_spin = setup_size_range_spin ();
  g_signal_connect_swapped (G_OBJECT (g_size_rrange_spin), "activate",
			    G_CALLBACK (app_find), g_app_find_button);
  g_size_runit_combo_box_text = setup_size_unit_combo_box_text ();
  gtk_box_pack_start (GTK_BOX (g_size_rrange_hbox), g_size_rrange_spin, TRUE,
		      TRUE, 0);
  gtk_box_pack_start (GTK_BOX (g_size_rrange_hbox),
		      g_size_runit_combo_box_text, TRUE, TRUE, 0);

  GtkWidget *size_and_label = gtk_label_new_with_mnemonic ("and");

  gtk_table_attach (GTK_TABLE (size_table), g_size_lrange_hbox, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (size_table), size_and_label, 1, 2, 0, 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (size_table), g_size_rrange_hbox, 2, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

  set_widgets_sensitive (FALSE, 2, g_size_lrange_hbox, g_size_rrange_hbox);


  g_size_cond_combo_box_text = setup_size_cond_combo_box_text ();
  g_signal_connect (G_OBJECT (g_size_cond_combo_box_text), "changed",
		    G_CALLBACK (size_cond_changed), NULL);

  gtk_label_set_mnemonic_widget (GTK_LABEL (content_label), g_content_entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (size_label),
				 g_size_cond_combo_box_text);


  gtk_table_attach (GTK_TABLE (content_page_table), content_label, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (content_page_table), g_content_entry, 1, 2, 0,
		    1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (content_page_table), check_csetting_table, 1,
		    2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (content_page_table), size_label, 0, 1, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (content_page_table), size_table, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (content_page_table),
		    g_size_cond_combo_box_text, 1, 2, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
}
