#include <pebble.h>
  
#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ITEMS 101
#define CELL_HEIGHT 32
#define TITLE_HEIGHT 64

#define NUM_TRIALS 10
  
// Scrolling constants
#define SLOW_SCROLL_THR 150
#define FAST_SCROLL_THR 400
#define XTRA_FAST_SCROLL_THR 500
#define SLOW_SCROLL_SPEED 50
#define FAST_SCROLL_SPEED 10
#define XTRA_FAST_SCROLL_SPEED 5
#define XTRA_FAST_TIME_THR 2000
#define SELECT_THR 400
#define SELECT_TIME_THR 2000

// Enum specifying the current type of trial.
// Order of trials is the order specified in the enum.
typedef enum TrialType {
  SETUP, PRACTICE_KNOWN, PRACTICE_UNKNOWN, TRIAL_KNOWN, TRIAL_UNKNOWN, RESULTS
} TrialType;

// Trial indices
//static int known_trials[NUM_TRIALS] = { 10 };
//static int unknown_trials[NUM_TRIALS] = { 10 };
static int known_trials[NUM_TRIALS] = { 10, 51, 24, 3, 36, 49, 75, 98, 66, 87 };
static int unknown_trials[NUM_TRIALS] = { 36, 98, 10, 3, 51, 49, 87, 66, 75, 24 };
static int practice_known = 28;
static int practice_unknown = 51;

static TrialType current_type = SETUP;
static int trial_num = 0;
static time_t time_sec;
static uint16_t time_msec;
static uint64_t time_results[NUM_TRIALS * 2];
static int accuracy_results[NUM_TRIALS * 2];

// Baseline fields
static bool baselineSet = false;
static short int baselineX = 0;
static short int baselineY = 0;
static short int baselineZ = 0;

// Scrolling fields
static bool scrollEnabled = false;
static uint64_t fast_scroll_start = 0;
static int current_speed = 0;
static int count = 0;
static time_t select_timeout_sec;
static uint16_t select_timeout_msec;

static Window *window;

// This is a menu layer
// You have more control than with a simple menu layer
static MenuLayer *menu_layer;

void start_timer(time_t *sec, uint16_t *msec);
uint64_t stop_timer(time_t *sec, uint16_t *msec);

void enable_tilt_scroll();
void disable_tilt_scroll();
void accel_data_handler(AccelData *data, uint32_t num_samples);

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
void select_item();
void window_load(Window *window);
void window_unload(Window *window);
  