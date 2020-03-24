/*
** tzdump - display time zone info in TZ format
**
** This program was almost named unzic.  However, since zic's input isn't
** fully recoverable from its output, that name would be a bit misleading.
*/

// #if !defined(lint) && !defined(__lint)
// static const char rcsid[] =
// 		"@(#)$Id$";
// static const char rcsname[] =
// 		"@(#)$Name$";
// #endif

/*
** Bugs:
**	o There should be a method for searching for timezone name and
**	  abbreviation aliases.
*/

// #define RCSID rcsid
// #define RCSNAME rcsname
// #else
// #define RCSID NULL
// #define RCSNAME NULL
// #endif

#ifdef ENABLETRACE
#define TRACE(args) (void)printf args
#else
#define TRACE(args)
#endif

/* Ought to have an autoconf configure script */

#include "tzdump.h"

// #ifndef TZNAME_MAX
// #endif
// #ifndef TZDIR
// #define TZDIR "/usr/share/lib/zoneinfo"
// #endif

// #ifndef MAXPATHLEN
// #define MAXPATHLEN 1024
// #endif

#ifndef SECSPERMIN
#define SECSPERMIN 60
#endif
#ifndef MINSPERHOUR
#define MINSPERHOUR 60
#endif
#ifndef SECSPERHOUR
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#endif

#define TIMEBUFSIZE 16
// #if
// 	/* Compute size required for ZoneInfoBufferSize */
// 		sizeof (struct tzhead)
// 			+ TZ_MAX_TIMES * (4 + 1) * sizeof (char)
// 			+ TZ_MAX_TYPES * (4 + 2) * sizeof (char)
// 			+ TZ_MAX_CHARS * sizeof (char)
// 			+ TZ_MAX_LEAPS * (4 + 4) * sizeof (char)
// 			+ TZ_MAX_TYPES * sizeof (char)
// #ifdef STRUCT_TZHEAD_TTISGMTCNT
// 			+ TZ_MAX_TYPES * sizeof (char)
// #endif
// #endif /* 0 */

char *comment = "";

/*
**  TZ variable can either be:
**	:<characters>
**	("zoneinfo timezone")
**  or
**	<std><offset1>[<dst>[<offset2>]][,<start>[/<time>],<end>[/<time>]
**	("POSIX timezone" -> see elsie.nci.nih.gov for public domain info)
**
**  Solaris extends this in that a timezone without a leading colon that
**  doesn't parse as a POSIX timezone is treated as a zoneinfo timezone.
**  A zoneinfo timezone refers to a data file that contains a set of rules
**  for calculating time offsets from UTC:
**	/usr/share/lib/zoneinfo/$TZ
**
**	TZDIR -> /usr/share/lib/zoneinfo; defined in <tzfile.h>
**	See lib/libc/port/gen/time_comm.c:_tzload()
**
**	std/dst: timezone abbreviations (e.g. EST/EDT)
**	offset1/offset2: offset from GMT ([+-]hour:min:sec)
**	start/end: Mm.w.d
**		m - month (1-12)
**		w - week (1-5)
**			1 - week in which the d'th day first falls
**			5 - week in which the d'th day last falls (4 or 5)
**		d - day (0-6, 0=Sunday)
**		NB: Two other formats are defined, but not used here.
**	time: HH[:MM[:SS]]
**
*/

/* XXX Unsafe replacements */

// #ifndef HAVE_STRNCPY
// #define strncpy(dest, src, len) strcpy(dest, src)
// #endif

// #ifndef HAVE_STRNCAT
// #define strncat(dest, src, len) strcat(dest, src)
// #endif

#if 0
#ifdef HAVE_SNPRINTF
	(void)snprintf(p, len, fmt, ...);
#else
	(void)sprintf(p, fmt, ...);
#endif
#endif /* 0 */

/*
**	bcopy(a,b,c)	memcpy(b,a,c)
**	bzero(a,b)	memset(a,0,b)
**	index(a,b)	strchr(a,b)
**	rindex(a,b)	strrchr(a,b)
*/

