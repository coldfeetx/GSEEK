// SPDX-License-Identifier: GPL-2.0-only
/*
 * regexp.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "gtk_common.h"
#include "regexp.h"


char *
regexp_regerror (int regexp_err, regex_t * compiled)
{
  size_t regexp_err_buf_len = regerror (regexp_err, compiled, NULL, 0);
  char *regexp_err_buf = malloc (regexp_err_buf_len);
  (void) regerror (regexp_err, compiled, regexp_err_buf, regexp_err_buf_len);

  return regexp_err_buf;
}


char *
regexp_comp (regex_t * preg, char *pattern, int regcomp_options)
{
  char *regexp_comp_error = NULL;
  int regexp_comp_err;

  if ((regexp_comp_err = regcomp (preg, pattern, regcomp_options)))
    {
      print_to_progress_statusbar ("regcomp() failed on Pattern \"%s\"",
				   pattern);
      regexp_comp_error = regexp_regerror (regexp_comp_err, preg);
    }

  return regexp_comp_error;
}


regexp_list_t *
regexp_exec (regex_t * preg, char *string, int regexec_options,
	     int regexp_options)
{
  int regexp_err, subexp_index, regexp_error = APP_SUCCESS;
  size_t nmatch, matches;
  regmatch_t *pmatch;

  regexp_node_t *regexp_nodes = NULL;
  regexp_list_t *regexp_list =
    (regexp_list_t *) malloc (sizeof (regexp_list));

  matches = nmatch = preg->re_nsub + 1;

  if (regexp_options == REGEXP_FAST)
    matches = 1;

  pmatch = malloc (sizeof (regmatch_t) * nmatch);

  if (!(regexp_err = regexec (preg, string, nmatch, pmatch, regexec_options)))
    {
      regexp_nodes =
	(regexp_node_t *) malloc ((nmatch + 1) * sizeof (regexp_node_t));

      for (subexp_index = 0; subexp_index < matches; subexp_index++)
	{
	  regexp_nodes[subexp_index].regexp_start_index =
	    pmatch[subexp_index].rm_so;
	  regexp_nodes[subexp_index].regexp_end_index =
	    pmatch[subexp_index].rm_eo - 1;
	  regexp_nodes[subexp_index].regexp =
	    malloc (pmatch[subexp_index].rm_eo - pmatch[subexp_index].rm_so +
		    1);
	  sprintf (regexp_nodes[subexp_index].regexp, "%.*s",
		   pmatch[subexp_index].rm_eo - pmatch[subexp_index].rm_so,
		   &string[pmatch[subexp_index].rm_so]);
	}
    }
  else
    {
      regexp_error = APP_FAILURE;
    }
  free (pmatch);

return_back:
  regexp_list->regexp_nodes = regexp_nodes;
  regexp_list->regexps = matches;
  regexp_list->regexp_error = regexp_error;
  return regexp_list;
}


int
regexp_exec_matched (regex_t * preg, char *string, int regexec_options)
{
  size_t nmatch;
  regmatch_t *pmatch;

  nmatch = preg->re_nsub + 1;
  pmatch = malloc (sizeof (regmatch_t) * nmatch);

  int regexp_error = regexec (preg, string, nmatch, pmatch, regexec_options);
  free (pmatch);

  return regexp_error;
}


void
regexp_free (regexp_list_t * regexp_list)
{
  if (regexp_list != NULL)
    free (regexp_list);
}


static void
regexp_free_nodes (regexp_list_t * regexp_list)
{
  regexp_node_t *subexp_nodes = regexp_list->regexp_nodes;

  size_t subexp_index, subexps = regexp_list->regexps;
  for (subexp_index = 0; subexp_index < subexps; subexp_index++)
    {
      free (subexp_nodes[subexp_index].regexp);
    }

  free (subexp_nodes);
}


void
regexp_free_all (regexp_list_t * regexp_list)
{
  if (regexp_list->regexp_error == APP_SUCCESS)
    regexp_free_nodes (regexp_list);

  regexp_free (regexp_list);
}
