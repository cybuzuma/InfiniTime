#include "displayapp/screens/ApplicationList.h"
#include <lvgl/lvgl.h>
#include "displayapp/DisplayApp.h"
#include <nrf_log.h>

using namespace Pinetime::Applications::Screens;

namespace {
  void lv_update_task(struct _lv_task_t* task) {
    auto* user_data = static_cast<ApplicationList*>(task->user_data);
    user_data->UpdateScreen();
  }

  void event_handler(lv_obj_t* obj, lv_event_t event) {
    static bool longPress = false;
    if (event == LV_EVENT_LONG_PRESSED) {
      longPress = true;
    }
    //NRF_LOG_INFO("Applist Event %i", event);
    ApplicationList* screen = static_cast<ApplicationList*>(obj->user_data);
    auto* eventDataPtr = (uint32_t*) lv_event_get_data();
    if (eventDataPtr == nullptr) {
      return;
    }
    uint32_t eventData = *eventDataPtr;
    if (event == LV_EVENT_VALUE_CHANGED) {
      if (longPress) {
        screen->OnLongPressed(obj, eventData);
      } else {
        screen->OnValueChangedEvent(obj, eventData);
      }
      longPress = false;
    }
    return;
  }
}

ApplicationList::ApplicationList(Pinetime::Applications::DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 Pinetime::Controllers::Battery& batteryController,
                                 Pinetime::Controllers::Ble& bleController,
                                 Controllers::DateTime& dateTimeController)
  : Screen(app),
    settingsController {settingsController},
    batteryController {batteryController},
    bleController {bleController},
    dateTimeController {dateTimeController},
    dateTimeController {dateTimeController}{
  NRF_LOG_INFO("Applist Ctor");

  settingsController.SetAppMenu(0);

  // Time
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_align(label_time, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Battery
  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, -8, 0);

  uint8_t numScreens = 3;
  uint8_t screenID = 1;

  if (numScreens > 1) {
    pageIndicatorBasePoints[0].x = LV_HOR_RES - 1;
    pageIndicatorBasePoints[0].y = 0;
    pageIndicatorBasePoints[1].x = LV_HOR_RES - 1;
    pageIndicatorBasePoints[1].y = LV_VER_RES;

    pageIndicatorBase = lv_line_create(lv_scr_act(), nullptr);
    lv_obj_set_style_local_line_width(pageIndicatorBase, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 3);
    lv_obj_set_style_local_line_color(pageIndicatorBase, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x111111));
    lv_line_set_points(pageIndicatorBase, pageIndicatorBasePoints, 2);

    const uint16_t indicatorSize = LV_VER_RES / numScreens;
    const uint16_t indicatorPos = indicatorSize * screenID;

    pageIndicatorPoints[0].x = LV_HOR_RES - 1;
    pageIndicatorPoints[0].y = indicatorPos;
    pageIndicatorPoints[1].x = LV_HOR_RES - 1;
    pageIndicatorPoints[1].y = indicatorPos + indicatorSize;

    pageIndicator = lv_line_create(lv_scr_act(), nullptr);
    lv_obj_set_style_local_line_width(pageIndicator, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 3);
    lv_obj_set_style_local_line_color(pageIndicator, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xb0, 0xb0, 0xb0));
    lv_line_set_points(pageIndicator, pageIndicatorPoints, 2);
  }

  updateButtonMap();

  btnm1 = lv_btnmatrix_create(lv_scr_act(), nullptr);
  lv_btnmatrix_set_map(btnm1, btnmMap);
  lv_obj_set_size(btnm1, LV_HOR_RES - 16, LV_VER_RES - 60);
  lv_obj_align(btnm1, NULL, LV_ALIGN_CENTER, 0, 10);

  lv_obj_set_style_local_radius(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, 20);
  lv_obj_set_style_local_bg_opa(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, LV_OPA_50);
  lv_obj_set_style_local_bg_color(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, LV_COLOR_AQUA);
  lv_obj_set_style_local_bg_opa(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DISABLED, LV_OPA_50);
  lv_obj_set_style_local_bg_color(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DISABLED, lv_color_hex(0x111111));
  lv_obj_set_style_local_pad_all(btnm1, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_inner(btnm1, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 10);

  enableButtons();

  btnm1->user_data = this;
  lv_obj_set_event_cb(btnm1, event_handler);

  taskUpdate = lv_task_create(lv_update_task, 5000, LV_TASK_PRIO_MID, this);

  UpdateScreen();
}