/* XXX Replacements for memset() and memcpy()??? */
#if 0
#ifndef HAVE_MEMSET
  /* XXX Assumes that val=0 */
#define memset(dest, val, len) bzero(dest, len)
#endif
#ifndef HAVE_MEMCPY
#define memcpy(dest, src, len) bcopy(src, dest, len)
#endif
#endif /* 0 */

void freePoint(void *block)
{
	// call by reference
	// example freePoint(&point);
	// NOTE:   -------> '&' <-------
	if (*(void **)block)
		free(*(void **)block);
	*(void **)block = NULL;
}

char *readFile(const char *filename)
{
	char *buffer = NULL;
	int string_size, read_size;
	FILE *handler = fopen(filename, "r");

	if (handler)
	{
		// Seek the last byte of the file
		fseek(handler, 0, SEEK_END);
		// Offset from the first to the last byte, or in other words, filesize
		string_size = ftell(handler);
		// go back to the start of the file
		rewind(handler);

		// Allocate a string that can hold it all
		buffer = (char *)malloc(sizeof(char) * (string_size + 1));

		// Read it all in one operation
		read_size = fread(buffer, sizeof(char), string_size, handler);

		// fread doesn't set it so put a \0 in the last position
		// and buffer is now officially a string
		buffer[string_size] = '\0';

		if (string_size != read_size)
		{
			// Something went wrong, throw away the memory and set
			// the buffer to NULL
			free(buffer);
			buffer = NULL;
		}

		// Always remember to close the file.
		fclose(handler);
	}

	return buffer;
}

/*
** Determine n'th week holding wday.
*/
int weekofmonth(int mday, int wday)
{
	// Um... wday unused.
	TRACE(("weekofmonth(mday = %d, wday = %d)\n", mday, wday));

	int tmp;
	for (tmp = 0; mday > 0; tmp++)
		mday -= 7;
	/* Assume that the last week of the month is desired. */
	if (tmp == 4)
		tmp++;
	return tmp;
}

/*
** Convert seconds to hour:min:sec format.
*/
char *timefmt(char *p, int len, int interval)
{
	int hours = 0, mins = 0, secs = 0;
	char *fmt, *sign;
	static char fmtbuf[TIMEBUFSIZE];

	TRACE(("timefmt(p = %p, len = %d, interval = %d)\n", p, len, interval));

	if (p == NULL)
	{
		p = &fmtbuf[0];
		len = sizeof fmtbuf;
	}

	/* XXX Verify for negative values of interval. */
	sign = "";
	if (interval < 0)
	{
		sign = "-";
		interval = -interval;
	}

	secs = interval % SECSPERMIN;
	interval -= secs;
	interval /= SECSPERMIN;
	mins = interval % MINSPERHOUR;
	interval -= mins;
	hours = interval / MINSPERHOUR;

	fmt = ((secs != 0)
						 ? "%s%d:%d:%d"
						 : ((mins != 0)
										? "%s%d:%d"
										: "%s%d"));
	snprintf(p, len, fmt, sign, hours, mins, secs);
	return p;
}

/*
**
*/
char *ctimeGMT(long long int time)
{
	if (time > 0xFFFFFFFF)
	{
		// printf("  %lld ", time);
		return "null";
	}

	static char buf[2 * TIMEBUFSIZE];
	struct tm *tmptr;

	TRACE(("ctimeGMT(time = %ld)\n", time));

	tmptr = gmtime((time_t *)&time);
	strftime(buf, sizeof buf, "%a %b %d %T GMT %Y", tmptr);

	return &buf[0];
}

/*
** Generate an int from field_size chars.
*/
long long int tzhdecode(const char *p, size_t field_size)
{
	TRACE(("tzhdecode(p = %p [%d])\n", p, (p) ? p : "(null)"));
	int i;
	long long int tmp;
	tmp = (*p++ & 0xff);
	for (i = 1; i < field_size; i++)
		tmp = (tmp << 8) | (*p++ & 0xff);
	return tmp;
}

