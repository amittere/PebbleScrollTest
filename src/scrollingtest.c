#include "scrollingtest.h"

void start_timer(time_t *sec, uint16_t *msec) {
  time_ms(sec, msec);
}

uint64_t stop_timer(time_t *sec, uint16_t *msec) {
  time_t elapsed_sec;
  uint16_t elapsed_msec;
  time_ms(&elapsed_sec, &elapsed_msec);
  uint64_t time1 = *sec * 1000 + *msec;
  uint64_t time2 = elapsed_sec * 1000 + elapsed_msec;
  return time2 - time1;
}

void select_item() {
  uint64_t elapsed = stop_timer(&select_timeout_sec, &select_timeout_msec);
  if (elapsed > SELECT_TIME_THR) {
    MenuIndex idx = menu_layer_get_selected_index(menu_layer);
    menu_select_callback(menu_layer, &idx, NULL);
  }
}

void accel_data_handler(AccelData *data, uint32_t num_samples) {
  if (!scrollEnabled) return;
  
  int avgX = 0, avgY = 0, avgZ = 0;
  uint64_t latest_time = 0;
  AccelData d = data[0];
  if (d.did_vibrate) return;
  avgX = d.x;
  avgY = d.y;
  avgZ = d.z;
  latest_time = d.timestamp;
  
  if (!baselineSet) {
    baselineX = avgX;
    baselineY = avgY;
    baselineZ = avgZ;
    baselineSet = true;
    return;
  }
  
  // Selection
  if (avgX - baselineX > SELECT_THR ||
      avgX - baselineX < -SELECT_THR) {
    select_item();
  }
  // Fast downward scroll
  else if (avgY - baselineY > FAST_SCROLL_THR) {
    if (current_speed != FAST_SCROLL_SPEED &&
        current_speed != XTRA_FAST_SCROLL_SPEED) {
      current_speed = FAST_SCROLL_SPEED;
      fast_scroll_start = latest_time;
    }
    
    if (avgY - baselineY > XTRA_FAST_SCROLL_THR &&
        latest_time - fast_scroll_start >= XTRA_FAST_TIME_THR) {
      current_speed = XTRA_FAST_SCROLL_SPEED;
    }
    
    count++;
    if (count >= current_speed) {
      count -= current_speed;
      menu_layer_set_selected_next(menu_layer, false, MenuRowAlignCenter, true);
    }
  }
  // Slow downward scroll
  else if (avgY - baselineY > SLOW_SCROLL_THR) {
    current_speed = SLOW_SCROLL_SPEED;
    fast_scroll_start = 0;
    count++;
    if (count >= current_speed) {
      count -= current_speed;
      menu_layer_set_selected_next(menu_layer, false, MenuRowAlignCenter, true);
    }
  }
  // Fast upward scroll
  else if (avgY - baselineY < -FAST_SCROLL_THR) {
    if (current_speed != FAST_SCROLL_SPEED &&
        current_speed != XTRA_FAST_SCROLL_SPEED) {
      current_speed = FAST_SCROLL_SPEED;
      fast_scroll_start = latest_time;
    }
    
    if (avgY - baselineY < -XTRA_FAST_SCROLL_THR &&
        latest_time - fast_scroll_start >= XTRA_FAST_TIME_THR) {
      current_speed = XTRA_FAST_SCROLL_SPEED;
    }
    
    count++;
    if (count >= current_speed) {
      count -= current_speed;
      menu_layer_set_selected_next(menu_layer, true, MenuRowAlignCenter, true);
    }
  }
  // Slow upward scroll
  else if (avgY - baselineY < -SLOW_SCROLL_THR) {
    current_speed = SLOW_SCROLL_SPEED;
    fast_scroll_start = 0;
    count++;
    if (count >= current_speed) {
      count -= current_speed;
      menu_layer_set_selected_next(menu_layer, true, MenuRowAlignCenter, true);        
    }
  }
  // No scroll
  else {
    current_speed = 0;
    count = 0;
    fast_scroll_start = 0;
  }
}

void enable_tilt_scroll() {
  // Subscribe to the accelerometer service
  scrollEnabled = true;
  accel_data_service_subscribe(1, &accel_data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);
}

void disable_tilt_scroll() {
  accel_data_service_unsubscribe();
  scrollEnabled = false;
}

// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (current_type) {
    case SETUP:
      return 2;
    case RESULTS:
      return NUM_TRIALS * 4;
    default:
      return NUM_MENU_ITEMS;
  }
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (current_type == SETUP || 
      current_type == RESULTS || 
      cell_index->row == 0) {
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
    case SETUP:
      snprintf(str, 24, "Setup");
      break;
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
  if (current_type == SETUP) {
    if (cell_index->row == 0) {
      snprintf(str, 64, "ENABLE Tilt Scroll");
    }
    else if (cell_index->row == 1) {
      snprintf(str, 64, "DISABLE Tilt Scroll");
    }
  }
  else if (current_type == RESULTS) {
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
    case SETUP:
      if (cell_index->row == 0) {
        enable_tilt_scroll();
      }
      current_type = PRACTICE_KNOWN;
      break;
    case PRACTICE_KNOWN:
      current_type = PRACTICE_UNKNOWN;
      break;
    case PRACTICE_UNKNOWN:
      current_type = TRIAL_KNOWN;
      trial_num = 0;
      break;
    case TRIAL_KNOWN:
      // Finish the trial
      time = stop_timer(&time_sec, &time_msec);
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
      time = stop_timer(&time_sec, &time_msec);
      time_results[trial_num + NUM_TRIALS] = time;
      accuracy_results[trial_num + NUM_TRIALS] = cell_index->row - unknown_trials[trial_num];
      
      // Transition to next trial
      if (trial_num == NUM_TRIALS - 1) {
        current_type = RESULTS;
        trial_num = 0;
        
        // Log the results
        char str[256];
        int cx = 0;
        int i;
        for (i = 0; i < NUM_TRIALS; i++) {
          cx += snprintf(str + cx, 256 - cx, "%llu,", time_results[i]);
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, str);
        
        cx = 0;
        memset(str, 0, 256);
        for (i = NUM_TRIALS; i < NUM_TRIALS * 2; i++) {
          cx += snprintf(str + cx, 256 - cx, "%llu,", time_results[i]);
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, str);
        
        cx = 0;
        memset(str, 0, 256);
        for (i = 0; i < NUM_TRIALS; i++) {
          cx += snprintf(str + cx, 512 - cx, "%d,", accuracy_results[i]);
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, str);
        
        cx = 0;
        memset(str, 0, 256);
        for (i = NUM_TRIALS; i < NUM_TRIALS * 2; i++) {
          cx += snprintf(str + cx, 512 - cx, "%d,", accuracy_results[i]);
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, str);
      }
      else {
        trial_num++;
      }
      break;
    case RESULTS:
      return;
  }
  
  // Start tilt select timer
  start_timer(&select_timeout_sec, &select_timeout_msec);
  
  // If we're starting a new timed trial, start the timer
  if (current_type == TRIAL_KNOWN || current_type == TRIAL_UNKNOWN) {
    start_timer(&time_sec, &time_msec);
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
  
  if (scrollEnabled) disable_tilt_scroll();
  window_destroy(window);
}
