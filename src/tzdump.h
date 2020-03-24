
#ifndef TZDUMP_H
#define TZDUMP_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h> /* about va_list */
#include <unistd.h> /* about getopt */
#include <ctype.h>
#include <time.h>
#include "tzfile.h"

#define HEADER_LEN 44

struct transit_t
{
  long long int time;
  long type;
};

struct leap_t
{
  time_t transit; /* trans */
  long correct;   /* corr */
  char roll;      /* ??? */
};

struct tt_t
{
  time_t time;
  int index;
  unsigned char type;
  struct tm *tm;
};
struct lti_t
{
  int32_t gmtoffset;     /* gmtoffs */
  unsigned char isdst;   /* isdsts */
  unsigned char abbrind; /* abbrinds */
  int stds;              /* 1 or 0 */
  int gmts;              /* 1 or 0 */
};

/**
  @brief Get zyxel time zone format (string.)

  @param file(IANA time zone file)

  @return string

  example:
    const char *filepath = "/usr/share/zoneinfo/America/New_York"
    struct tm *tmstart; // DST start time
    struct tm *tmend;   // DST end time
    char *result = ZyTimeZone(filepath, &tmstart, &tmend);

*/
char *ZyTimeZone(char *file, struct tm **dst_start, struct tm **dst_end);
// int weekofmonth(int mday, int wday);
#endif /* !defined TZDUMP_H */
