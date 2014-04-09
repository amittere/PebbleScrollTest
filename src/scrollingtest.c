#include <pebble.h>

#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ITEMS 101
#define CELL_HEIGHT 32
#define TITLE_HEIGHT 64

#define NUM_TRIALS 10

// Enum specifying the current type of trial.
// Order of trials is the order specified in the enum.
typedef enum TrialType {
  PRACTICE_KNOWN, PRACTICE_UNKNOWN, TRIAL_KNOWN, TRIAL_UNKNOWN, RESULTS
} TrialType;

// Trial indices
static int known_trials[NUM_TRIALS] = { 10, 51, 24, 3, 36, 49, 75, 98, 66, 87 };
static int unknown_trials[NUM_TRIALS] = { 36, 98, 10, 3, 51, 49, 87, 66, 75, 24 };
static int practice_known = 28;
static int practice_unknown = 51;

static TrialType current_type = PRACTICE_KNOWN;
static int trial_num = 0;
static time_t time_sec;
static uint16_t time_msec;
static uint64_t time_results[NUM_TRIALS * 2];
static int accuracy_results[NUM_TRIALS * 2];

static Window *window;

// This is a menu layer
// You have more control than with a simple menu layer
static MenuLayer *menu_layer;

void start_timer() {
  time_ms(&time_sec, &time_msec);
}

uint64_t stop_timer() {
  time_t elapsed_sec;
  uint16_t elapsed_msec;
  time_ms(&elapsed_sec, &elapsed_msec);
  uint64_t time1 = time_sec * 1000 + time_msec;
  uint64_t time2 = elapsed_sec * 1000 + elapsed_msec;
  return time2 - time1;
}

// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  if (current_type == RESULTS) {
    return NUM_TRIALS * 4;
  }
  else {
    return NUM_MENU_ITEMS;
  }
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {\
  if (current_type == RESULTS || cell_index->row == 0) {
    return TITLE_HEIGHT;
  }
  return CELL_HEIGHT;
}

// A callback is used to specify the height of the section header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // This is a define provided in pebble.h that you may use for the default height
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  char str[24];
  switch (current_type) {
    case PRACTICE_KNOWN:
    case PRACTICE_UNKNOWN:
      snprintf(str, 24, "Practice");
      break;
    case TRIAL_KNOWN:
      snprintf(str, 24, "Trial %d - Known", trial_num);
      break;
    case TRIAL_UNKNOWN:
      snprintf(str, 24, "Trial %d - Unknown", trial_num);
      break;
    case RESULTS:
      snprintf(str, 24, "Results");
      break;
  }
  
  menu_cell_basic_header_draw(ctx, cell_layer, str);
}

// This is the menu item draw callback where you specify what each item should look like
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char str[64];
  if (current_type == RESULTS) {
    if (cell_index->row < NUM_TRIALS * 2) {
      snprintf(str, 64, "Trial %d Time: %llu ms", cell_index->row, time_results[cell_index->row]);
    }
    else {
      snprintf(str, 64, "Trial %d Acc: %d", cell_index->row - (NUM_TRIALS * 2), accuracy_results[cell_index->row - (NUM_TRIALS * 2)]);
    }
  }
  else if (cell_index->row == 0) {
    switch (current_type) {
      case PRACTICE_KNOWN:
        snprintf(str, 64, "Select Item %d", practice_known);
        break;
      case TRIAL_KNOWN:
        snprintf(str, 64, "Select Item %d", known_trials[trial_num]);
        break;
      case PRACTICE_UNKNOWN:
      case TRIAL_UNKNOWN:
        snprintf(str, 64, "Select Item \"*****NEXT\"");
        break;
      default:
        break;
    }
  }
  else {
    switch (current_type) {
      case PRACTICE_UNKNOWN:
      case TRIAL_UNKNOWN:
        if (cell_index->row == unknown_trials[trial_num]) {
          snprintf(str, 64, "*****NEXT");
        }
        else {
          snprintf(str, 64, "Item");
        }
        break;
      case PRACTICE_KNOWN:
      case TRIAL_KNOWN:
        snprintf(str, 64, "Item %d", cell_index->row);
        break;
      default:
        break;
    }
  }
  menu_cell_title_draw(ctx, cell_layer, str);
}

// Here we capture when a user selects a menu item
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Handle transitions to different trial types
  uint64_t time;
  switch (current_type) {
    case PRACTICE_KNOWN:
      current_type = PRACTICE_UNKNOWN;
      break;
    case PRACTICE_UNKNOWN:
      current_type = TRIAL_KNOWN;
      trial_num = 0;
      break;
    case TRIAL_KNOWN:
      // Finish the trial
      time = stop_timer();
      time_results[trial_num] = time;
      accuracy_results[trial_num] = cell_index->row - known_trials[trial_num];
      
      // Transition to next trial
      if (trial_num == NUM_TRIALS - 1) {
        current_type = TRIAL_UNKNOWN;
        trial_num = 0;
      }
      else {
        trial_num++;
      }
      break;
    case TRIAL_UNKNOWN:
      // Finish the trial
      time = stop_timer();
      time_results[trial_num + NUM_TRIALS] = time;
      accuracy_results[trial_num + NUM_TRIALS] = cell_index->row - unknown_trials[trial_num];
      
      // Transition to next trial
      if (trial_num == NUM_TRIALS - 1) {
        current_type = RESULTS;
        trial_num = 0;
      }
      else {
        trial_num++;
      }
      break;
    case RESULTS:
      return;
  }
  
  // If we're starting a new timed trial, start the timer
  if (current_type == TRIAL_KNOWN || current_type == TRIAL_UNKNOWN) {
    start_timer();
  }
  
  MenuIndex idx;
  idx.section = 0;
  idx.row = 0;
  menu_layer_set_selected_index(menu_layer, idx, MenuRowAlignCenter, false);
  menu_layer_reload_data(menu_layer);
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
    .get_cell_height = menu_get_cell_height_callback,
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
  // Destroy the menu layer
  menu_layer_destroy(menu_layer);
}

int main(void) {
  // Setup the window
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true /* Animated */);

  app_event_loop();

  window_destroy(window);
}
