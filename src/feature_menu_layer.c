#include "pebble.h"
#include <time.h>

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
	
typedef struct {
	char subject[MAX_CAL_TEXT_LENGTH];
	time_t start;
	time_t end;
	char location[MAX_CAL_TEXT_LENGTH];
} CalendarItem;

static CalendarItem cals[MAX_CAL_ENTRIES];
static int cal_entries = 0;
static int day_breaks[MAX_CAL_ENTRIES+1];
static int day_entries = 0;

void app_message_fetch_calendar();

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

static Window *window;

// This is a menu layer
// You have more control than with a simple menu layer
static MenuLayer *menu_layer;

// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
	return day_entries;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	if (section_index > day_entries) return 0;
	return day_breaks[section_index+1] - day_breaks[section_index];
}

// A callback is used to specify the height of the section header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	// This is a define provided in pebble.h that you may use for the default height
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
	struct tm *start_tm = localtime(&(cals[day_breaks[section_index]].start));
	char buf[48];

	strftime(buf, 48, "%A, %b %e", start_tm);
	menu_cell_basic_header_draw(ctx, cell_layer, buf);
}

void render_time(char *temp, struct tm *t) {
	char b[8];
	
	int h = t->tm_hour;
	if (h == 0) {
		h = 12;
	} else if (h > 12) {
		h -= 12;
	}
	snprintf(temp, 10, "%d", h);

	if (t->tm_min != 0) {
		strftime(b, 8, ":%M", t);
		strcat(temp, b);
	}
	
	if (t->tm_hour == 0 || t->tm_hour >= 12) {
		strcat(temp, "p");
	} else {
		strcat(temp, "a");
	}
	
}

// This is the menu item draw callback where you specify what each item should look like
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	CalendarItem item = cals[day_breaks[cell_index->section] + cell_index->row];
	char buf[64] = "";

	if (item.start != 0) {
		struct tm *start_tm = localtime(&(item.start));
		char st[10];
		render_time(st, start_tm);
		struct tm *end_tm = localtime(&(item.end));
		char et[10];
		render_time(et, end_tm);

		snprintf(buf, 64, "%s-%s %s", st, et, item.location); 
	
		menu_cell_basic_draw(ctx, cell_layer, item.subject, buf, NULL);
	}
}

// Here we capture when a user selects a menu item
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "menu_select_callback");
	//app_message_fetch_calendar(0);
	
	CalendarItem item = cals[day_breaks[cell_index->section] + cell_index->row];
	
	if (item.start != 0) {
		struct tm *start_tm = localtime(&(item.start));
		char st[10];
		render_time(st, start_tm);

		struct tm *end_tm = localtime(&(item.end));
		char et[10];
		render_time(et, end_tm);
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "times %lu %s %lu %s", item.start, st, item.end, et);
	}

	return;
}

// This initializes the menu upon window load
void window_load(Window *window) {
  // Now we prepare to initialize the menu layer
  // We need the bounds to specify the menu layer's viewport size
  // In this case, it'll be the same as the window's
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  menu_layer = menu_layer_create(bounds);

  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(menu_layer, window);

  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

void window_unload(Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "window_unload");
	// Destroy the menu layer
	menu_layer_destroy(menu_layer);
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "out_sent_handler");
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
	APP_LOG(APP_LOG_LEVEL_DEBUG, "out_failed_handler");
}


void pull_meetings() {
	bool fetched = 0;
	int success = 0;
	for (int i=0; i<cal_entries; i++) {
		if (cals[i].start == 0) {
			if (! fetched) {
				app_message_fetch_calendar(i+1);
				fetched = 1;
			} else {
				app_timer_register(100, pull_meetings, NULL);
				break;
			}
		} else {
			success ++;
		}
	}
	if (success % 3 == 1) {
		compute_day_breaks();
		menu_layer_reload_data(menu_layer);
	}
}

void in_received_handler(DictionaryIterator *received, void *context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "in_received_handler");
	// incoming message received
	// Check for fields you expect to receive
	Tuple *index_tuple = dict_find(received, CAL_INDEX);
	Tuple *subject_tuple = dict_find(received, CAL_SUBJECT);
	Tuple *start_tuple = dict_find(received, CAL_START);
	Tuple *end_tuple = dict_find(received, CAL_END);
	Tuple *location_tuple = dict_find(received, CAL_LOCATION);
	Tuple *total_tuple = dict_find(received, CAL_TOTAL);

	// Act on the found fields received
	if (index_tuple) {
		int i = index_tuple->value->int32;
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
	} else if (total_tuple) {
		cal_entries = total_tuple->value->int32;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "total_tuple %d", cal_entries);
		
		app_timer_register(100, pull_meetings, NULL);
	}
}


void in_dropped_handler(AppMessageResult reason, void *context) {
	// incoming message dropped
	APP_LOG(APP_LOG_LEVEL_DEBUG, "in_dropped_handler");
}

void app_message_fetch_calendar(int i) {
	DictionaryIterator *iter;
		
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) return;

	if (i == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_fetch_calendar %d", i);
		Tuplet value = TupletInteger(CAL_FETCH_ALL, 1);
		dict_write_tuplet(iter, &value);
		wipe_cals();
	} else {
		Tuplet value = TupletInteger(CAL_FETCH_ONE, i);
		dict_write_tuplet(iter, &value);
	}
	
	app_message_outbox_send();
}

// Set up app sync, defining initial values for data parameters
void app_message_init() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_init");

	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	
	const uint32_t inbound_size = 256;
	const uint32_t outbound_size = 256;
	app_message_open(inbound_size, outbound_size);

	// Send a refresh request
	app_message_fetch_calendar(0);
}

int main(void) {
	// initialize data structures
	wipe_cals();
	compute_day_breaks();
	
	window = window_create();

	// Setup the window handlers
	window_set_window_handlers(window, (WindowHandlers) {
    	.load = window_load,
    	.unload = window_unload,
	});

	app_message_init();
	
	window_stack_push(window, true /* Animated */);

	app_event_loop();

	app_message_deregister_callbacks();
	window_destroy(window);
}