char *wrapabbrev(char *abbrev)
{
	static char wrapbuf[TZ_MAX_CHARS + 2];
	int i;

	for (i = 0; abbrev[i] != '\0'; ++i)
	{
		if (isdigit(abbrev[i]) || (abbrev[i] == '+') || (abbrev[i] == '-'))
		{
			snprintf(wrapbuf, sizeof(wrapbuf), "<%s>", abbrev);
			wrapbuf[sizeof(wrapbuf) - 1] = '\0';
			return wrapbuf;
		}
	}
	return abbrev;
}

/*
** Read zoneinfo data file and generate expanded TZ value.
*/
char *dumptzdata(struct tzhead *tzh_ptr, const char *tzfile_buffer_ptr, size_t field_size, struct tm **dst_start_tm, struct tm **dst_end_tm)
{
	int i;
	int ttisgmtcnt, ttisstdcnt, leapcnt, timecnt, typecnt, charcnt; // time zone header, each 4 bytes
	char stdoffset[TIMEBUFSIZE], dstoffset[TIMEBUFSIZE];
	char startdate[TIMEBUFSIZE], enddate[TIMEBUFSIZE];
	char starttime[TIMEBUFSIZE], endtime[TIMEBUFSIZE];
	char chars[TZ_MAX_CHARS];
	char *p = (char *)tzfile_buffer_ptr;
	char *retstring = malloc(TZ_MAX_CHARS * sizeof(char));
	struct transit_t transit[TZ_MAX_TIMES];
	struct leap_t leaps[TZ_MAX_LEAPS];
	struct lti_t lti[TZ_MAX_TYPES];
	struct tt_t tt[2] = {
			{0, 0, 0, NULL},
			{0, 0, 0, NULL}};

	time_t now;
	int startindex, endindex;
	struct tm *starttm, *endtm;
	struct tm tmbuf[2];

	/* initialize */
	memset(transit, 0, sizeof transit);
	memset(lti, 0, sizeof lti);
	memset(leaps, 0, sizeof leaps);
	now = time(NULL);

	/*
	** Parse the zoneinfo data file header.
	*/
	ttisgmtcnt = tzhdecode(tzh_ptr->tzh_ttisgmtcnt, 4);
	ttisstdcnt = tzhdecode(tzh_ptr->tzh_ttisstdcnt, 4);
	leapcnt = tzhdecode(tzh_ptr->tzh_leapcnt, 4);
	timecnt = tzhdecode(tzh_ptr->tzh_timecnt, 4);
	typecnt = tzhdecode(tzh_ptr->tzh_typecnt, 4);
	charcnt = tzhdecode(tzh_ptr->tzh_charcnt, 4);

#ifdef DEBUG
	printf("tzh_ttisgmtcnt = %d\n", ttisgmtcnt);
	printf("tzh_ttisstdcnt = %d\n", ttisstdcnt);
	printf("tzh_leapcnt = %d\n", leapcnt);
	printf("tzh_timecnt = %d\n", timecnt);
	printf("tzh_typecnt = %d\n", typecnt);
	printf("tzh_charcnt = %d\n", charcnt);
#endif

	if (timecnt > TZ_MAX_TIMES)
	{
		fprintf(stderr, "timecnt too large (%d)\n", timecnt);
		return NULL;
	}
	if (typecnt == 0)
	{
		fprintf(stderr, "typecnt too small (%d)\n", typecnt);
		return NULL;
	}
	if (typecnt > TZ_MAX_TYPES)
	{
		fprintf(stderr, "typecnt too large (%d)\n", typecnt);
		return NULL;
	}
	if (charcnt > TZ_MAX_CHARS)
	{
		fprintf(stderr, "charcnt too large (%d)\n", charcnt);
		return NULL;
	}
	if (leapcnt > TZ_MAX_LEAPS)
	{
		fprintf(stderr, "leapcnt too large (%d)\n", leapcnt);
		return NULL;
	}
	if (ttisstdcnt > TZ_MAX_TYPES)
	{
		fprintf(stderr, "ttisstdcnt too large (%d)\n", ttisstdcnt);
		return NULL;
	}
	if (ttisgmtcnt > TZ_MAX_TYPES)
	{
		fprintf(stderr, "ttisgmtcnt too large (%d)\n", ttisgmtcnt);
		return NULL;
	}

	/*
	** Parse the remainder of the zoneinfo data file.
	*/

	/* transition times */
	for (i = 0; i < timecnt; i++)
	{
		transit[i].time = tzhdecode(p, field_size);
		p += field_size;
		/* record the next two (future) transitions according to the current time */
		// (most) 31,622,400s == 366days == 1 year
		if ((field_size == 4 && (time_t)transit[i].time > now && (time_t)transit[i].time < now + 31622400) ||
				(field_size == 8 && transit[i].time > now && transit[i].time < now + 31622400))
		{
			if (tt[0].time == 0)
			{
				tt[0].time = transit[i].time;
				tt[0].index = i;
			}
			else if (tt[1].time == 0)
			{
				tt[1].time = transit[i].time;
				tt[1].index = i;
			}
		}
	}
	/* local time types for above: 0 = std, 1 = dst */
	for (i = 0; i < timecnt; i++)
		transit[i].type = (unsigned char)*p++;

#ifdef DEBUG
	// debug
	for (i = 0; i < timecnt; i++)
	{
		field_size == 4 ? printf("4: transit[%d]: time=%ld", i, (long)transit[i].time) : printf("8: transit[%d]: time=%lld", i, transit[i].time);
		printf(" (%s) type=%ld\n", ctimeGMT(transit[i].time), transit[i].type);
	}
#endif

	/* GMT offset seconds, local time type, abbreviation index */
	for (i = 0; i < typecnt; i++)
	{
		lti[i].gmtoffset = tzhdecode(p, 4);
		p += 4;
		lti[i].isdst = (unsigned char)*p++;
		lti[i].abbrind = (unsigned char)*p++;
	}

	/* timezone abbreviation strings */

	for (i = 0; i < charcnt; i++)
		chars[i] = *p++;
	chars[i] = '\0'; /* ensure '\0' at end */

	/* leap second transitions, accumulated correction */
	for (i = 0; i < leapcnt; i++)
	{
		leaps[i].transit = tzhdecode(p, 4);
		p += 4;
		leaps[i].correct = tzhdecode(p, 4);
		p += 4;
#ifdef DEBUG
		printf("leaps[%d]: transit=%ld correct=%ld\n", i, leaps[i].transit, leaps[i].correct);
#endif
	}

	/*
	** indexed by type:
	**	0 = transition is wall clock time
	**	1 = transition time is standard time
	**	default (if absent) is wall clock time
	*/
	for (i = 0; i < ttisstdcnt; i++)
	{
		lti[i].stds = *p++;
	}

	/*
	** indexed by type:
	**	0 = transition is local time
	**	1 = transition time is GMT
	**	default (if absent) is local time
	*/
	for (i = 0; i < ttisgmtcnt; i++)
	{
		lti[i].gmts = *p++;
	}

#ifdef DEBUG
	for (i = 0; i < typecnt; i++)
	{
		printf("lti[%d]: gmtoffset=%d isdst=%d abbrind=%d stds=%d gmts=%d\n",
					 i, lti[i].gmtoffset, lti[i].isdst, lti[i].abbrind, lti[i].stds, lti[i].gmts);
	}
#endif
	/* Simple case of no dst */
	if (typecnt == 1)
	{
		timefmt(stdoffset, sizeof stdoffset, -lti[0].gmtoffset);
		sprintf(retstring, "%s%s%s", comment, wrapabbrev(&chars[(unsigned char)lti[0].abbrind]), stdoffset);
		return retstring;
	}

	/*
	** XXX If no transitions exist in the future, should we assume
	** XXX that dst no longer applies, or should we assume the most
	** XXX recent rules continue to apply???
	** XXX For the moment, we assume the latter and proceed.
	*/

	if (tt[0].time == 0 && tt[1].time == 0)
	{ // is not dst
		timefmt(stdoffset, sizeof stdoffset, -lti[transit[timecnt - 1].type].gmtoffset);
		sprintf(retstring, "%s%s%s", comment, wrapabbrev(&chars[(unsigned char)lti[transit[timecnt - 1].type].abbrind]), stdoffset);
		return retstring;
	}
	else if (tt[1].time == 0)
	{
		tt[1].index = tt[0].index;
		tt[0].index--;
		tt[1].time = transit[tt[1].index].time;
		tt[0].time = transit[tt[0].index].time;
	}

	tt[0].type = transit[tt[0].index].type;
	tt[1].type = transit[tt[1].index].type;

	/*
	** Convert time_t values to struct tm values.
	*/

	for (i = 0; i <= 1; i++)
	{
		time_t tmptime;
		tmptime = tt[i].time + lti[tt[(i > 0) ? 0 : 1].type].gmtoffset;
		tt[i].time = tt[i].time + lti[tt[i].type].gmtoffset;
		// if (lti[tt[i].type].gmts == 1)
		// 	tmptime = tt[i].time;
		// else
		//   tmptime = tt[i].time + lti[tt[(i > 0) ? 0 : 1].type].gmtoffset;
		// printf("%d %ld\n", i, tmptime);
		// if (lti[tt[i].type].stds != 0 && lti[tt[i].type].isdst != 0)
		//   tmptime += lti[tt[i].type].gmtoffset - lti[tt[(i > 0) ? 0 : 1].type].gmtoffset;
		tt[i].tm = gmtime(&tmptime);
		tt[i].tm = memcpy(&tmbuf[i], tt[i].tm, sizeof(struct tm));
	}

#ifdef DEBUG
	// debug
	printf("tt[0]: time=%ld (%s) index=%d type=%d\n", tt[0].time, ctimeGMT(tt[0].time), tt[0].index, tt[0].type);
	printf("tt[1]: time=%ld (%s) index=%d type=%d\n", tt[1].time, ctimeGMT(tt[1].time), tt[1].index, tt[1].type);
#endif

	if (lti[tt[0].type].isdst == 1)
	{
		startindex = 0;
		endindex = 1;
	}
	else
	{
		startindex = 1;
		endindex = 0;
	}
	starttm = tt[startindex].tm; // dst start time
	endtm = tt[endindex].tm;		 // dst end time
	*dst_start_tm = starttm;
	*dst_end_tm = endtm;
	/* XXX This calculation of the week is too simple-minded??? */
	/* XXX A hueristic would be to round 4 up to 5. */

	sprintf(startdate, "M%d.%d.%d", 1 + starttm->tm_mon, weekofmonth(starttm->tm_mday, starttm->tm_wday), starttm->tm_wday);
	if ((starttm->tm_min != 0) || (starttm->tm_sec != 0))
		sprintf(starttime, "/%.2d:%.2d:%.2d", starttm->tm_hour, starttm->tm_min, starttm->tm_sec);
	else
	{
		if (starttm->tm_hour != 2) // 2 is default in dst
			sprintf(starttime, "/%d", starttm->tm_hour);
		else
			starttime[0] = 0;
	}

	sprintf(enddate, "M%d.%d.%d", 1 + endtm->tm_mon, weekofmonth(endtm->tm_mday, endtm->tm_wday), endtm->tm_wday);
	if ((endtm->tm_min != 0) || (endtm->tm_sec != 0))
	{
		sprintf(endtime, "/%.2d:%.2d:%.2d", endtm->tm_hour, endtm->tm_min, endtm->tm_sec);
	}
	else
	{
		if (endtm->tm_hour != 2) // 2 is default in dst
			sprintf(endtime, "/%d", endtm->tm_hour);
		else
			endtime[0] = 0;
	}

	timefmt(stdoffset, sizeof stdoffset, -lti[tt[endindex].type].gmtoffset);
	timefmt(dstoffset, sizeof dstoffset, -lti[tt[startindex].type].gmtoffset);
	sprintf(retstring, "%s%s%s%s", comment, wrapabbrev(&chars[(unsigned char)lti[tt[endindex].type].abbrind]), stdoffset, wrapabbrev(&chars[(unsigned char)lti[tt[startindex].type].abbrind]));
	if ((lti[tt[startindex].type].gmtoffset - lti[tt[endindex].type].gmtoffset) != 3600)
		sprintf(retstring, "%s%s", retstring, dstoffset);
	sprintf(retstring, "%s,%s%s,%s%s", retstring, startdate, starttime, enddate, endtime);
	/*modify */
	return retstring;
}

