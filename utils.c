// SPDX-License-Identifier: GPL-2.0-only
/*
 * utils.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"


GList *g_malloc_list = NULL;


void *
app_malloc (size_t size)
{
  void *ptr;
  if ((ptr = malloc (size)) != NULL)
    g_malloc_list = g_list_append (g_malloc_list, ptr);

  return ptr;
}


char *
app_strdup (char *string)
{
  char *new_string;
  if ((new_string = strdup (string)) != NULL)
    g_malloc_list = g_list_append (g_malloc_list, new_string);

  return new_string;
}


char *
app_strndup (char *string, size_t size)
{
  char *new_string;
  if ((new_string = strndup (string, size)) != NULL)
    g_malloc_list = g_list_append (g_malloc_list, new_string);

  return new_string;
}


void
app_free (void *ptr)
{
  if (ptr != NULL && g_list_find (g_malloc_list, ptr) != NULL)
    {
      g_malloc_list = g_list_remove (g_malloc_list, ptr);
      free (ptr);
    }
}


void
app_free_all ()
{
  g_list_free_full (g_malloc_list, free);
}


void
app_sleep (unsigned long int seconds)
{
  g_usleep (1000000 * seconds);
}


char *
str_subchr (char *source, int from, int to)
{
  char *orig_source = source;
  while (*source != '\0')
    {
      if (*source == from)
	*source = to;
      source++;
    }

  return orig_source;
}


char *
str_toupper (char *source, int start, int end)
{
  char *orig_source = source;

  int curr = start;
  while (source[curr] != '\0')
    {
      if (end != -1 && curr > end)
	break;

      source[curr] = toupper (source[curr]);
      curr++;
    }

  return orig_source;
}


char *
str_rstr (char *string, char *pattern)
{
  char *string_matched_pattern = NULL, *string_matched_pattern_prev = NULL;

  while ((string_matched_pattern = strstr (string, pattern)) != NULL)
    {
      string_matched_pattern_prev = string_matched_pattern;

      string = string_matched_pattern + strlen (pattern);
    }

  return string_matched_pattern_prev;
}


char *
str_caserstr (char *string, char *pattern)
{
  char *string_matched_pattern = NULL, *string_matched_pattern_prev = NULL;

  while ((string_matched_pattern = strcasestr (string, pattern)) != NULL)
    {
      string_matched_pattern_prev = string_matched_pattern;

      string = string_matched_pattern + strlen (pattern);
    }

  return string_matched_pattern_prev;
}


char *
get_date ()
{
  char *time_str = app_malloc (8 + 1);
  time_t timet;
  time (&timet);

  strftime (time_str, 9, "%d/%m/%g", localtime (&timet));

  return time_str;
}

char *
get_time (time_t * timet)
{
  char *time_str = app_malloc (8 + 1);

  strftime (time_str, 9, "%d/%m/%g", localtime (timet));

  return time_str;
}


char *
get_tm_to_time (struct tm *timem, const char *format)
{
  time_t timet = mktime (timem);

  char *time_str = app_malloc (strlen (format) + 1);
  strftime (time_str, strlen (format) + 1, format, localtime (&timet));

  return time_str;
}


static void
swap_elements (char *string, size_t index_a, size_t index_b)
{
  size_t temp = string[index_b];
  string[index_b] = string[index_a];
  string[index_a] = temp;
}


char *
flip_date (char *date)
{
  char *flipped_date = app_strdup (date);

  swap_elements (flipped_date, 0, 6);
  swap_elements (flipped_date, 1, 7);

  return flipped_date;
}
