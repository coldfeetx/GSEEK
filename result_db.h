// SPDX-License-Identifier: GPL-2.0-only
/*
 * result_db.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

enum
{
  FILE_ICON,
  FILE_NAME,
  FILE_SUBFOLDER,
  FILE_SIZE,
  FILE_MODIFIED,
  FILE_MEMBERSHIP,
  FILE_PERM,
  COLUMNS,
};


typedef struct search_result
{
  char *file_content;
  char *file_name;
  char *file_subfolder;
  char *file_size;
  char *file_modified;
  char *file_membership;
  char *file_perm;
} search_result_t;
