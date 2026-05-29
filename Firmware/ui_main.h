#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "alzguard_types.h"

namespace ReminderManager {
    uint8_t getAll(Reminder **out);
}

// ─── COLOUR PALETTE ───
#define COL_BG          lv_color_hex(0x0A0E1A)   
#define COL_CARD        lv_color_hex(0x141A2E)   
#define COL_ACCENT      lv_color_hex(0x00E5C8)   
#define COL_ACCENT2     lv_color_hex(0x7B61FF)   
#define COL_TEXT        lv_color_hex(0xEEF0F8)   
#define COL_SUBTEXT     lv_color_hex(0x8892B0)   
#define COL_ALERT       lv_color_hex(0xFF5252)   
#define COL_WARN        lv_color_hex(0xFFB800)   
#define COL_GREEN       lv_color_hex(0x00E676)   
#define COL_WATER       lv_color_hex(0x4ECDC4) 

namespace UI {

static lv_obj_t *scr_main = nullptr;
static lv_obj_t *tabview = nullptr;
static lv_obj_t *tab_watch = nullptr;
static lv_obj_t *tab_vitals = nullptr;
static lv_obj_t *tab_remind = nullptr;
static lv_obj_t *tab_status = nullptr;

static lv_obj_t *lbl_time = nullptr;
static lv_obj_t *lbl_date = nullptr;
static lv_obj_t *arc_battery = nullptr;
static lv_obj_t *lbl_bat_pct = nullptr;
static lv_obj_t *lbl_ble_icon = nullptr;
static lv_obj_t *lbl_wifi_icon = nullptr;

static lv_obj_t *arc_hr = nullptr;
static lv_obj_t *lbl_hr_val = nullptr;
static lv_obj_t *arc_spo2 = nullptr;
static lv_obj_t *lbl_spo2_val = nullptr;
static lv_obj_t *lbl_temp_val = nullptr;
static lv_obj_t *lbl_hum_val = nullptr;

static lv_obj_t *list_reminders = nullptr;
static lv_obj_t *lbl_conn_status= nullptr;
static lv_obj_t *lbl_fall_hist  = nullptr;
static lv_obj_t *bar_battery    = nullptr;

static lv_obj_t *modal_reminder = nullptr;
static lv_obj_t *modal_fall     = nullptr;
static lv_obj_t *toast_battery  = nullptr;

static lv_style_t style_screen;
static lv_style_t style_card;
static lv_style_t style_btn_primary;
static lv_style_t style_btn_danger;
static lv_style_t style_label_time;
static lv_style_t style_label_title;

static void initStyles() {
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, COL_BG);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_border_width(&style_screen, 0);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COL_CARD);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, 16);
    lv_style_set_border_width(&style_card, 0);
    lv_style_set_pad_all(&style_card, 12);

    lv_style_init(&style_btn_primary);
    lv_style_set_bg_color(&style_btn_primary, COL_ACCENT);
    lv_style_set_text_color(&style_btn_primary, COL_BG);
    lv_style_set_radius(&style_btn_primary, 12);
    lv_style_set_border_width(&style_btn_primary, 0);
    lv_style_set_pad_ver(&style_btn_primary, 14);

    lv_style_init(&style_btn_danger);
    lv_style_set_bg_color(&style_btn_danger, COL_ALERT);
    lv_style_set_text_color(&style_btn_danger, COL_TEXT);
    lv_style_set_radius(&style_btn_danger, 12);
    lv_style_set_border_width(&style_btn_danger, 0);
    lv_style_set_pad_ver(&style_btn_danger, 14);

    lv_style_init(&style_label_time);
    lv_style_set_text_color(&style_label_time, COL_TEXT);
    lv_style_set_text_font(&style_label_time, &lv_font_montserrat_48);

    lv_style_init(&style_label_title);
    lv_style_set_text_color(&style_label_title, COL_TEXT);
    lv_style_set_text_font(&style_label_title, &lv_font_montserrat_20);
}

static lv_obj_t* makeCard(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_add_style(c, &style_card, 0);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

static lv_obj_t* makeArc(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t size, lv_color_t col, int16_t val) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_value(arc, val);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1E2540), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, col, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 6, LV_PART_INDICATOR);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

