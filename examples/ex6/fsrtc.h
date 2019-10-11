#ifndef _FSRTC_H
#define _FSRTC_H

struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
};

#define FSRTC_MAGIC	'f'

#define FSRTC_SET	_IOW(FSRTC_MAGIC, 0, struct rtc_time)
#define FSRTC_GET	_IOR(FSRTC_MAGIC, 1, struct rtc_time)

#endif
