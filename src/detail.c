#include <pebble.h>
#include "detail.h"

static ScrollLayer *scroll_layer;
static TextLayer *text_layer;

static CalendarItem *item;
static char scroll_text[512];

#define LONG_TIME_LEN 24
void long_time(char *buf, time_t t) {
	struct tm *m = localtime(&(t));
	strftime(buf, LONG_TIME_LEN, "%r", m);
}

static void detail_window_load(Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "detail_window_load");
	
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);

	// Initialize the scroll layer
	scroll_layer = scroll_layer_create(bounds);
	// This binds the scroll layer to the window so that up and down map to scrolling
	// You may use scroll_layer_set_callbacks to add or override interactivity
	scroll_layer_set_click_config_onto_window(scroll_layer, window);

	char st[LONG_TIME_LEN];
	char et[LONG_TIME_LEN];
	long_time(st, item->start);
	long_time(et, item->end);
	
	snprintf(scroll_text, 512, 
		"%s\n\n"
		"Start: %s\n"
		"   End: %s\n"
		"   Loc: %s",
		item->subject, st, et, item->location);
	
	// Initialize the text layer
	text_layer = text_layer_create(max_text_bounds);
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_text(text_layer, scroll_text);

	// Trim text layer and scroll content to fit text box
	GSize max_size = text_layer_get_content_size(text_layer);
	text_layer_set_size(text_layer, max_size);
	scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, max_size.h + 4));
	
	// Add the layers for display
	scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_layer));
	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));
}

static void detail_window_unload(Window *window) {
	text_layer_destroy(text_layer);
	scroll_layer_destroy(scroll_layer);
}

void show_detail(CalendarItem *item_to_show) {
	item = item_to_show;
	
	Window *detail_window = window_create();
	
	window_set_window_handlers(detail_window, (WindowHandlers) {
		.load = detail_window_load,
		.unload = detail_window_unload,
	});
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "showing detail %s", item->subject);
	window_stack_push(detail_window, true);
}