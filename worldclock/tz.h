#ifndef _TZ_H__
#define _TZ_H__

struct tzinfo {
	const char* name;
	const char* tz;
	long latmin;
	long latmax;
	long lonmin;
	long lonmax;
	short hours;
};

#define TZ_HALF 	0b10000000
#define TZ_NEG	 	0b01000000
#define TZ_DST 		0b00100000

struct tzinfo tz_list[] = {
	{"Brisbane", "AEST",
		-25900761,-28120489,
		137913970,153451528,
		10},

	{"Sydney", "AEDT",
		-25900761,-28120489,
		137913970,153451528,
		10 | TZ_DST},

	{"Adelaide", "ACDT",
		-25900761,-28120489,
		137913970,153451528,
		9 | TZ_DST | TZ_HALF },
};

unsigned short tz_length = sizeof(tz_list) /  sizeof(struct tzinfo);


#endif
