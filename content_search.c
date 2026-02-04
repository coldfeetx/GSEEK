// SPDX-License-Identifier: GPL-2.0-only
/*
 * content_search.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "utils.h"
#include "gtk_common.h"
#include "content_search.h"


static char *
get_password_for_pdf (char *file)
{
  GtkWidget *pdfpasswd_dialog = gtk_dialog_new_with_buttons ("Enter Password",
							     GTK_WINDOW
							     (g_app_window),
							     GTK_DIALOG_MODAL
							     |
							     GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_STOCK_OK,
							     GTK_RESPONSE_OK,
							     GTK_STOCK_CANCEL,
							     GTK_RESPONSE_REJECT,
							     NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (pdfpasswd_dialog),
				   GTK_RESPONSE_OK);

  char *pdfpasswd_query;
  asprintf (&pdfpasswd_query, "Unlock PDF File \"%s\"", file);
  GtkWidget *pdfpasswd_query_label = gtk_label_new (pdfpasswd_query);
  free (pdfpasswd_query);

  GtkWidget *pdfpasswd_label = gtk_label_new_with_mnemonic ("_Password:");

  GtkWidget *pdfpasswd_entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (pdfpasswd_entry), FALSE);
  gtk_entry_set_invisible_char (GTK_ENTRY (pdfpasswd_entry), '*');

  gtk_label_set_mnemonic_widget (GTK_LABEL (pdfpasswd_label),
				 pdfpasswd_entry);

  GtkWidget *pdfpasswd_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start (GTK_BOX (pdfpasswd_hbox), pdfpasswd_label, FALSE,
		      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pdfpasswd_hbox), pdfpasswd_entry, TRUE, TRUE,
		      0);

  GtkWidget *pdfpasswd_main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (pdfpasswd_main_vbox), pdfpasswd_query_label,
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pdfpasswd_main_vbox), pdfpasswd_hbox, TRUE,
		      TRUE, 0);

  gtk_widget_show_all (pdfpasswd_main_vbox);

  GtkWidget *pdfpasswd_dialog_content_area =
    gtk_dialog_get_content_area (GTK_DIALOG (pdfpasswd_dialog));

  gtk_container_add (GTK_CONTAINER (pdfpasswd_dialog_content_area),
		     pdfpasswd_main_vbox);


  gint pdfpasswd_dialog_response =
    gtk_dialog_run (GTK_DIALOG (pdfpasswd_dialog));

  gtk_widget_hide (pdfpasswd_dialog);

  char *password = NULL;
  if (pdfpasswd_dialog_response == GTK_RESPONSE_OK)
    {
      password =
	app_strdup ((char *)
		    gtk_entry_get_text (GTK_ENTRY (pdfpasswd_entry)));
    }

  gtk_widget_destroy (pdfpasswd_dialog);

  return password;
}


search_stream_obj *
search_stream_init (char *file)
{
  search_stream_obj *stream_obj =
    (search_stream_obj *) app_malloc (sizeof (search_stream_obj));

  memset (stream_obj, '\0', sizeof (search_stream_obj));

  char *content_type;
  if ((content_type = get_content_type (file)) == NULL)
    {
      sprintf (stream_obj->stream_error,
	       "Failed to Retrieve Content Type of File \"%s\"", file);
      goto return_stream;
    }


  char *content_desc = content_table_get_description (content_type);

  int is_content_type_pdf = content_type_pdf (content_type);
  app_free (content_type);

  if (is_content_type_pdf == APP_SUCCESS)
    {
      stream_obj->stream_type = SEARCH_STREAM_TYPE_PDF;

      GFile *gfile;
      if ((gfile = g_file_new_for_path (file)) == NULL)
	{
	  sprintf (stream_obj->stream_error,
		   "Failed to Retrieve GFile for File \"%s\"", file);
	  goto return_stream;
	}

      int password_authenticated = 0;
      char *password = NULL;
      GError *g_error = NULL;

    authenticate_password:
      if ((stream_obj->stream_content_obj.stream_pdf_obj.pdf =
	   poppler_document_new_from_file (g_file_get_uri (gfile), password,
					   &g_error)) == NULL)
	{
	  if (!password_authenticated
	      && g_error->domain == POPPLER_ERROR
	      && !strcasecmp (g_error->message, "Document is encrypted"))
	    {

	      password = get_password_for_pdf (file);
	      if (password != NULL)
		{

		  g_clear_error (&g_error);
		  password_authenticated = 1;

		  goto authenticate_password;
		}
	      else
		{
		  sprintf (stream_obj->stream_error,
			   "Invalid Password Entered for PDF File: \"%s\"",
			   file);
		}
	    }
	  else
	    {
	      sprintf (stream_obj->stream_error,
		       "Failed to Open PDF Stream for File \"%s\": %s", file,
		       g_error->message);
	    }

	  g_clear_error (&g_error);
	  goto return_pdf_stream;
	}
      else
	{
	  if (password != NULL)
	    app_free (password);
	  stream_obj->stream_content_obj.stream_pdf_obj.pages =
	    poppler_document_get_n_pages (stream_obj->
					  stream_content_obj.stream_pdf_obj.
					  pdf);
	  stream_obj->stream_content_obj.stream_pdf_obj.content =
	    stream_obj->stream_content_obj.stream_pdf_obj.content_next = "";
	}

    return_pdf_stream:
      if (gfile != NULL)
	g_object_unref (gfile);

    }
  else if (content_desc_text_generic_searchable (content_desc) == APP_SUCCESS)
    {
      stream_obj->stream_type = SEARCH_STREAM_TYPE_TEXT_GENERIC;

      errno = 0;
      if ((stream_obj->stream_content_obj.stream_text_obj.fp =
	   fopen (file, "r")) == NULL)
	{
	  sprintf (stream_obj->stream_error,
		   "fopen() Failed on File \"%s\": %s", file,
		   strerror (errno));
	  goto return_stream;
	}
    }

return_stream:
  return stream_obj;
}


char *
search_stream_get_content_next (search_stream_obj * stream_obj)
{
  char *stream_content_next = NULL;

  if (stream_obj->stream_type == SEARCH_STREAM_TYPE_PDF)
    {
      if (*stream_obj->stream_content_obj.stream_pdf_obj.content_next == '\0')
	{
	  if (strlen
	      (stream_obj->stream_content_obj.stream_pdf_obj.content) > 0)
	    free (stream_obj->stream_content_obj.stream_pdf_obj.content);
	  stream_obj->stream_content_obj.stream_pdf_obj.to_be_freed = FALSE;


	  if (stream_obj->stream_content_obj.stream_pdf_obj.page <
	      stream_obj->stream_content_obj.stream_pdf_obj.pages)
	    {
	      stream_obj->stream_content_obj.stream_pdf_obj.content_next =
		stream_obj->stream_content_obj.stream_pdf_obj.content =
		poppler_page_get_text (poppler_document_get_page
				       (stream_obj->
					stream_content_obj.stream_pdf_obj.pdf,
					stream_obj->
					stream_content_obj.stream_pdf_obj.
					page));

	      stream_obj->stream_content_obj.stream_pdf_obj.line = 0;
	      stream_obj->stream_content_obj.stream_pdf_obj.page += 1;
	      stream_obj->stream_content_obj.stream_pdf_obj.to_be_freed =
		TRUE;
	    }
	  else
	    return NULL;
	}

      char *stream_content_end =
	strpbrk (stream_obj->stream_content_obj.stream_pdf_obj.content_next,
		 "\n");
      if (stream_content_end == NULL)
	stream_content_end =
	  stream_obj->stream_content_obj.stream_pdf_obj.content_next +
	  strlen (stream_obj->stream_content_obj.stream_pdf_obj.content_next);

      sprintf (stream_obj->stream_content_next, "%.*s",
	       (int) (stream_content_end -
		      stream_obj->stream_content_obj.stream_pdf_obj.
		      content_next),
	       stream_obj->stream_content_obj.stream_pdf_obj.content_next);

      stream_content_next = stream_obj->stream_content_next;

      size_t stream_content_next_inc = 0;
      if (*stream_content_end == '\n')
	stream_content_next_inc = 1;

      stream_obj->stream_content_obj.stream_pdf_obj.content_next =
	stream_content_end + stream_content_next_inc;

      stream_obj->stream_content_obj.stream_pdf_obj.line += 1;
    }
  else if (stream_obj->stream_type == SEARCH_STREAM_TYPE_TEXT_GENERIC)
    {
      if (fgets
	  (stream_obj->stream_content_next, LINE_MAX,
	   stream_obj->stream_content_obj.stream_text_obj.fp) == NULL)
	{
	  return NULL;
	}

      stream_obj->stream_content_next[strlen (stream_obj->stream_content_next)
				      - 1] = '\0';

      stream_obj->stream_content_obj.stream_text_obj.line += 1;

      stream_content_next = stream_obj->stream_content_next;
    }

  return stream_content_next;
}


char *
search_stream_get_pos_formatted (search_stream_obj * stream_obj)
{
  char *stream_pos_formatted = NULL;

  if (stream_obj->stream_type == SEARCH_STREAM_TYPE_PDF)
    {

      asprintf (&stream_pos_formatted,
		"\n\nPage Number:%zd  Line Number:%zd\n\t",
		stream_obj->stream_content_obj.stream_pdf_obj.page,
		stream_obj->stream_content_obj.stream_pdf_obj.line);
    }
  else if (stream_obj->stream_type == SEARCH_STREAM_TYPE_TEXT_GENERIC)
    {

      asprintf (&stream_pos_formatted, "\n\nLine Number:%zd\n\t",
		stream_obj->stream_content_obj.stream_text_obj.line);
    }

  return stream_pos_formatted;
}


void
search_stream_cleanup (search_stream_obj * stream_obj)
{
  if (stream_obj->stream_type == SEARCH_STREAM_TYPE_PDF)
    {
      if (stream_obj->stream_content_obj.stream_pdf_obj.to_be_freed == TRUE)
	{
	  free (stream_obj->stream_content_obj.stream_pdf_obj.content);
	}

      g_object_unref (stream_obj->stream_content_obj.stream_pdf_obj.pdf);
    }
  else if (stream_obj->stream_type == SEARCH_STREAM_TYPE_TEXT_GENERIC)
    {

      fclose (stream_obj->stream_content_obj.stream_text_obj.fp);
    }

  app_free (stream_obj);
}