char *ZyTimeZone(char *file, struct tm **dst_start, struct tm **dst_end)
{
	int field_size; // TZif = 4, TZif2 and TZif3 = 8
	char *buffer;
	char *start_dataptr;
	char *POSIX_tz_string;
	struct tzhead *tzh_ptr = malloc(sizeof(struct tzhead));

	buffer = readFile(file); // read time zone file
	if (buffer == NULL)
	{
		fprintf(stderr, "The file was not readed: %s\n", file);
#ifdef ZLOG_TOOL
		size_t needed = snprintf(NULL, 0, "zlog 5 2 'In tz_parser: The file was not readed: %s'", file);
		char *cmd = malloc((needed + 1) * sizeof(char));
		snprintf(cmd, needed + 1, "zlog 5 2 'In tz_parser: The file was not readed: %s'", file);
		system(cmd);
		freePoint(&cmd);
#endif
		return NULL;
	}

	memcpy(tzh_ptr, buffer, HEADER_LEN); // assign to tzhead struct

	if (tzh_ptr->tzh_version[0] != '\0' && tzh_ptr->tzh_version[0] != '2' && tzh_ptr->tzh_version[0] != '3')
	{
		fprintf(stderr, "TZ version: unknown...\n");
#ifdef ZLOG_TOOL
		system("zlog 5 2 'In tz_parser: TZ version: unknown...");
#endif
		return NULL;
	}

#if 1
	// only useing version 1 part because only support 32bit time_t
#ifdef DEBUG
	printf("TZ version: \"TZif%c\"\n", tzh_ptr->tzh_version[0]);
#endif
	start_dataptr = buffer + HEADER_LEN;
	field_size = 4;
#else
	if (tzh_ptr->tzh_version[0] == '\0') // TZif
	{
#ifdef DEBUG
		printf("TZ version:1");
#endif
		start_dataptr = buffer + HEADER_LEN;
		field_size = 4;
	}
	else if (tzh_ptr->tzh_version[0] == '2' || tzh_ptr->tzh_version[0] == '3') // TZif2
	{
		struct stat file_status;
		stat(file, &file_status);
#ifdef DEBUG
		printf("TZ version: \"TZif%c\"\n", tzh_ptr->tzh_version[0]);
#endif
		start_dataptr = memmem(buffer + HEADER_LEN, file_status.st_size - HEADER_LEN, tzh_ptr->tzh_version[0] == '2' ? "TZif2" : "TZif3", 5);
		if (start_dataptr == NULL)
		{
			printf("error finding TZif2 header");
			return NULL;
		}
		memcpy(tzh_ptr, start_dataptr, HEADER_LEN); // reassign to tzhead
		start_dataptr = start_dataptr + HEADER_LEN;
		field_size = 8;
	}
#endif
	if (start_dataptr != NULL)
		POSIX_tz_string = dumptzdata(tzh_ptr, start_dataptr, field_size, dst_start, dst_end);

	freePoint(&buffer);
	freePoint(&tzh_ptr);
	return POSIX_tz_string;
}