static void buildWatchface(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, COL_BG, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab, 0, 0); // Remove invisible padding!

    lbl_time = lv_label_create(tab);
    lv_label_set_text(lbl_time, "12:00");
    lv_obj_add_style(lbl_time, &style_label_time, 0);
    lv_obj_align(lbl_time, LV_ALIGN_TOP_MID, 0, 45); // Centered, breathing room

    lbl_date = lv_label_create(tab);
    lv_label_set_text(lbl_date, "Jan 1, 2025");
    lv_obj_set_style_text_color(lbl_date, COL_SUBTEXT, 0);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_14, 0);
    lv_obj_align_to(lbl_date, lbl_time, LV_ALIGN_OUT_BOTTOM_MID, 0, 5); // Crisp gap

    lbl_ble_icon = lv_label_create(tab);
    lv_label_set_text(lbl_ble_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(lbl_ble_icon, COL_SUBTEXT, 0);
    lv_obj_set_pos(lbl_ble_icon, 15, 15);

    lbl_wifi_icon = lv_label_create(tab);
    lv_label_set_text(lbl_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(lbl_wifi_icon, COL_SUBTEXT, 0);
    lv_obj_set_pos(lbl_wifi_icon, 40, 15);

    arc_battery = makeArc(tab, 185, 15, 40, COL_GREEN, 80);
    lbl_bat_pct = lv_label_create(tab);
    lv_label_set_text(lbl_bat_pct, "80%");
    lv_obj_set_style_text_font(lbl_bat_pct, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_bat_pct, COL_TEXT, 0);
    lv_obj_align_to(lbl_bat_pct, arc_battery, LV_ALIGN_CENTER, 0, 0);

    // Perfectly symmetrical cards (Left: 15, Right: 125)
    lv_obj_t *card_hr = makeCard(tab, 15, 145, 100, 65);
    lv_obj_t *ic_hr = lv_label_create(card_hr);
    lv_label_set_text(ic_hr, LV_SYMBOL_CHARGE " HR");
    lv_obj_set_style_text_color(ic_hr, COL_ALERT, 0);
    lv_obj_set_style_text_font(ic_hr, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(ic_hr, 0, 0);

    lbl_hr_val = lv_label_create(card_hr);
    lv_label_set_text(lbl_hr_val, "-- bpm");
    lv_obj_set_style_text_color(lbl_hr_val, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_hr_val, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_hr_val, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *card_temp = makeCard(tab, 125, 145, 100, 65);
    lv_obj_t *ic_temp = lv_label_create(card_temp);
    lv_label_set_text(ic_temp, "TEMP");
    lv_obj_set_style_text_color(ic_temp, COL_WARN, 0);
    lv_obj_set_style_text_font(ic_temp, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(ic_temp, 0, 0);

    lbl_temp_val = lv_label_create(card_temp);
    lv_label_set_text(lbl_temp_val, "-- C");
    lv_obj_set_style_text_color(lbl_temp_val, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_temp_val, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_temp_val, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

static void buildVitals(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, COL_BG, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab, 0, 0);

    lv_obj_t *title = lv_label_create(tab);
    lv_label_set_text(title, "VITALS");
    lv_obj_set_style_text_color(title, COL_ACCENT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Symmetrical Arcs (Left: 20, Right: 130)
    arc_hr = makeArc(tab, 20, 50, 90, COL_ALERT, 0);
    lv_obj_t *lbl_hr_unit = lv_label_create(tab);
    lv_label_set_text(lbl_hr_unit, "bpm");
    lv_obj_set_style_text_color(lbl_hr_unit, COL_SUBTEXT, 0);
    lv_obj_set_style_text_font(lbl_hr_unit, &lv_font_montserrat_12, 0);
    lv_obj_align_to(lbl_hr_unit, arc_hr, LV_ALIGN_CENTER, 0, 10);

    lbl_hr_val = lv_label_create(tab);
    lv_label_set_text(lbl_hr_val, "--");
    lv_obj_set_style_text_color(lbl_hr_val, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_hr_val, &lv_font_montserrat_24, 0);
    lv_obj_align_to(lbl_hr_val, arc_hr, LV_ALIGN_CENTER, 0, -10);

    arc_spo2 = makeArc(tab, 130, 50, 90, COL_ACCENT2, 0);
    lv_obj_t *lbl_spo2_unit = lv_label_create(tab);
    lv_label_set_text(lbl_spo2_unit, "%");
    lv_obj_set_style_text_color(lbl_spo2_unit, COL_SUBTEXT, 0);
    lv_obj_set_style_text_font(lbl_spo2_unit, &lv_font_montserrat_12, 0);
    lv_obj_align_to(lbl_spo2_unit, arc_spo2, LV_ALIGN_CENTER, 0, 10);

    lbl_spo2_val = lv_label_create(tab);
    lv_label_set_text(lbl_spo2_val, "--");
    lv_obj_set_style_text_color(lbl_spo2_val, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_spo2_val, &lv_font_montserrat_24, 0);
    lv_obj_align_to(lbl_spo2_val, arc_spo2, LV_ALIGN_CENTER, 0, -10);

    // Symmetrical Cards (Left: 15, Right: 125)
    lv_obj_t *card_t = makeCard(tab, 15, 155, 100, 65);
    lv_obj_t *ic_t = lv_label_create(card_t);
    lv_label_set_text(ic_t, "TEMP");
    lv_obj_set_style_text_color(ic_t, COL_WARN, 0);
    lv_obj_set_style_text_font(ic_t, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(ic_t, 0, 0);

    lbl_temp_val = lv_label_create(card_t);
    lv_label_set_text(lbl_temp_val, "-- C");
    lv_obj_set_style_text_color(lbl_temp_val, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_temp_val, &lv_font_montserrat_22, 0);
    lv_obj_align(lbl_temp_val, LV_ALIGN_CENTER, 0, 8);

    lv_obj_t *card_h = makeCard(tab, 125, 155, 100, 65);
    lv_obj_t *ic_h = lv_label_create(card_h);
    lv_label_set_text(ic_h, "HUMIDITY");
    lv_obj_set_style_text_color(ic_h, COL_WATER, 0);
    lv_obj_set_style_text_font(ic_h, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(ic_h, 0, 0);

    lbl_hum_val = lv_label_create(card_h);
    lv_label_set_text(lbl_hum_val, "--%");
    lv_obj_set_style_text_color(lbl_hum_val, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_hum_val, &lv_font_montserrat_22, 0);
    lv_obj_align(lbl_hum_val, LV_ALIGN_CENTER, 0, 8);
}

static void buildReminders(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, COL_BG, 0);
    lv_obj_set_style_pad_all(tab, 0, 0);

    lv_obj_t *title = lv_label_create(tab);
    lv_label_set_text(title, LV_SYMBOL_BELL "  REMINDERS");
    lv_obj_set_style_text_color(title, COL_ACCENT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    list_reminders = lv_list_create(tab);
    lv_obj_set_size(list_reminders, 210, 200);
    lv_obj_align(list_reminders, LV_ALIGN_TOP_MID, 0, 45); // Perfectly centered width
    lv_obj_set_style_bg_color(list_reminders, COL_BG, 0);
    lv_obj_set_style_border_width(list_reminders, 0, 0);
}

static void buildStatus(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, COL_BG, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab, 0, 0);

    lv_obj_t *title = lv_label_create(tab);
    lv_label_set_text(title, "DEVICE STATUS");
    lv_obj_set_style_text_color(title, COL_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Switched to automatic centering for wide cards
    lv_obj_t *card_conn = lv_obj_create(tab);
    lv_obj_add_style(card_conn, &style_card, 0);
    lv_obj_set_size(card_conn, 210, 65);
    lv_obj_align(card_conn, LV_ALIGN_TOP_MID, 0, 45);
    lbl_conn_status = lv_label_create(card_conn);
    lv_label_set_text(lbl_conn_status, LV_SYMBOL_BLUETOOTH " BLE: Disconnected\n" LV_SYMBOL_WIFI " WiFi: Disconnected");
    lv_obj_set_style_text_color(lbl_conn_status, COL_SUBTEXT, 0);
    lv_obj_align(lbl_conn_status, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *card_bat = lv_obj_create(tab);
    lv_obj_add_style(card_bat, &style_card, 0);
    lv_obj_set_size(card_bat, 210, 75);
    lv_obj_align(card_bat, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_t *lbl_bat_ttl = lv_label_create(card_bat);
    lv_label_set_text(lbl_bat_ttl, "BATTERY");
    lv_obj_set_style_text_color(lbl_bat_ttl, COL_GREEN, 0);
    lv_obj_set_style_text_font(lbl_bat_ttl, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(lbl_bat_ttl, 0, 0);

    bar_battery = lv_bar_create(card_bat);
    lv_obj_set_size(bar_battery, 180, 14);
    lv_obj_align(bar_battery, LV_ALIGN_CENTER, 0, 4);
    lv_obj_set_style_bg_color(bar_battery, COL_GREEN, LV_PART_INDICATOR);
    lv_bar_set_range(bar_battery, 0, 100);

    lv_obj_t *card_fall = lv_obj_create(tab);
    lv_obj_add_style(card_fall, &style_card, 0);
    lv_obj_set_size(card_fall, 210, 55);
    lv_obj_align(card_fall, LV_ALIGN_TOP_MID, 0, 205);
    lv_obj_t *lbl_fall_ttl = lv_label_create(card_fall);
    lv_label_set_text(lbl_fall_ttl, LV_SYMBOL_WARNING " FALL HISTORY");
    lv_obj_set_style_text_color(lbl_fall_ttl, COL_ALERT, 0);
    lv_obj_set_style_text_font(lbl_fall_ttl, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(lbl_fall_ttl, 0, 0);

    lbl_fall_hist = lv_label_create(card_fall);
    lv_label_set_text(lbl_fall_hist, "No falls today");
    lv_obj_set_style_text_color(lbl_fall_hist, COL_SUBTEXT, 0);
    lv_obj_set_style_text_font(lbl_fall_hist, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_fall_hist, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

static void dismissReminderCb(lv_event_t *e) {
    if (modal_reminder) { lv_obj_del(modal_reminder); modal_reminder = nullptr; }
    noTone(42); 
}

void showReminderAlert(const char *label, ReminderType type) {
    if (modal_reminder) lv_obj_del(modal_reminder);
    modal_reminder = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal_reminder, 240, 280);
    lv_obj_set_style_bg_color(modal_reminder, lv_color_hex(0x1A0A10), 0);
    lv_obj_set_style_border_width(modal_reminder, 0, 0);

    lv_obj_t *lbl_msg = lv_label_create(modal_reminder);
    lv_label_set_text(lbl_msg, label);
    lv_obj_set_style_text_color(lbl_msg, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_msg, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_msg, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn = lv_btn_create(modal_reminder);
    lv_obj_add_style(btn, &style_btn_primary, 0);
    lv_obj_set_size(btn, 180, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn, dismissReminderCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "GOT IT " LV_SYMBOL_OK);
    lv_obj_center(btn_lbl);
}

static void fallSafeCb(lv_event_t *e) {
    if (modal_fall) { lv_obj_del(modal_fall); modal_fall = nullptr; }
    noTone(42);
}

void showFallAlert() {
    if (modal_fall) lv_obj_del(modal_fall);
    modal_fall = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal_fall, 240, 280);
    lv_obj_set_style_bg_color(modal_fall, lv_color_hex(0x1A0000), 0);
    lv_obj_set_style_border_width(modal_fall, 0, 0);

    lv_obj_t *lbl_title = lv_label_create(modal_fall);
    lv_label_set_text(lbl_title, "FALL DETECTED");
    lv_obj_set_style_text_color(lbl_title, COL_ALERT, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_22, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *btn = lv_btn_create(modal_fall);
    lv_obj_add_style(btn, &style_btn_danger, 0);
    lv_obj_set_size(btn, 180, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn, fallSafeCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "I AM SAFE");
    lv_obj_center(btn_lbl);
}

void dismissFallAlert() {
    if (modal_fall) { lv_obj_del(modal_fall); modal_fall = nullptr; }
}

void showBatteryWarning(float pct) {
    if (toast_battery) return;
    toast_battery = lv_obj_create(lv_scr_act());
    lv_obj_set_size(toast_battery, 220, 44);
    lv_obj_align(toast_battery, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(toast_battery, lv_color_hex(0x2A1500), 0);
    
    char buf[48]; snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING " Low Battery: %.0f%%", pct);
    lv_obj_t *lbl = lv_label_create(toast_battery);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, COL_WARN, 0);
    lv_obj_center(lbl);

    lv_timer_create([](lv_timer_t *t) {
        if (toast_battery) { lv_obj_del(toast_battery); toast_battery = nullptr; }
        lv_timer_del(t);
    }, 5000, NULL);
}

void updateVitals(const DeviceState &s) {
    char buf[32];
    if (lbl_time) { snprintf(buf, sizeof(buf), "%02d:%02d", s.hour, s.minute); lv_label_set_text(lbl_time, buf); }
    if (lbl_date) {
        const char* months[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        uint8_t m = constrain(s.month, 1, 12);
        snprintf(buf, sizeof(buf), "%s %d, 20%02d", months[m], s.day, s.year);
        lv_label_set_text(lbl_date, buf);
    }
    if (arc_battery) { lv_arc_set_value(arc_battery, (int16_t)s.batteryPct); }
    if (lbl_ble_icon) lv_obj_set_style_text_color(lbl_ble_icon, s.bleConnected ? COL_ACCENT : COL_SUBTEXT, 0);
    if (lbl_wifi_icon) lv_obj_set_style_text_color(lbl_wifi_icon, s.wifiConnected ? COL_ACCENT : COL_SUBTEXT, 0);

    if (arc_hr && s.heartRate > 0) { lv_arc_set_value(arc_hr, constrain((int)(s.heartRate - 40) * 100 / 140, 0, 100)); }
    if (lbl_hr_val && s.heartRate > 0) { snprintf(buf, sizeof(buf), "%.0f", s.heartRate); lv_label_set_text(lbl_hr_val, buf); }
    if (arc_spo2 && s.spo2 > 0) { lv_arc_set_value(arc_spo2, constrain((int)(s.spo2 - 85) * 100 / 15, 0, 100)); }
    if (lbl_spo2_val && s.spo2 > 0) { snprintf(buf, sizeof(buf), "%.0f", s.spo2); lv_label_set_text(lbl_spo2_val, buf); }
    if (lbl_temp_val) { snprintf(buf, sizeof(buf), "%.1f C", s.temperature); lv_label_set_text(lbl_temp_val, buf); }
    if (lbl_hum_val) { snprintf(buf, sizeof(buf), "%.0f%%", s.humidity); lv_label_set_text(lbl_hum_val, buf); }
    if (bar_battery) lv_bar_set_value(bar_battery, (int32_t)s.batteryPct, LV_ANIM_ON);
}

void refreshReminders() {
    if (!list_reminders) return;
    lv_obj_clean(list_reminders);
    Reminder *rems;
    uint8_t count = ReminderManager::getAll(&rems);

    if (count == 0) {
        lv_obj_t *empty = lv_label_create(list_reminders);
        lv_label_set_text(empty, "No reminders set");
        lv_obj_set_style_text_color(empty, COL_SUBTEXT, 0);
        lv_obj_center(empty);
        return;
    }
    for (uint8_t i = 0; i < count; i++) {
        lv_obj_t *row = lv_list_add_btn(list_reminders, LV_SYMBOL_BELL, rems[i].label);
        lv_obj_set_style_bg_color(row, COL_CARD, 0);
        lv_obj_set_style_border_width(row, 0, 0);
    }
}

void begin() {
    initStyles();
    lv_obj_t *scr = lv_scr_act();
    lv_obj_add_style(scr, &style_screen, 0);
    
    tabview = lv_tabview_create(scr, LV_DIR_LEFT, 0);
    lv_obj_set_size(tabview, 240, 280);
    lv_obj_set_style_bg_color(tabview, COL_BG, 0);
    
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_height(tab_btns, 0);
    lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);
    
    tab_watch  = lv_tabview_add_tab(tabview, "Watch");
    tab_vitals = lv_tabview_add_tab(tabview, "Vitals");
    tab_remind = lv_tabview_add_tab(tabview, "Reminders");
    tab_status = lv_tabview_add_tab(tabview, "Status");

    buildWatchface(tab_watch);
    buildVitals(tab_vitals);
    buildReminders(tab_remind);
    buildStatus(tab_status);
    
    lv_tabview_set_act(tabview, 0, LV_ANIM_OFF);
    refreshReminders();
}
} // namespace UI