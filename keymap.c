# cat-try
#include QMK_KEYBOARD_H
#include "frames/animation_frames.h"   // raw idle_frames/tap_frames in PROGMEM

// ---------------- Layers ----------------
enum layer_names {
  _MA,
  _FN
};

enum custom_keycodes {
  KC_CUST = SAFE_RANGE,
};

// ---------------- Keymap ----------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_MA] = LAYOUT_ansi(
            KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, KC_HOME,
    KC_F13, KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS, KC_DEL,
    KC_F14, KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,  KC_PGUP,
    KC_F15, KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,          KC_UP,   KC_PGDN,
    KC_F16, KC_LCTL, KC_LGUI, KC_LALT,                   KC_SPC,                    MO(_FN), KC_RALT, KC_RCTL, KC_LEFT,          KC_DOWN, KC_RGHT
  ),
  [_FN] = LAYOUT_ansi(
             QK_BOOT,   KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,  KC_F10,  KC_F11,  KC_F12, _______,  KC_END,
    UG_TOGG, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______, _______,
    _______, _______, _______, _______,                   _______,                   _______, _______, _______, _______,          _______, _______
  ),
};

// ---------------- Encoder/RGB ----------------
static void change_RGB(bool clockwise) {
    bool shift = get_mods() & MOD_MASK_SHIFT;
    bool alt   = get_mods() & MOD_MASK_ALT;
    bool ctrl  = get_mods() & MOD_MASK_CTRL;

    if (clockwise) {
        if      (alt)  rgblight_increase_hue();
        else if (ctrl) rgblight_increase_val();
        else if (shift)rgblight_increase_sat();
        else           rgblight_step();
    } else {
        if      (alt)  rgblight_decrease_hue();
        else if (ctrl) rgblight_decrease_val();
        else if (shift)rgblight_decrease_sat();
        else           rgblight_step_reverse();
    }
}

bool encoder_update_user(uint8_t index, bool clockwise) {
  if (layer_state_is(_FN)) change_RGB(clockwise);
  else tap_code(clockwise ? KC_VOLU : KC_VOLD);
  return true;
}

// ---------------- Remote keyboard passthrough ----------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  process_record_remote_kb(keycode, record);
  switch (keycode) {
    case KC_CUST:
      if (record->event.pressed) {
        // your macro here
      }
      break;
  }
  return true; // do not block keys
}

void matrix_init_user(void) { matrix_init_remote_kb(); }
void matrix_scan_user(void) { matrix_scan_remote_kb(); }

// ---------------- OLED: Bongo Cat ----------------
#ifdef OLED_ENABLE

#define IDLE_FRAME_DURATION 200  // ms per idle frame

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_270;  // adjust if cat is sideways
}

static uint32_t anim_timer = 0;
static uint32_t anim_sleep = 0;
static uint8_t  idle_idx   = 0;
static bool     tap_toggle = false;

static void render_anim(void) {
    // keep OLED awake while typing
    if (get_current_wpm() != 0) {
        oled_on();
        anim_sleep = timer_read32();
    } else if (timer_elapsed32(anim_sleep) > OLED_TIMEOUT) {
        oled_off();
    }

    // advance + draw idle frame
    if (timer_elapsed32(anim_timer) > IDLE_FRAME_DURATION) {
        anim_timer = timer_read32();
        idle_idx = (idle_idx + 1) % NUM_IDLE_FRAMES;

        oled_clear();            // clear buffer each frame
        oled_set_cursor(0, 0);   // reset position
        oled_write_raw_P(idle_frames[idle_idx], NUM_OLED_BYTES);
    }
}

bool oled_task_user(void) {
    render_anim();

    // write WPM counter (after bitmap)
    oled_set_cursor(0, 14);
    char buf[6];
    uint8_t n = get_current_wpm();
    buf[5] = '\0';
    buf[4] = '0' + (n % 10);
    buf[3] = '0' + ((n / 10) % 10);
    buf[2] = '0' + (n / 100);
    buf[1] = '0';
    buf[0] = '>';
    oled_write_ln(buf, false);

    return false;
}
#endif // OLED_ENABLE

// ---------------- Tap animation trigger ----------------
#ifdef OLED_ENABLE
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) return false;

    if (record->event.pressed) {
        bool typing_key =
            (keycode >= KC_A   && keycode <= KC_0) ||
            (keycode >= KC_TAB && keycode <= KC_SLASH);

        if (typing_key) {
            tap_toggle = !tap_toggle;
            oled_on();
            oled_clear();           // clear before drawing tap frame
            oled_set_cursor(0, 0);
            oled_write_raw_P(tap_frames[tap_toggle], NUM_OLED_BYTES);
            anim_timer = timer_read32();
        }
    }
    return true;
}
#endif

