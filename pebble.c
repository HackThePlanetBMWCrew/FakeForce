#include "pebble.h"

#define NUM_MENU_SECTIONS 1
#define NUM_FIRST_MENU_ITEMS 3

static Window *s_main_window;
static Window *mini_window;
static TextLayer *mini_layer;
static TextLayer *maxi_layer;
static MenuLayer *s_menu_layer;
static char buffer[15];
static char STARTED;
static int KMleft;
static char KMbuffer[20];


static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return NUM_FIRST_MENU_ITEMS;
    default:
      return 0;
  }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Displays the name of the app
  menu_cell_basic_header_draw(ctx, cell_layer, "RideFlow");
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Determine which section we're going to draw in
  
      switch (cell_index->row) {
        case 0:
          // Lets you start a trip
          menu_cell_basic_draw(ctx, cell_layer, buffer, "", NULL);
          break;
        case 1:
          //Will Display estimated distance until you need to refuel
          menu_cell_basic_draw(ctx, cell_layer, "KM until refuel", KMbuffer, NULL);
          break;
        case 2:
          //Future leaderboard option
          menu_cell_basic_draw(ctx, cell_layer, "Leaderboard", "Future Implementation",NULL);
          break;
      }
  
}

static void menu_select_callback (struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){
	if (cell_index->row==0){
		//we only care about the first option being clicked
		if (STARTED==0){
			snprintf(buffer, 15, "Trip Started");
			STARTED=1;
			if (KMleft<=30){
				//create new window to start directions && buzz watch
				vibes_long_pulse();
				window_stack_push(mini_window,true);
			}
		}else {
			snprintf(buffer, 15, "Begin Trip?");
			STARTED=0;
			KMleft=25;
			snprintf(KMbuffer, 20, "%d KM remaining",KMleft);
		}
		layer_mark_dirty(menu_layer_get_layer(menu_layer));
	}
}

static void mini_window_load(Window *window){
	
	Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);
	
	//part 1
	mini_layer = text_layer_create(GRect(12, 12, 122, 28));
	text_layer_set_text(mini_layer, "Gas Needed! Closest station:");
	text_layer_set_overflow_mode(mini_layer, GTextOverflowModeWordWrap);
	text_layer_set_text_alignment(mini_layer, GTextAlignmentCenter);
  	text_layer_set_font(mini_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(mini_layer));
	//part 2
	maxi_layer = text_layer_create(GRect(15, 49, 116, 106));
	text_layer_set_text(maxi_layer, "7 KM North");
	text_layer_set_overflow_mode(maxi_layer, GTextOverflowModeWordWrap);
	text_layer_set_text_alignment(maxi_layer, GTextAlignmentCenter);
  	text_layer_set_font(maxi_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
	layer_add_child(window_layer, text_layer_get_layer(maxi_layer));
}

static void mini_window_unload(Window *window){
	// Destroy the menu layer
  text_layer_destroy(mini_layer);
}

static void main_window_load(Window *window) {
  
  // Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
	.select_click=menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void main_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(s_menu_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  // Get the first pair
  //Tuple *t = dict_find(iterator, KEY_VIBRATE);
	
//   // Process all pairs present
//   while (t != NULL) {
//     // Long lived buffer
//     static char s_buffer[64];

//     // Process this pair's key
//     switch (t->key) {
//       case KEY_DATA:
//         // Copy value and display
//         snprintf(s_buffer, sizeof(s_buffer), "Received '%s'", t->value->cstring);
//         text_layer_set_text(s_output_layer, s_buffer);
//         break;
//     }

//     // Get next pair, if any
//     t = dict_read_next(iterator);
//   }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  //initially sets the values pertaining to whether or not a trip is going on
  snprintf(buffer, 15, "Begin Trip?");
  STARTED=0;
  // Register callbacks
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	// Open AppMessage
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	KMleft=235;
	snprintf(KMbuffer, 20, "%d KM remaining", KMleft);
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  mini_window=window_create();
  window_set_window_handlers(mini_window, (WindowHandlers) {
    .load = mini_window_load,
    .unload = mini_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
