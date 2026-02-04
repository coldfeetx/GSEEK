// SPDX-License-Identifier: GPL-2.0-only
/*
 * shexp.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "gtk_common.h"
#include <wordexp.h>


void
shexp_print (wordexp_t expanded_wordv, FILE * fs)
{
  int expanded_wordv_len = expanded_wordv.we_wordc;

  int expanded_wordv_index;
  for (expanded_wordv_index = expanded_wordv.we_offs;
       expanded_wordv_index < expanded_wordv_len; expanded_wordv_index++)
    {
      char *word = expanded_wordv.we_wordv[expanded_wordv_index];

      fprintf (fs, "%s\n", word);
    }
}


int
shexp_expand (char *word, wordexp_t * expanded_wordv, int options)
{
  int shexp_return = 0;

  switch (shexp_return = wordexp (word, expanded_wordv, options))
    {

    case WRDE_BADCHAR:
      print_to_progress_statusbar
	("Word \"%s\" Expansion Failed: Contains an Unquoted Invalid Character!",
	 word);
      break;

    case WRDE_BADVAL:
      print_to_progress_statusbar
	("Word \"%s\" Expansion Failed: Contains an UnDefined Shell Variable!",
	 word);
      break;

    case WRDE_CMDSUB:
      print_to_progress_statusbar
	("Word \"%s\" Expansion Failed: Trying to use Command Substitution!",
	 word);
      break;

    case WRDE_SYNTAX:
      print_to_progress_statusbar
	("Word \"%s\" Expansion Failed: Syntax Error!", word);
      break;

    case WRDE_NOSPACE:
      print_to_progress_statusbar
	("Word \"%s\" Expansion Failed: Out of Space!", word);
      wordfree (expanded_wordv);
      break;

    case APP_SUCCESS:
      break;

    default:
      print_to_progress_statusbar
	("Word \"%s\" Expansion Failed: UnKnown Error!!", word);
    }

  return shexp_return;
}


int
shexp_match (char *iword, wordexp_t expanded_wordv)
{
  int expanded_wordv_len = expanded_wordv.we_wordc;

  int expanded_wordv_index;
  for (expanded_wordv_index = expanded_wordv.we_offs;
       expanded_wordv_index < expanded_wordv_len; expanded_wordv_index++)
    {
      char *word = expanded_wordv.we_wordv[expanded_wordv_index];

      if (!strcmp (iword, word))
	return APP_SUCCESS;
    }

  return APP_FAILURE;
}


void
shexp_free (wordexp_t * expanded_wordv)
{
  wordfree (expanded_wordv);
}
