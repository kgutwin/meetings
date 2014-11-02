#pragma once

enum {
	CAL_FETCH_ONE = 0,
	CAL_SUBJECT = 1,
	CAL_START = 2,
	CAL_END = 3,
	CAL_LOCATION = 4,
	CAL_FETCH_ALL = 5,
	CAL_TOTAL = 6,
	CAL_INDEX = 7
};

#define MAX_CAL_ENTRIES 32
#define MAX_CAL_TEXT_LENGTH 32
#define CAL_FETCH_INTERVAL_MS 100
	
typedef struct {
	char subject[MAX_CAL_TEXT_LENGTH];
	time_t start;
	time_t end;
	char location[MAX_CAL_TEXT_LENGTH];
} CalendarItem;

extern CalendarItem cals[MAX_CAL_ENTRIES];
extern int cal_entries;
extern int day_breaks[MAX_CAL_ENTRIES+1];
extern int day_entries;

	
// Wipe the cals data structure.
void wipe_cals();

// Compute the day_breaks data structure.
void compute_day_breaks();

// Retrieve the calendar; external calls should always pass the parameter zero.
void app_message_fetch_calendar(int);

// Initialize the AppMessage system.
void app_message_init();
