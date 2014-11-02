#include <pebble.h>
#include "main.h"

#include "commdata.h"
#include "detail.h"

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
	CalendarItem item = cals[day_breaks[cell_index->section] + cell_index->row];
	
	if (item.start != 0) {
		show_detail(&item);
	}
}

// This initializes the menu upon window load
void window_load(Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "window_load");

	// Now we prepare to initialize the menu layer
	// We need the bounds to specify the menu layer's viewport size
	// In this case, it'll be the same as the window's
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);

	// Create the menu layer
	menu_layer = menu_layer_create(bounds);

	// Set all the callbacks for the menu layer
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
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

void refresh_menulayer() {
	menu_layer_reload_data(menu_layer);
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
