// SPDX-License-Identifier: GPL-2.0-only
/*
 * gtk_time.h
 *
 *  Copyright (C) 2012 coldfeet (soumikbanerjee68@yahoo.com)
 *
 */

enum
{
  MINUTES,
  HOURS,
  DAYS,
  WEEKS,
  MONTHS,
  YEARS,
  USER,
  GROUP,
  OTHERS,
};


typedef struct clock_scale
{
  char *scale;
  int range;
  double secs;
} clock_scale_t;
