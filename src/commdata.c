#include <pebble.h>
#include "commdata.h"
#include "main.h"

CalendarItem cals[MAX_CAL_ENTRIES];
int cal_entries = 0;
int day_breaks[MAX_CAL_ENTRIES+1];
int day_entries = 0;	
	
void wipe_cals() {
	cal_entries = 0;
	for (int i=0; i<MAX_CAL_ENTRIES; i++) {
		cals[i].subject[0] = '\0';
		cals[i].start = 0;
		cals[i].end = 0;
		cals[i].location[0] = '\0';
	}
}

// Determine day_breaks
void compute_day_breaks() {
	int cur_day = -1;
	day_entries = 0;
	day_breaks[0] = 0;
	day_breaks[1] = 0;
		
	for (int i=0; i<cal_entries; i++) {
		if (cals[i].start == 0) continue;
		struct tm *start_tm = localtime(&(cals[i].start));
		
		if (cur_day == -1) {
			cur_day = start_tm->tm_yday;
			day_entries ++;
		}
		if (cur_day != start_tm->tm_yday) {
			day_breaks[day_entries] = i;
			day_entries ++;
			cur_day = start_tm->tm_yday;
		}
	}
	if (day_entries) {
		day_breaks[day_entries] = cal_entries;
	}
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
	APP_LOG(APP_LOG_LEVEL_DEBUG, "out_sent_handler");
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
	APP_LOG(APP_LOG_LEVEL_DEBUG, "out_failed_handler");
}


void pull_meetings() {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "pull_meetings");
	bool fetched = 0;
	int success = 0;
	for (int i=0; i<cal_entries; i++) {
		if (cals[i].start == 0) {
			if (! fetched) {
				app_message_fetch_calendar(i+1);
				fetched = 1;
			} else {
				//APP_LOG(APP_LOG_LEVEL_DEBUG, "set pull_meetings timer");
				app_timer_register(CAL_FETCH_INTERVAL_MS, pull_meetings, NULL);
				break;
			}
		} else {
			success ++;
		}
	}
	if (success % 3 == 1) {
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "display data");
		compute_day_breaks();
		refresh_menulayer();
	}
}

// Send a log message containing the content of the tuple.
void tuple_log(const Tuple* t) {
	if (!t) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "null tuple");
		return;
	}
	switch (t->type) {
		case TUPLE_CSTRING:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu cstring:'%s'", t->key, t->value->cstring);
			break;
		case TUPLE_BYTE_ARRAY:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu bytearray len:%d", t->key, t->length);
			break;
		case TUPLE_INT:
			if (t->length == 4) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int32:%ld", t->key, t->value->int32); }
			if (t->length == 2) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int16:%d", t->key, t->value->int16); }
			if (t->length == 1) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int8:%d", t->key, t->value->int8); }
			break;
		case TUPLE_UINT:
			if (t->length == 4) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int32:%lu", t->key, t->value->uint32); }
			if (t->length == 2) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int16:%u", t->key, t->value->uint16); }
			if (t->length == 1) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int8:%u", t->key, t->value->uint8); }
			break;
	}
}


void in_received_handler(DictionaryIterator *received, void *context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "in_received_handler %p %p", received, context);
	// incoming message received
	// Check for fields you expect to receive
	Tuple *index_tuple = dict_find(received, CAL_INDEX);
	//tuple_log(index_tuple);
	Tuple *subject_tuple = dict_find(received, CAL_SUBJECT);
	//tuple_log(subject_tuple);
	Tuple *start_tuple = dict_find(received, CAL_START);
	//tuple_log(start_tuple);
	Tuple *end_tuple = dict_find(received, CAL_END);
	//tuple_log(end_tuple);
	Tuple *location_tuple = dict_find(received, CAL_LOCATION);
	//tuple_log(location_tuple);
	Tuple *total_tuple = dict_find(received, CAL_TOTAL);
	//tuple_log(total_tuple);

	
	
	// Act on the found fields received
	if (index_tuple) {
		int i = index_tuple->value->int32;
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "in_received_handler %d", i);

		if (subject_tuple) {
			strncpy(cals[i].subject, subject_tuple->value->cstring, MAX_CAL_TEXT_LENGTH);
		}
		if (start_tuple) {
			cals[i].start = start_tuple->value->uint32;
		}
		if (end_tuple) {
			cals[i].end = end_tuple->value->uint32;
		}
		if (location_tuple) {
			strncpy(cals[i].location, location_tuple->value->cstring, MAX_CAL_TEXT_LENGTH);
		}
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "in_received_handler done");
	} else if (total_tuple) {
		cal_entries = total_tuple->value->int32;
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "total_tuple %d", cal_entries);
		
		app_timer_register(CAL_FETCH_INTERVAL_MS, pull_meetings, NULL);
	}
}


void in_dropped_handler(AppMessageResult reason, void *context) {
	// incoming message dropped
	APP_LOG(APP_LOG_LEVEL_DEBUG, "in_dropped_handler");
}

void app_message_fetch_calendar(int i) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_fetch_calendar %d", i);
	DictionaryIterator *iter;
		
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) return;

	if (i == 0) {
		Tuplet value = TupletInteger(CAL_FETCH_ALL, 1);
		dict_write_tuplet(iter, &value);
		wipe_cals();
	} else {
		Tuplet value = TupletInteger(CAL_FETCH_ONE, i);
		dict_write_tuplet(iter, &value);
	}
	
	app_message_outbox_send();
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_fetch_calendar done");
}

void app_message_fetch_first(void *x) {
	app_message_fetch_calendar(0);
}

// Set up app sync, defining initial values for data parameters
void app_message_init() {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_init");

	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	
	const uint32_t inbound_size = 256;
	const uint32_t outbound_size = 256;
	app_message_open(inbound_size, outbound_size);

	// Send a refresh request
	//app_message_fetch_calendar(0);
	app_timer_register(1000, app_message_fetch_first, NULL);
}
