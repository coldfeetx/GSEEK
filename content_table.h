// SPDX-License-Identifier: GPL-2.0-only
/*
 * content_table.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

typedef struct content
{
  char *content_type;
  char *content_desc;
  struct content *content_next;
} content_t;


typedef int (*subset_of_t) (char *, ...);

typedef struct generic_content
{
  char *content_desc;
  subset_of_t subset_of;
  char *subset_of_arg;
  int content_searchable;
} generic_content_t;
