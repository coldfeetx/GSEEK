// SPDX-License-Identifier: GPL-2.0-only
/*
 * app_externs.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

#include "regexp.h"
#include "shexp.h"
#include "result_db.h"
#include "gtk_time.h"

typedef struct search_settings
{
  char *name;
  char *location;
  int regexp;
  regex_t regexp_preg;
  int shexp;
  wordexp_t shexp_wordv;
  int shexp_tofree;
  int recurse;
  int casei;
  int hidden;
  int matchonce;
  char *content_type;
  int content_generic;
  int content;
  char *content_text;
  int ccasei;
  int cregexp;
  regex_t cregexp_preg;
  char *size_cond;
  off_t size_lrange;
  off_t size_rrange;
  int time;
  char *time_op;
  int time_between;
  char *time_between_lclock;
  char *time_between_rclock;
  int time_during;
  int time_range;
  int time_stamp;
  int permission;
  char *permissions;
  int user;
  uid_t uid;
  int group;
  gid_t gid;
} search_settings_t;


extern GtkWidget *g_app_window;
extern GtkWidget *g_app_status_statusbar, *g_app_progress_statusbar;
extern GtkWidget *g_name_combo_box_text;
extern GtkListStore *g_name_combo_box_text_entry_completion_store;
extern GtkWidget *g_location_entry;
extern GtkWidget *g_content_combo_box_text;
extern GtkWidget *g_regexp_setting_check, *g_shexp_setting_check,
  *g_recurse_setting_check, *g_case_setting_check, *g_hidden_setting_check,
  *g_matchonce_setting_check;
extern GList *g_name_history_list;

extern GtkWidget *g_content_entry;
extern GtkWidget *g_ccase_setting_check, *g_cregexp_setting_check;
extern GtkWidget *g_size_cond_combo_box_text;
extern GtkWidget *g_size_lrange_hbox, *g_size_rrange_hbox;
extern GtkWidget *g_size_lrange_spin, *g_size_rrange_spin;
extern GtkWidget *g_size_lunit_combo_box_text, *g_size_runit_combo_box_text;

extern clock_scale_t g_clock_scales[5];
extern int g_clock_scales_len;
extern GtkWidget *g_time_check;
extern GtkWidget *g_time_op_combo_box_text;
extern GtkWidget *g_time_between_radio_button, *g_time_during_radio_button;
extern GtkWidget *g_time_lclock_button, *g_time_rclock_button;
extern GtkWidget *g_time_range_spin;
extern GtkWidget *g_time_stamp_combo_box_text;
extern GtkWidget *g_permissions_check;
extern GtkWidget *g_user_permission_hbox, *g_group_permission_hbox,
  *g_others_permission_hbox;
extern GtkWidget *g_user_read_permission_combo_box_text,
  *g_group_read_permission_combo_box_text,
  *g_others_read_permission_combo_box_text;
extern GtkWidget *g_user_write_permission_combo_box_text,
  *g_group_write_permission_combo_box_text,
  *g_others_write_permission_combo_box_text;
extern GtkWidget *g_user_exe_permission_combo_box_text,
  *g_group_exe_permission_combo_box_text,
  *g_others_exe_permission_combo_box_text;
extern GtkWidget *g_user_combo_box_text, *g_group_combo_box_text;


extern GtkWidget *g_result_treeview;
extern GtkListStore *g_result_store;
extern GtkTreeSortable *g_result_sortable;


void append_result_window (GtkListStore *, search_result_t *,
			   GtkTreeSortable *, int, int);
