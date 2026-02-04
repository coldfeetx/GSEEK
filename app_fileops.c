// SPDX-License-Identifier: GPL-2.0-only
/*
 * app_fileops.c
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "common.h"
#include "utils.h"
#include "gtk_common.h"
#include "app_externs.h"
#include "nftw.h"
#include "content_search.h"


extern search_settings_t g_search_settings;
extern char g_search_error_str[LINE_MAX];
extern char *g_sdirname;
extern unsigned long int g_file_search_count_tot, g_file_search_count,
  g_file_match_count;
extern int g_app_stopped;

int g_search_error;
char *g_date_pattern = "%d/%m/%g %I:%M %p";
char *g_date_pattern_rev = "%d/%m/%y %I:%M %p";
char *g_date_sort_pattern = "%y/%m/%d %H:%M";

char *g_size_units[5] = {
  "B",
  "KB",
  "MB",
  "GB",
};

int g_size_units_len = 4;
char *g_search_location = NULL;

static void
refresh_app ()
{
  print_to_progress_statusbar ("Search Progress: %d/%d Found",
			       g_file_match_count, g_file_search_count_tot);

  if (g_file_search_count == REFRESH_RESULT_RATE)
    {
      refresh_gtk ();
      g_file_search_count = 0;
    }
  else
    g_file_search_count++;
}


static void
app_free_search_result (search_result_t * search_result)
{
  if (search_result->file_content != NULL)
    app_free (search_result->file_content);
  if (search_result->file_name != NULL)
    app_free (search_result->file_name);
  if (search_result->file_subfolder != NULL)
    app_free (search_result->file_subfolder);
  if (search_result->file_size != NULL)
    app_free (search_result->file_size);
  if (search_result->file_modified != NULL)
    app_free (search_result->file_modified);
  if (search_result->file_membership != NULL)
    app_free (search_result->file_membership);
  if (search_result->file_perm != NULL)
    app_free (search_result->file_perm);
}


static char *
format_size (off_t size)
{
  float size_fact = 1024, size_iter = 1024, sizeu = 0;
  char *sized;

  int size_units_index;
  for (size_units_index = 0; size_units_index < g_size_units_len;
       size_units_index++)
    {
      if (size < size_iter)
	{
	  sized = g_size_units[size_units_index];

	  sizeu = size / (size_iter / size_fact);
	  break;
	}

      size_iter *= size_fact;
    }

  if (sized == NULL)
    return NULL;
  char *fsize = app_malloc (4 + strlen (sized) + 1);

  char *size_format;
  if (!strcmp (sized, "B"))
    size_format = "%.0f %s";
  else
    size_format = "%.2f %s";
  sprintf (fsize, size_format, sizeu, sized);

  return fsize;
}


static char *
format_time (time_t * uf_time, char *date_pattern)
{
  int ftime_len = strlen (date_pattern) + 1;
  char *ftime = app_malloc (ftime_len);
  strftime (ftime, ftime_len, date_pattern, localtime (uf_time));

  return ftime;
}


static char *
format_usergroup (uid_t uid, gid_t gid)
{
  struct passwd *pass;
  char *user, *group;

  errno = 0;
  if ((pass = getpwuid (uid)) == NULL)
    {
      g_search_error = errno;

      char userbuf[100];
      sprintf (userbuf, "%ld", (long) uid);
      user = userbuf;
    }
  else
    {
      user = pass->pw_name;
    }

  struct group *grp;
  errno = 0;
  if ((grp = getgrgid (gid)) == NULL)
    {
      g_search_error = errno;

      char groupbuf[100];
      sprintf (groupbuf, "%ld", (long) gid);
      group = groupbuf;
    }
  else
    {
      group = grp->gr_name;
    }

  char *fusergroup = app_malloc (strlen (user) + strlen (group) + 2);
  sprintf (fusergroup, "%s/%s", user, group);

  return fusergroup;
}


static char *
format_perm (mode_t mode)
{
  char *fperm = app_malloc (9 + 1);

  sprintf (fperm, "%c%c%c%c%c%c%c%c%c",
	   (mode & S_IRUSR) ? 'r' : '-',
	   (mode & S_IWUSR) ? 'w' : '-',
	   (mode & S_IXUSR) ?
	   ((mode & S_ISUID) ? 's' : 'x') : ((mode & S_ISUID) ? 'S' : '-'),
	   (mode & S_IRGRP) ? 'r' : '-',
	   (mode & S_IWGRP) ? 'w' : '-',
	   (mode & S_IXGRP) ?
	   ((mode & S_ISGID) ? 's' : 'x') : ((mode & S_ISGID) ? 'S' : '-'),
	   (mode & S_IROTH) ? 'r' : '-',
	   (mode & S_IWOTH) ? 'w' : '-',
	   (mode & S_IXOTH) ?
	   ((mode & S_ISVTX) ? 't' : 'x') : ((mode & S_ISVTX) ? 'T' : '-'));

  return fperm;
};


static void
append_result_table (char *content_type, char *filename, char *subfolder,
		     struct stat *sbuf, int type)
{
  search_result_t search_result;
  memset (&search_result, 0, sizeof (search_result));

  search_result.file_content = content_type;

  search_result.file_name = filename;

  search_result.file_subfolder = subfolder;

  if (type == FTW_DNR || type == FTW_NS)
    goto append_result_list;

  search_result.file_size = format_size (sbuf->st_size);

  search_result.file_modified = format_time (&sbuf->st_mtime, g_date_pattern);

  search_result.file_membership =
    format_usergroup (sbuf->st_uid, sbuf->st_gid);

  search_result.file_perm = format_perm (sbuf->st_mode);

append_result_list:
  if (search_result.file_subfolder == NULL)
    {
      search_result.file_subfolder = app_strdup ("-");

      sprintf (END (g_search_error_str),
	       "File \"%s\": SubFolder Retrieval Failed",
	       search_result.file_name);
    }

  if (search_result.file_size == NULL)
    {
      search_result.file_size = app_strdup ("-");

      sprintf (END (g_search_error_str), "File \"%s\": Size Retrieval Failed",
	       search_result.file_name);
    }

  if (isdigit (search_result.file_membership[0]))
    {
      sprintf (END (g_search_error_str),
	       "File \"%s\": Membership Retrieval Failed from System Account Database: %s",
	       search_result.file_name, strerror (g_search_error));
    }

  print_to_progress_statusbar ("Found File %lu/%lu:\t\t%s",
			       ++g_file_match_count, g_file_search_count_tot,
			       search_result.file_name);

  append_result_window (g_result_store, &search_result, g_result_sortable,
			FILE_NAME, GTK_SORT_ASCENDING);

  app_free_search_result (&search_result);
}


static char *
get_subdirname (char *pathname, int start, int end)
{

  if (start == end - 1)
    return app_strdup ("");
  start++;
  return app_strndup (&pathname[start], end - start);
}


static char *
get_dirname (char *pathname, int base)
{
  return app_strndup (pathname, base - 1);
}


int
verify_ctext (char *pathname, char *text, int casei)
{
  search_stream_obj *stream_obj = search_stream_init (pathname);
  if (strlen (stream_obj->stream_error) > 0)
    {
      print_to_progress_statusbar (stream_obj->stream_error);
      return APP_FAILURE;
    }

  char *(*generic_strstr) (const char *, const char *);
  if (casei)
    generic_strstr = strstr;
  else
    {
      generic_strstr = strcasestr;
    }

  int ctext_result = APP_FAILURE;
  char *filebuf;
  while ((filebuf = search_stream_get_content_next (stream_obj)) != NULL)
    {
      if (generic_strstr (filebuf, text) != NULL)
	{
	  ctext_result = APP_SUCCESS;
	  break;
	}
    }
  search_stream_cleanup (stream_obj);

  return ctext_result;
}


int
verify_cregexp (char *pathname, regex_t cregexp_preg)
{
  search_stream_obj *stream_obj = search_stream_init (pathname);
  if (strlen (stream_obj->stream_error) > 0)
    {
      print_to_progress_statusbar (stream_obj->stream_error);
      return APP_FAILURE;
    }

  int cregexp_result = APP_FAILURE;
  char *filebuf;
  while ((filebuf = search_stream_get_content_next (stream_obj)) != NULL)
    {
      if (regexp_exec_matched (&cregexp_preg, filebuf, 0) == APP_SUCCESS)
	{
	  cregexp_result = APP_SUCCESS;
	  break;
	}

    }
  search_stream_cleanup (stream_obj);

  return cregexp_result;
}


int
verify_time_between (char *lclock, char *clock, char *rclock)
{
  if (strcmp (lclock, clock) > 0 || strcmp (clock, rclock) > 0)
    return APP_FAILURE;

  return APP_SUCCESS;
}


int
verify_time_during (int search_name_time_range, int search_name_time_stamp,
		    time_t filename_timet)
{
  double search_name_sectime =
    search_name_time_range * g_clock_scales[search_name_time_stamp].secs;

  time_t curr_timet;
  time (&curr_timet);

  double filename_diff_sectime = difftime (curr_timet, filename_timet);

  if (filename_diff_sectime > search_name_sectime)
    return APP_FAILURE;

  return APP_SUCCESS;
}


int
verify_permissions (char *search_name_permissions, mode_t filename_perm)
{
  char *filename_permissions = format_perm (filename_perm);

  int permissions_index;
  for (permissions_index = 0; permissions_index < 9; permissions_index++)
    {
      if (search_name_permissions[permissions_index] != ' ')
	{
	  if (filename_permissions[permissions_index] !=
	      search_name_permissions[permissions_index])
	    {
	      app_free (filename_permissions);
	      return APP_FAILURE;
	    }
	}

    }

  return APP_SUCCESS;
}


int
Search (const char *c_pathname, const struct stat *c_sbuf, int type,
	struct FTW *ftwbuf)
{

  refresh_app ();

  if (g_app_stopped)
    return FTW_STOP;

  int level = ftwbuf->level;
  if (level == 0)
    goto continue_search;

  g_file_search_count_tot++;


  char *pathname = (char *) c_pathname;
  struct stat *sbuf = (struct stat *) c_sbuf;

  char *filename = app_strdup (&pathname[ftwbuf->base]);

  char *search_name = g_search_settings.name;
  char *search_location = g_search_settings.location;
  g_search_location = strcmp (search_location, "/") ? search_location : "";

  char *dirname = get_dirname (pathname, ftwbuf->base);

  char *subdirname =
    get_subdirname (pathname, strlen (g_search_location), ftwbuf->base);


  if (!g_search_settings.hidden)
    {
      if (filename[0] == '.')
	{
	  if (type == FTW_D)
	    return FTW_SKIP_SUBTREE;

	  goto continue_search;
	}
    }


  if (g_search_settings.regexp)
    {
      if (regexp_exec_matched (&g_search_settings.regexp_preg, filename, 0) !=
	  APP_SUCCESS)
	goto continue_search;
    }
  else if (g_search_settings.shexp)
    {
      if (g_sdirname == NULL || strcmp (dirname, g_sdirname))
	{
	  if (g_sdirname != NULL)
	    {
	      app_free (g_sdirname);
	      if (g_search_settings.shexp_tofree)
		shexp_free (&g_search_settings.shexp_wordv);
	    }

	  g_sdirname = dirname;
	  char *cdir = get_current_dir_name ();
	  chdir (g_sdirname);

	  int shexp_result =
	    shexp_expand (search_name, &g_search_settings.shexp_wordv,
			  WRDE_UNDEF);

	  chdir (cdir);
	  if (shexp_result != APP_SUCCESS)
	    {
	      g_search_settings.shexp_tofree = FALSE;
	      print_to_progress_statusbar
		("File Search for File \"%s\"Failed", search_name);
	      return FTW_SKIP_SIBLINGS;
	    }
	}

      if (shexp_match (filename, g_search_settings.shexp_wordv) !=
	  APP_SUCCESS)
	{
	  goto continue_search;
	}
    }
  else
    {
      if (STRING_IS_EMPTY (search_name))
	goto match_type;

      if (g_search_settings.casei)
	{
	  if (strcmp (filename, search_name))
	    goto continue_search;
	}
      else
	{
	  if (strcasecmp (filename, search_name))
	    goto continue_search;
	}
    }


match_type:
  ;
  char *filename_content_type;
  if ((filename_content_type = get_content_type (pathname)) == NULL)
    goto continue_search;

  char *search_name_content_type = g_search_settings.content_type;

  int content_cmp;
  if (g_search_settings.content_generic)
    {
      content_cmp =
	content_desc_match_generic (filename_content_type,
				    search_name_content_type);
    }
  else
    {
      content_cmp =
	content_match (filename_content_type, search_name_content_type);
    }

  app_free (filename_content_type);

  if (content_cmp)
    {
      goto continue_search;
    }


content_check:
  if (g_search_settings.content)
    {
      char *content_type = get_content_type (pathname);

      int content_is_searchable =
	content_desc_searchable (content_table_get_description
				 (content_type));
      app_free (content_type);

      if (content_is_searchable != APP_SUCCESS)
	goto continue_search;

      int verify_content = APP_FAILURE;

      if (g_search_settings.cregexp)
	{
	  verify_content =
	    verify_cregexp (pathname, g_search_settings.cregexp_preg);
	}
      else
	{
	  verify_content =
	    verify_ctext (pathname, g_search_settings.content_text,
			  g_search_settings.ccasei);
	}

      if (verify_content != APP_SUCCESS)
	goto continue_search;
    }


  if (strcmp (g_search_settings.size_cond, "(None)"))
    {

      if (!strcmp (g_search_settings.size_cond, "Less Than"))
	{
	  if (g_search_settings.size_lrange <= sbuf->st_size)
	    goto continue_search;
	}
      else if (!strcmp (g_search_settings.size_cond, "Equals"))
	{
	  if (g_search_settings.size_lrange != sbuf->st_size)
	    goto continue_search;
	}
      else if (!strcmp (g_search_settings.size_cond, "Greater Than"))
	{
	  if (g_search_settings.size_lrange >= sbuf->st_size)
	    goto continue_search;
	}
      else
	{
	  if (g_search_settings.size_lrange > sbuf->st_size
	      || sbuf->st_size > g_search_settings.size_rrange)
	    goto continue_search;
	}

    }


  if (g_search_settings.time)
    {
      int verify_time = APP_FAILURE;

      time_t filename_timet;
      if (!strcmp (g_search_settings.time_op, "Accessed"))
	filename_timet = sbuf->st_atime;
      else if (!strcmp (g_search_settings.time_op, "Modified"))
	filename_timet = sbuf->st_mtime;
      else
	filename_timet = sbuf->st_ctime;

      if (g_search_settings.time_between)
	{
	  char *time_between_clock_unflipped = get_time (&filename_timet);
	  char *time_between_clock = flip_date (time_between_clock_unflipped);

	  verify_time =
	    verify_time_between (g_search_settings.time_between_lclock,
				 time_between_clock,
				 g_search_settings.time_between_rclock);

	  app_free (time_between_clock_unflipped);
	  app_free (time_between_clock);
	}
      else
	{
	  verify_time =
	    verify_time_during (g_search_settings.time_range,
				g_search_settings.time_stamp, filename_timet);
	}

      if (verify_time != APP_SUCCESS)
	goto continue_search;
    }


  if (g_search_settings.permission)
    {
      if (verify_permissions (g_search_settings.permissions, sbuf->st_mode) !=
	  APP_SUCCESS)
	goto continue_search;
    }


  if (g_search_settings.user)
    {
      if (sbuf->st_uid != g_search_settings.uid)
	goto continue_search;
    }


  if (g_search_settings.group)
    {
      if (sbuf->st_gid != g_search_settings.gid)
	goto continue_search;
    }


  if (g_app_stopped)
    return FTW_STOP;

append_result:
  ;
  char *pathname_content_type = get_content_type (pathname);
  append_result_table (pathname_content_type, filename, subdirname, sbuf,
		       type);

  if (g_search_settings.matchonce)
    return FTW_STOP;


continue_search:
  if (!g_search_settings.recurse)
    {
      if (level > 0 && (type == FTW_D || type == FTW_DNR))
	return FTW_SKIP_SUBTREE;
    }

  return FTW_CONTINUE;
}
