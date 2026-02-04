// SPDX-License-Identifier: GPL-2.0-only
/*
 * parse_contents.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "content_table.h"
#include "utils.h"
#include "gtk_common.h"


content_t *g_content_table = NULL;
guint g_content_table_len;


static int subset_of_all (char *);
static int subset_of_files (char *);
static int subset_of_generic_text (char *);
static int subset_of_spreadsheet (char *);
static int subset_of_generic (char *, char *);

generic_content_t g_generic_content_list[8] = {
  {.content_desc = "All Files and Folders",.subset_of =
   NULL,.subset_of_arg = NULL,.content_searchable = TRUE}
  ,
  {.content_desc = "All Files",.subset_of =
   NULL,.subset_of_arg = NULL,.content_searchable = TRUE}
  ,
  {.content_desc = "All Generic Text Files",.subset_of =
   NULL,.subset_of_arg = NULL,.content_searchable = TRUE}
  ,
  {.content_desc = "All Spreadsheets/WorkSheets",.subset_of =
   NULL,.subset_of_arg = NULL,.content_searchable = FALSE}
  ,
  {.content_desc = "All Videos",.subset_of =
   NULL,.subset_of_arg = "video/",.content_searchable = FALSE}
  ,
  {.content_desc = "All Audios",.subset_of =
   NULL,.subset_of_arg = "audio/",.content_searchable = FALSE}
  ,
  {.content_desc = "All Images",.subset_of =
   NULL,.subset_of_arg = "image/",.content_searchable = FALSE}
  ,
  {.content_desc = NULL,.subset_of = NULL,.subset_of_arg =
   NULL,.content_searchable = FALSE}
};


static void
generic_content_callbacks_init ()
{
  g_generic_content_list[0].subset_of = (subset_of_t) subset_of_all;
  g_generic_content_list[1].subset_of = (subset_of_t) subset_of_files;
  g_generic_content_list[2].subset_of = (subset_of_t) subset_of_generic_text;
  g_generic_content_list[3].subset_of = (subset_of_t) subset_of_spreadsheet;
  g_generic_content_list[4].subset_of = g_generic_content_list[5].subset_of =
    g_generic_content_list[6].subset_of = (subset_of_t) subset_of_generic;
  g_generic_content_list[7].subset_of = NULL;
}


GList *
create_mimetypes_files_list ()
{
  GList *mimetypes_files_list = NULL;
  char *mimetypes_file;
  asprintf (&mimetypes_file, "%s/.%s", getenv ("HOME"), MIMETYPES_FILE);
  mimetypes_files_list = g_list_append (mimetypes_files_list, mimetypes_file);

  asprintf (&mimetypes_file, "/etc/%s", MIMETYPES_FILE);
  mimetypes_files_list = g_list_append (mimetypes_files_list, mimetypes_file);

  return mimetypes_files_list;
}


FILE *
get_mimetypes_filep (GList * mimetypes_files_list)
{
  FILE *mimetypes_fp;
  while (mimetypes_files_list)
    {
      if ((mimetypes_fp = fopen (mimetypes_files_list->data, "r")) != NULL)
	return mimetypes_fp;

      mimetypes_files_list = g_list_next (mimetypes_files_list);
    }

  return NULL;
}


char *
get_mimetype_desc (FILE * mimetypes_fp, char *content_type)
{
  char *content_desc_prefix;
  if ((content_desc_prefix =
       g_content_type_get_description (content_type)) == NULL)
    return strdup (content_type);

  if (mimetypes_fp == NULL)
    {
      return content_desc_prefix;
    }

  char *mimetype_desc = content_desc_prefix;

  static char line[LINE_MAX + 1];
  char *lineptr = line, *lineptr_temp;
  char mimetype[LINE_MAX + 1];

  fseek (mimetypes_fp, 0, SEEK_SET);

  while ((lineptr = fgets (line, LINE_MAX, mimetypes_fp)) != NULL)
    {
      if (lineptr[0] == '#')
	goto next;

      if (!strncmp (lineptr, content_type, strlen (content_type)))
	{
	  lineptr += strlen (content_type);
	  if (*lineptr == '\n')
	    {
	      lineptr_temp = line;
	    }
	  else
	    {
	      if (*lineptr != '\n' && *lineptr != '\t')
		goto next;

	      while (*lineptr != '\n' && *lineptr == '\t')
		lineptr++;

	      lineptr_temp = lineptr;
	      while (*lineptr != '\n')
		{
		  if (*lineptr == ' ')
		    *lineptr = '/';
		  lineptr++;
		}

	    }

	  asprintf (&mimetype_desc, "%s [%.*s]", content_desc_prefix,
		    (int) (strlen (lineptr_temp) - 1), lineptr_temp);
	  free (content_desc_prefix);

	  return mimetype_desc;
	}

    next:
      lineptr = line;
    }

  asprintf (&mimetype_desc, "%s [%s]", content_desc_prefix, content_type);
  free (content_desc_prefix);

  return mimetype_desc;

}


static void
create_unsorted_content_table_from_list (gpointer contenttype_const,
					 gpointer mimetypes_fp_data)
{
  FILE *mimetypes_fp = mimetypes_fp_data;

  char *contenttype = contenttype_const;

  char *contentdesc = get_mimetype_desc (mimetypes_fp, contenttype);

  static guint content_table_index = 0;
  g_content_table[content_table_index].content_type = contenttype;
  g_content_table[content_table_index].content_desc = contentdesc;

  content_table_index++;
}


static int
content_table_sort_by_desc (const void *content1, const void *content2)
{
  return strcasecmp ((const char *) ((content_t *) content1)->content_desc,
		     (const char *) ((content_t *) content2)->content_desc);
}


int
content_table_get_len ()
{
  return g_content_table_len;
}


char *
content_table_get_content (char *content_desc)
{
  guint content_table_index;
  for (content_table_index = 0; content_table_index < g_content_table_len;
       content_table_index++)
    {
      if (!strcmp
	  (content_desc, g_content_table[content_table_index].content_desc))
	return g_content_table[content_table_index].content_type;
    }

  return NULL;
}


char *
content_table_get_description (char *content_type)
{
  guint content_table_index;
  for (content_table_index = 0; content_table_index < g_content_table_len;
       content_table_index++)
    {
      if (!strcmp
	  (content_type, g_content_table[content_table_index].content_type))
	return g_content_table[content_table_index].content_desc;
    }

  return NULL;
}


void
content_table_free ()
{
  free (g_content_table);
}


void
content_table_free_all ()
{
  guint content_table_index;
  for (content_table_index = 0; content_table_index < g_content_table_len;
       content_table_index++)
    {
      free (g_content_table[content_table_index].content_type);
      free (g_content_table[content_table_index].content_desc);
    }

  free (g_content_table);
}


static char *
content_desc_trim (char *content_desc)
{
  char *content_desc_end = str_rstr (content_desc, " [");
  size_t content_desc_trimmed_strlen =
    (content_desc_end !=
     NULL) ? content_desc_end - content_desc : strlen (content_desc);

  char *content_desc_trimmed;
  asprintf (&content_desc_trimmed, "%.*s",
	    (int) content_desc_trimmed_strlen, content_desc);

  return content_desc_trimmed;
}


int
content_type_pdf (char *content_type)
{
  return strcasecmp (content_type, "application/pdf");
}


static int
content_type_csv (char *content_type)
{
  return strcasecmp (content_type, "text/csv");
}


static int
content_type_html (char *content_type)
{
  return strcasecmp (content_type, "text/html");
}


static int
content_type_gnumeric (char *content_type)
{
  return strcasecmp (content_type, "application/x-gnumeric");
}


static int
content_type_applix (char *content_type)
{
  return strcasecmp (content_type, "application/x-applix-spreadsheet");
}


static int
content_type_gnu_oleo (char *content_type)
{
  return strcasecmp (content_type, "application/x-oleo");
}


static int
content_type_linear_integer_program (char *content_type)
{
  return strcasecmp (content_type, "application/x-mps");
}


static int
content_type_lotus_123 (char *content_type)
{
  return (strcasecmp (content_type, "application/vnd.lotus-1-2-3")
	  && strcasecmp (content_type, "application/x-123"));
}


static int
content_type_planperfect (char *content_type)
{
  return strcasecmp (content_type, "application/x-planperfect");
}


static int
content_type_quattro_pro (char *content_type)
{
  return (strcasecmp (content_type, "application/x-quattropro")
	  && strcasecmp (content_type, "application/x-quattro-pro"));
}


static int
content_type_sc_xspread (char *content_type)
{
  return strcasecmp (content_type, "application/x-sc");
}


static int
content_type_multiplan (char *content_type)
{
  return strcasecmp (content_type, "application/x-sylk");
}


static int
content_type_tsv (char *content_type)
{
  return strcasecmp (content_type, "text/tsv");
}


static int
content_type_xbase (char *content_type)
{
  return (strcasecmp (content_type, "application/dbase")
	  && strcasecmp (content_type, "application/dbf")
	  && strcasecmp (content_type, "application/x-dbase")
	  && strcasecmp (content_type, "application/x-dbf")
	  && strcasecmp (content_type, "application/x-xbase")
	  && strcasecmp (content_type, "zz-application/zz-winassoc-dbf"));
}


static int
content_type_excel_worksheet (char *content_type)
{
  int is_excel_worksheet = APP_FAILURE;

  char *content_desc;
  if ((content_desc = content_table_get_description (content_type)) != NULL)
    {
      char *content_desc_trimmed = content_desc_trim (content_desc);

      is_excel_worksheet =
	strcasecmp (content_desc_trimmed, "Microsoft Excel Worksheet")
	&& strcasecmp (content_desc_trimmed,
		       "Microsoft Excel Worksheet Template");
    }

  return is_excel_worksheet;
}


static int
content_type_opendocument_spreadsheet (char *content_type)
{
  int is_opendocument_spreadsheet = APP_FAILURE;

  char *content_desc;
  if ((content_desc = content_table_get_description (content_type)) != NULL)
    {
      char *content_desc_trimmed = content_desc_trim (content_desc);

      is_opendocument_spreadsheet =
	strcasecmp (content_desc_trimmed, "OpenDocument Spreadsheet")
	&& strcasecmp (content_desc_trimmed,
		       "OpenDocument Spreadsheet Template");
    }

  return is_opendocument_spreadsheet;
}


static int
content_type_libreoffice_spreadsheet (char *content_type)
{
  int is_libreoffice_spreadsheet = APP_FAILURE;

  char *content_desc;
  if ((content_desc = content_table_get_description (content_type)) != NULL)
    {
      char *content_desc_trimmed = content_desc_trim (content_desc);

      is_libreoffice_spreadsheet =
	strcasecmp (content_desc_trimmed, "LibreOffice Spreadsheet")
	&& strcasecmp (content_desc_trimmed,
		       "LibreOffice Spreadsheet Template");
    }

  return is_libreoffice_spreadsheet;
}


int
content_desc_in_generic_content_list (char *content_desc)
{
  generic_content_t *generic_content_list = g_generic_content_list;

  while (generic_content_list->content_desc != NULL)
    {
      if (!strcmp (content_desc, generic_content_list->content_desc))
	return APP_SUCCESS;
      generic_content_list++;
    }

  return APP_FAILURE;
}


static generic_content_t *
content_desc_get_generic_content (char *content_desc)
{
  generic_content_t *generic_content_list = g_generic_content_list;

  while (generic_content_list->content_desc != NULL)
    {
      if (!strcmp (content_desc, generic_content_list->content_desc))
	return generic_content_list;
      generic_content_list++;
    }

  return NULL;
}


static int
subset_of_all (char *content_type)
{
  return APP_SUCCESS;
}


static int
subset_of_files (char *content_type)
{
  return (strcmp (content_type, "inode/directory") == APP_SUCCESS);
}


static int
subset_of_generic (char *content_type, char *content_string)
{
  return (strncmp (content_type, content_string, strlen (content_string)));
}


static int
subset_of_spreadsheet (char *content_type)
{
  int is_a_subset_of = content_type_csv (content_type)
    && content_type_html (content_type)
    && content_type_gnumeric (content_type)
    && content_type_applix (content_type)
    && content_type_gnu_oleo (content_type)
    && content_type_linear_integer_program (content_type)
    && content_type_lotus_123 (content_type)
    && content_type_planperfect (content_type)
    && content_type_quattro_pro (content_type)
    && content_type_sc_xspread (content_type)
    && content_type_multiplan (content_type)
    && content_type_tsv (content_type)
    && content_type_xbase (content_type)
    && content_type_excel_worksheet (content_type)
    && content_type_opendocument_spreadsheet (content_type)
    && content_type_libreoffice_spreadsheet (content_type);

  return is_a_subset_of;
}


static int
subset_of_generic_text (char *content_type)
{
  int is_a_subset_of = APP_FAILURE;

  char *content_desc;
  if ((content_desc = content_table_get_description (content_type)) != NULL)
    {
      if (!STRING_IS_EMPTY (content_desc))
	{
	  char *content_desc_trimmed = content_desc_trim (content_desc);

	  if (strncmp (content_type, "text/", 5) == APP_SUCCESS
	      && subset_of_spreadsheet (content_type))
	    is_a_subset_of = APP_SUCCESS;
	  else if (strlen (content_desc_trimmed) == 6)
	    {
	      if (!strcasecmp (content_desc_trimmed, "script"))
		is_a_subset_of = APP_SUCCESS;
	    }
	  else if (strlen (content_desc_trimmed) > 6)
	    {
	      if (!strcasecmp (END (content_desc_trimmed) - 7, " script"))
		is_a_subset_of = APP_SUCCESS;
	    }

	  free (content_desc_trimmed);
	}
    }

  return is_a_subset_of;
}


int
content_desc_match_generic (char *content_type, char *content_desc)
{
  generic_content_t *generic_content =
    content_desc_get_generic_content (content_desc);

  if (generic_content->content_desc == NULL)
    return APP_FAILURE;

  return generic_content->subset_of
    (content_type, generic_content->subset_of_arg);
}


int
content_desc_generic_searchable (char *content_desc)
{
  generic_content_t *generic_content_list = g_generic_content_list;

  while (generic_content_list->content_desc != NULL)
    {
      if (!strcmp (content_desc, generic_content_list->content_desc)
	  && generic_content_list->content_searchable == TRUE)
	{
	  return APP_SUCCESS;
	}
      generic_content_list++;
    }

  return APP_FAILURE;
}


int
content_desc_text_generic_searchable (char *content_desc)
{
  char *content_type;
  if ((content_type = content_table_get_content (content_desc)) == NULL)
    return APP_FAILURE;

  return subset_of_generic_text (content_type);
}


int
content_desc_searchable (char *content_desc)
{
  int content_desc_is_searchable = APP_FAILURE;

  if (content_desc_in_generic_content_list (content_desc) == APP_SUCCESS)
    {
      if (content_desc_generic_searchable (content_desc) == APP_SUCCESS)
	content_desc_is_searchable = APP_SUCCESS;
    }
  else if (content_desc_text_generic_searchable (content_desc) == APP_SUCCESS)
    content_desc_is_searchable = APP_SUCCESS;
  else
    {
      char *content_type;
      if ((content_type = content_table_get_content (content_desc)) != NULL
	  && content_type_pdf (content_type) == APP_SUCCESS)
	content_desc_is_searchable = APP_SUCCESS;
    }

  return content_desc_is_searchable;
}


int
content_match (char *filename_content_type, char *search_name_content_type)
{
  return strcmp (filename_content_type, search_name_content_type);
}


int
executable (char *filename, char *content_type)
{
  int is_executable = APP_FAILURE;

  if (access (filename, X_OK) == APP_SUCCESS)
    {
      char *content_desc;
      if ((content_desc =
	   content_table_get_description (content_type)) != NULL)
	{
	  if (!STRING_IS_EMPTY (content_desc))
	    {
	      char *content_desc_trimmed = content_desc_trim (content_desc);

	      if (!strcasecmp (content_desc_trimmed, "executable"))
		{
		  is_executable = APP_SUCCESS;
		}
	      else if (strlen (content_desc_trimmed) == 6)
		{
		  if (!strcasecmp (content_desc_trimmed, "script"))
		    is_executable = APP_SUCCESS;
		}
	      else if (strlen (content_desc_trimmed) > 6)
		{
		  if (!strcasecmp (END (content_desc_trimmed) - 7, " script"))
		    is_executable = APP_SUCCESS;
		}

	      free (content_desc_trimmed);
	    }
	}

    }

  return is_executable;
}


GtkWidget *
setup_content_combo_box_text ()
{
  GtkWidget *content_combo_box_text = gtk_combo_box_text_new_with_entry ();

  GtkListStore *content_combo_box_text_entry_completion_store =
    gtk_list_store_new (1, G_TYPE_STRING);

  GtkTreeIter content_combo_box_text_entry_completion_list_iter;

  char *content_desc;
  generic_content_t *generic_content_list = g_generic_content_list;
  while ((content_desc = generic_content_list->content_desc) != NULL)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (content_combo_box_text), content_desc);

      gtk_list_store_append (content_combo_box_text_entry_completion_store,
			     &content_combo_box_text_entry_completion_list_iter);

      gtk_list_store_set (content_combo_box_text_entry_completion_store,
			  &content_combo_box_text_entry_completion_list_iter,
			  0, content_desc, -1);

      generic_content_list++;
    }

  GList *contenttype_list = NULL;
  if ((contenttype_list = g_content_types_get_registered ()) == NULL)
    goto return_combo;

  g_content_table_len = g_list_length (contenttype_list);
  g_content_table =
    (content_t *) malloc (sizeof (content_t) * g_content_table_len);

  GList *mimetypes_files_list = create_mimetypes_files_list ();

  FILE *mimetypes_fp;
  if ((mimetypes_fp = get_mimetypes_filep (mimetypes_files_list)) == NULL)
    {
      fprintf (stderr, "No Mime-Types File Found\n"
	       "File Type-based Search may be inaccurate!\n");
    }

  g_list_foreach (contenttype_list, create_unsorted_content_table_from_list,
		  mimetypes_fp);

  if (mimetypes_fp != NULL)
    {
      fclose (mimetypes_fp);
      g_list_free_full (mimetypes_files_list, free);
    }

  g_list_free (contenttype_list);

  qsort (g_content_table, g_content_table_len, sizeof (content_t),
	 content_table_sort_by_desc);

  guint content_table_index, content_table_diff = 0;
  for (content_table_index = 0; content_table_index < g_content_table_len;
       content_table_index++)
    {
      if (!strncasecmp
	  (g_content_table[content_table_index].content_type, "all/all", 7))
	{
	  free (g_content_table[content_table_index].content_type);
	  free (g_content_table[content_table_index].content_desc);
	  content_table_diff++;
	  continue;
	}

      if (content_table_diff != 0)
	{
	  g_content_table[content_table_index -
			  content_table_diff].content_type =
	    g_content_table[content_table_index].content_type;

	  g_content_table[content_table_index -
			  content_table_diff].content_desc =
	    g_content_table[content_table_index].content_desc;
	}

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT
				      (content_combo_box_text),
				      g_content_table
				      [content_table_index].content_desc);

      gtk_list_store_append (content_combo_box_text_entry_completion_store,
			     &content_combo_box_text_entry_completion_list_iter);
      gtk_list_store_set (content_combo_box_text_entry_completion_store,
			  &content_combo_box_text_entry_completion_list_iter,
			  0,
			  g_content_table[content_table_index].content_desc,
			  -1);
    }

  GtkEntryCompletion *content_combo_box_text_entry_completion =
    gtk_entry_completion_new ();
  gtk_entry_completion_set_text_column
    (content_combo_box_text_entry_completion, 0);
  gtk_entry_completion_set_inline_selection
    (content_combo_box_text_entry_completion, TRUE);

  gtk_entry_completion_set_model (content_combo_box_text_entry_completion,
				  GTK_TREE_MODEL
				  (content_combo_box_text_entry_completion_store));
  g_object_unref (content_combo_box_text_entry_completion_store);

  gtk_entry_set_completion (GTK_ENTRY
			    (gtk_bin_get_child
			     (GTK_BIN (content_combo_box_text))),
			    content_combo_box_text_entry_completion);


  g_content_table_len -= content_table_diff;
  generic_content_callbacks_init ();

return_combo:
  gtk_combo_box_set_active (GTK_COMBO_BOX (content_combo_box_text), 0);

  return content_combo_box_text;
}
