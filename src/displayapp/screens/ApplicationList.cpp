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
    ApplicationList* screen = static_cast<ApplicationList*>(obj->user_data);
    if (event == LV_EVENT_LONG_PRESSED) {
      screen->OnLongHold();
    }
    // NRF_LOG_INFO("Applist Event %i", event);
    auto* eventDataPtr = (uint32_t*) lv_event_get_data();
    if (eventDataPtr == nullptr) {
      return;
    }
    uint32_t eventData = *eventDataPtr;
    if (event == LV_EVENT_VALUE_CHANGED) {
      screen->OnValueChangedEvent(obj, eventData);
    }
    return;
  }
}

ApplicationList::ApplicationList(Pinetime::Applications::DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 Pinetime::Controllers::Battery& batteryController,
                                 Pinetime::Controllers::Ble& bleController,
                                 Controllers::DateTime& dateTimeController,
                                 Controllers::MotorController& motorController,
                                 bool addingApps)
  : Screen(app),
    settingsController {settingsController},
    batteryController {batteryController},
    bleController {bleController},
    dateTimeController {dateTimeController},
    motorController {motorController},
    addingApps {addingApps},
    pageIndicator(0, 1) {

  // Time
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_align(label_time, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Battery
  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, -8, 0);

  if (!addingApps) {
    page = settingsController.GetAppMenu();
  }

  // page indicator
  pageIndicator.Create();
  calculatePages();
  pageIndicator.Update(page, pages);

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

uint8_t ApplicationList::getStartAppIndex(uint8_t page) {
  uint8_t enabled = 0;
  uint8_t appIndex = 0;

  // TODO: check to not overrun applist
  while (appIndex < applications.size() && (!(enabled >= page * 6) || (!isShown(appIndex)))) {
    if (!isShown(appIndex)) {
      appIndex++;
      continue;
    }
    enabled++;
    appIndex++;
  }
  return appIndex;
}

void ApplicationList::updateButtonMap() {
  uint8_t btIndex = 0;
  uint8_t appIndex = getStartAppIndex(page);

  // need to create 6 buttons + 1 newline =7
  while (btIndex < 7) {
    // Third button is newline
    if (btIndex == 3) {
      btnmMap[btIndex++] = "\n";
      continue;
    }
    // we ran out of apps, fill with empty buttons
    if (appIndex >= applications.size()) {
      NRF_LOG_INFO("filling map %i", btIndex);
      btnmMap[btIndex++] = " ";
      continue;
    }
    // Skip disabled or invalid apps
    if (!isShown(appIndex)) {
      appIndex++;
      continue;
    }
    // set Icon
    btnmMap[btIndex] = applications[appIndex].icon;
    btIndex++;
    appIndex++;
  }
  // close buttonMap
  btnmMap[btIndex] = "";
}

void ApplicationList::enableButtons() {
  NRF_LOG_INFO("enableButtons");
  for (uint8_t i = 0; i < 7; i++) {
    lv_btnmatrix_clear_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_DISABLED);
    lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_CLICK_TRIG);
    lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_NO_REPEAT);
    if (strcmp(btnmMap[i], " ") == 0) {
      uint8_t buttonId = i;
      if (i > 3) {
        buttonId--;
      }
      NRF_LOG_INFO("disabling button %i", buttonId);
      lv_btnmatrix_set_btn_ctrl(btnm1, buttonId, LV_BTNMATRIX_CTRL_DISABLED);
    }
  }
}

void ApplicationList::UpdateScreen() {
  if (addingApps) {
    lv_label_set_text(label_time, "add App");
  } else {
    lv_label_set_text(label_time, dateTimeController.FormattedTime().c_str());
  }
  batteryIcon.SetBatteryPercentage(batteryController.PercentRemaining());
}

ApplicationList::~ApplicationList() {
  lv_task_del(taskUpdate);
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

bool ApplicationList::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  switch (event) {
    case TouchEvents::SwipeDown:
      if (page > 0) {
        page--;
      } else {
        return false;
      }
      if (!addingApps) {
        settingsController.SetAppMenu(page);
      }
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
      updateButtonMap();
      enableButtons();
      pageIndicator.Update(page, pages);
      return true;
    case TouchEvents::SwipeUp:
      if (page + 1 < pages) {
        page++;
      } else {
        return false;
      }
      if (!addingApps) {
        settingsController.SetAppMenu(page);
      }
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Up);
      updateButtonMap();
      enableButtons();
      pageIndicator.Update(page, pages);
      return true;
  }
  // return screens.OnTouchEvent(event);
  return false;
}

bool ApplicationList::isShown(uint8_t id) const {
  uint64_t currentState = settingsController.GetAppDisabled();
  bool disabled = (currentState >> id) & 1;
  if (addingApps) {
    disabled = !disabled;
  }
  return (applications[id].application != Apps::None) && (!disabled);
}

uint8_t ApplicationList::getAppIdOnButton(uint8_t buttonNr) {
  uint8_t enabledAppNr = 0;
  for (uint8_t i = getStartAppIndex(page); i < applications.size(); i++) {
    if (!isShown(i)) {
      continue;
    }
    if (enabledAppNr == buttonNr) {
      return i;
    }
    enabledAppNr++;
  }
  return 0;
}

void ApplicationList::OnLongHold() {
  longPressed = true;
  motorController.RunForDuration(50);
}

void ApplicationList::toggleApp(uint8_t id) {
  if (applications[id].application == Apps::LauncherAddApp) {
    return;
  }

  NRF_LOG_INFO("toggling app %i", id);

  uint64_t currentState = settingsController.GetAppDisabled();

  currentState ^= (1L << id);

  settingsController.SetAppDisabled(currentState);
}

void ApplicationList::OnValueChangedEvent(lv_obj_t* obj, uint32_t buttonId) {
  if (obj != btnm1)
    return;
  uint8_t appId = getAppIdOnButton(buttonId);
  if (longPressed && !addingApps) {
    toggleApp(appId);
    calculatePages();
    updateButtonMap();
    enableButtons();
  } else {
    if (addingApps) {
      toggleApp(appId);
      calculatePages();
      updateButtonMap();
      enableButtons();
    } else {
      Applications* app = &applications[appId];
      Screen::app->StartApp(app->application, DisplayApp::FullRefreshDirections::Up);
      running = false;
    }
  }
  longPressed = false;
}

void ApplicationList::calculatePages() {
  uint8_t enabledApps = 0;
  for (uint8_t i = 0; i < applications.size(); i++) {
    if (isShown(i)) {
      enabledApps++;
    }
  }
  if (enabledApps == 0) {
    page = 0;
    pages = 1;
    pageIndicator.Update(page, 1);
    // This does not work, when app is launched without anything to show
    // app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
    // app->ReturnToPreviousApp();
    NRF_LOG_INFO("Zero pages");
    return;
  }
  pages = (enabledApps + 5) / 6;
  if (page >= pages) {
    page = pages - 1;
    app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
  }
  pageIndicator.Update(page, pages);
}