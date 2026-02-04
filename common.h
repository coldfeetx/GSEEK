// SPDX-License-Identifier: GPL-2.0-only
/*
 * common.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#define _GNU_SOURCE
#define _BSD_SOURCE

//This one needs to be looked into; It is System-Dependent and makes "stat" API on LINUX absolutely suck for files larger than 1G
#define _FILE_OFFSET_BITS 64

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>


#define APP_SUCCESS 0
#define APP_FAILURE 1

#define DISABLE 0
#define ENABLE 1

#define APP_NAME "GSEEK"
#define APP_DIR ".GSEEK"
#define APP_HISTORY_FILE ".history"
#define APP_ICON "gseek.png"
#define WINDOW_ICON "gseek.png"
#define LOGO_ICON "gseek.png"
#define SAVE_AS_SEARCH_RESULT_FILE "Search_Results.csv"
#define CSV_FSP ","
#define MIMETYPES_FILE "mime.types"

#define REFRESH_RESULT_RATE 150

#define errprint_exit(...)\
do {\
	fprintf(stderr, __VA_ARGS__);\
	exit(EXIT_FAILURE);\
   } while(0)

#define END(strbuf) (strbuf + strlen(strbuf))

#define STRING_IS_NULL(string) (string == NULL)

#define STRING_IS_EMPTY(string) (*string == '\0')

#define STRING_IS_NULL_OR_EMPTY(string) (STRING_IS_NULL(string) || STRING_IS_EMPTY(string))
