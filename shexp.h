// SPDX-License-Identifier: GPL-2.0-only
/*
 * shexp.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include <wordexp.h>

int shexp_expand (char *, wordexp_t *, int);

int shexp_match (char *, wordexp_t);

void shexp_free ();

void shexp_print (wordexp_t, FILE *);
