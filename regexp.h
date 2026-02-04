// SPDX-License-Identifier: GPL-2.0-only
/*
 * regexp.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include <regex.h>

#define REGEXP_FAST 0
#define REGEXP_NORMAL 1

typedef struct regexp_node
{
  char *regexp;
  size_t regexp_start_index;
  size_t regexp_end_index;
} regexp_node_t;

typedef struct regexp_list
{
  regexp_node_t *regexp_nodes;
  int regexps;
  int regexp_error;
} regexp_list_t;


char *regexp_regerror (int, regex_t *);

char *regexp_comp (regex_t *, char *, int);

regexp_list_t *regexp_exec (regex_t *, char *, int, int);

int regexp_exec_matched (regex_t *, char *, int);

void regexp_free (regexp_list_t *);

void regexp_free_all (regexp_list_t *);
