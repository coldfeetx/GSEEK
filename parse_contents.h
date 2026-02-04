// SPDX-License-Identifier: GPL-2.0-only
/*
 * parse_contents.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

extern g_content_table;

GtkWidget *setup_content_combo_box_text (void);

int content_table_get_len (void);

char *content_table_get_content (char *);

char *content_table_get_description (char *);

void content_table_free (void);

void content_table_free_all (void);

int content_desc_in_generic_content_list (char *);

int content_desc_match_generic (char *, char *);

int content_type_pdf (char *);

int content_desc_searchable (char *);

int content_match (char *, char *);

int content_desc_text_generic_searchable (char *);

int executable (char *, char *);
