// SPDX-License-Identifier: GPL-2.0-only
/*
 * filematches.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

typedef struct filematches
{
  int content;
  char *content_text;
  int ccasei;
  int cregexp;
  regex_t cregexp_preg;
} filematches_t;