uint8_t ApplicationList::getStartAppIndex(uint8_t page){
  uint8_t enabled = 0;
  uint8_t appIndex = 0;

  //TODO: check to not overrung applist
  while(!(enabled >= page * 6) || (!isShown(applications[appIndex]))){
    if (!isShown(applications[appIndex])){
      appIndex++;
      continue;
    }
    enabled++;
    appIndex++;
  }
  return appIndex;
}

void ApplicationList::updateButtonMap() {
  uint8_t page = 1;
  uint8_t btIndex = 0;
  uint8_t appIndex = getStartAppIndex(page);
  

  //need to create 6 buttons + 1 newline =7
  while (btIndex < 7) {
    //Third button is newline
    if (btIndex == 3){
      btnmMap[btIndex++] = "\n";
      continue;
    }
    //we ran out of apps, fill with empty buttons
    if (appIndex >= applications.size()){
      btnmMap[btIndex++] = " ";
      continue;
    }
    //Skip disabled or invalid apps
    if (!isShown(applications[appIndex])) {
      appIndex++;
      continue;
    }
    //set Icon
    btnmMap[btIndex] = applications[appIndex].icon;
    btIndex++;
    appIndex++;
  }
  //close buttonMap
  btnmMap[btIndex] = "";
}

void ApplicationList::enableButtons() {
  for (uint8_t i = 0; i < 7; i++) {
    lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_CLICK_TRIG);
    lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_NO_REPEAT);
    if (btnmMap[i] == " ") {
      uint8_t buttonId = i;
      if (i > 3) {
        buttonId--;
      }
      NRF_LOG_INFO("Applist: Disabled map %i", buttonId);
      lv_btnmatrix_set_btn_ctrl(btnm1, buttonId, LV_BTNMATRIX_CTRL_DISABLED);
    }
  }
}

void ApplicationList::UpdateScreen() {
  lv_label_set_text(label_time, dateTimeController.FormattedTime().c_str());
  batteryIcon.SetBatteryPercentage(batteryController.PercentRemaining());
}

ApplicationList::~ApplicationList() {
  lv_task_del(taskUpdate);
  lv_obj_clean(lv_scr_act());
}

bool ApplicationList::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // return screens.OnTouchEvent(event);
  return false;
}

bool ApplicationList::isShown(const ApplicationList::Applications &app) const {
  return (app.application != Apps::None) && (app.enabled);
}

ApplicationList::Applications * ApplicationList::getAppOnButton(uint8_t buttonNr){
  uint8_t page = 1;
  uint8_t enabledAppNr = 0;
  for (uint8_t i =  getStartAppIndex(page); i < applications.size(); i++){
    if (!isShown(applications[i])){
      NRF_LOG_INFO("Applist: Ignoring %i", i);
      continue;
    }
    if ( enabledAppNr == buttonNr){
      NRF_LOG_INFO("Applist: found App %i", i);
      return &applications[i];
    }
    enabledAppNr++;
  }
  return nullptr;
}


void ApplicationList::OnLongPressed(lv_obj_t* obj, uint32_t buttonId) {
  if (obj != btnm1) {
    NRF_LOG_INFO("Wrong object");
    return;
  }

  NRF_LOG_INFO("Applist: App %i disabled", buttonId);
  Applications * app = getAppOnButton(buttonId);
  if (app == nullptr){
    return;
  }
  app->enabled = false;
  updateButtonMap();
  enableButtons();
}

void ApplicationList::OnValueChangedEvent(lv_obj_t* obj, uint32_t buttonId) {
  if (obj != btnm1)
    return;
  Applications * app = getAppOnButton(buttonId);
  if (app == nullptr){
    return;
  }
  Screen::app->StartApp(app->application, DisplayApp::FullRefreshDirections::Up);
  running = false;
}
