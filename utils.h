// SPDX-License-Identifier: GPL-2.0-only
/*
 * utils.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

void *app_malloc (size_t);

char *app_strdup (char *);

char *app_strndup (char *, size_t);

void app_free (void *);

void app_free_all (void);

void app_sleep (unsigned long int);

char *str_subchr (char *, int, int);

char *str_toupper (char *, int, int);

char *str_rstr (char *, char *);

char *str_caserstr (char *, char *);

char *get_date (void);

char *get_time (time_t *);

char *get_tm_to_time (struct tm *, const char *);

char *flip_date (char *);
