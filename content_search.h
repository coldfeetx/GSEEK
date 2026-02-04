// SPDX-License-Identifier: GPL-2.0-only
/*
 * content_search.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include <poppler.h>
#include "parse_contents.h"


typedef enum search_stream_type
{
  SEARCH_STREAM_TYPE_TEXT_GENERIC,
  SEARCH_STREAM_TYPE_PDF,
} search_stream_type;


typedef struct search_stream_text
{
  FILE *fp;
  size_t line;
} search_stream_text_obj;


typedef struct search_stream_pdf
{
  PopplerDocument *pdf;
  size_t pages;
  size_t page;
  size_t line;
  char *content;
  char *content_next;
  int to_be_freed;
} search_stream_pdf_obj;


typedef union search_stream_content
{
  search_stream_text_obj stream_text_obj;
  search_stream_pdf_obj stream_pdf_obj;
} search_stream_content_obj;


typedef struct search_stream
{
  search_stream_type stream_type;
  search_stream_content_obj stream_content_obj;
  char stream_content_next[LINE_MAX + 1];
  char stream_error[LINE_MAX + 1];
} search_stream_obj;


extern GtkWidget *g_app_window;

search_stream_obj *search_stream_init (char *);

char *search_stream_get_content_next (search_stream_obj *);

char *search_stream_get_pos_formatted (search_stream_obj *);

void search_stream_cleanup (search_stream_obj *);
