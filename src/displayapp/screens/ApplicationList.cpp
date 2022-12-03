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

  void btn_handler(lv_obj_t* obj, lv_event_t event) {
    ApplicationList::Overlay* screen = static_cast<ApplicationList::Overlay*>(obj->user_data);
    screen->HandleButtons(obj, event);
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

  // Adding apps always starts at page 0
  if (!addingApps) {
    page = settingsController.GetAppMenu();
  }

  // page indicator
  pageIndicator.Create();
  CalculatePages();
  pageIndicator.Update(page, pages);

  // Prepare the string map for the button matrix
  UpdateButtonMap();

  // vreate the button matrix
  btnm1 = lv_btnmatrix_create(lv_scr_act(), nullptr);
  lv_btnmatrix_set_map(btnm1, btnmMap);
  lv_obj_set_size(btnm1, LV_HOR_RES - 8, LV_VER_RES - 30);
  lv_obj_align(btnm1, NULL, LV_ALIGN_IN_LEFT_MID, 0, 15);

  lv_obj_set_style_local_radius(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, 20);
  lv_obj_set_style_local_bg_opa(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, LV_OPA_50);
  lv_obj_set_style_local_bg_color(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, LV_COLOR_AQUA);
  lv_obj_set_style_local_bg_opa(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DISABLED, LV_OPA_50);
  lv_obj_set_style_local_bg_color(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DISABLED, lv_color_hex(0x111111));
  lv_obj_set_style_local_pad_all(btnm1, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_inner(btnm1, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 10);

  // Update button matrix buttons' state
  EnableButtons();

  btnm1->user_data = this;
  lv_obj_set_event_cb(btnm1, event_handler);

  taskUpdate = lv_task_create(lv_update_task, 5000, LV_TASK_PRIO_MID, this);

  UpdateScreen();
}

ApplicationList::~ApplicationList() {
  lv_task_del(taskUpdate);
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

uint8_t ApplicationList::GetStartAppIndex(uint8_t page) {
  uint8_t enabled = 0;
  uint8_t appIndex = 0;

  // TODO: check to not overrun applist
  while (appIndex < applications.size() && (!(enabled >= page * 6) || (!IsShown(appIndex)))) {
    if (!IsShown(appIndex)) {
      appIndex++;
      continue;
    }
    enabled++;
    appIndex++;
  }
  return appIndex;
}

void ApplicationList::UpdateButtonMap() {
  uint8_t btIndex = 0;
  uint8_t appIndex = GetStartAppIndex(page);

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
    if (!IsShown(appIndex)) {
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

void ApplicationList::EnableButtons() {
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
      UpdateButtonMap();
      EnableButtons();
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
      UpdateButtonMap();
      EnableButtons();
      pageIndicator.Update(page, pages);
      return true;
  }
  return false;
}

bool ApplicationList::IsShown(uint8_t id) const {
  uint64_t currentState = settingsController.GetAppDisabled();

  if (applications[id].application == Apps::LauncherAddApp) {
    if (addingApps) {
      return false;
    } else {
      return currentState != 0;
    }
  }
  
  bool disabled = (currentState >> id) & 1;
  if (addingApps) {
    disabled = !disabled;
  }
  return (applications[id].application != Apps::None) && (!disabled);
}

uint8_t ApplicationList::GetAppIdOnButton(uint8_t buttonNr) {
  uint8_t enabledAppNr = 0;
  for (uint8_t i = GetStartAppIndex(page); i < applications.size(); i++) {
    if (!IsShown(i)) {
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

void ApplicationList::ToggleApp(uint8_t id) {
  if (applications[id].application == Apps::LauncherAddApp) {
    return;
  }

  uint64_t currentState = settingsController.GetAppDisabled();

  //clearing all bits in currentState which do not belong to a regular app to be resistant against changes
  //all ones
  uint64_t mask = -1;
  //clear bits associated with available apps, except addingApps app
  mask = mask << (applications.size() - 1);
  // invert, only bits associated with available apps are set
  mask = ~mask;
  //apply mask
  currentState &= mask;

  currentState ^= (1L << id);

  settingsController.SetAppDisabled(currentState);

  CalculatePages();
  UpdateButtonMap();
  EnableButtons();
}

void ApplicationList::OnValueChangedEvent(lv_obj_t* obj, uint32_t buttonId) {
  if (obj != btnm1)
    return;
  // if overlay is shown, do not react to touch but hide overlay
  if (overlay.get() != nullptr) {
    hideOverlay();
    return;
  }

  uint8_t appId = GetAppIdOnButton(buttonId);
  if (longPressed && !addingApps) {
    // Adding apps is a special app which can not be hidden
    if (applications[appId].application == Apps::LauncherAddApp) {
      longPressed = false;
      return;
    }
    showOverlay(appId);
  } else {
    if (addingApps) {
      ToggleApp(appId);
    } else {
      Screen::app->StartApp(applications[appId].application, DisplayApp::FullRefreshDirections::Up);
      running = false;
    }
  }
  longPressed = false;
}

void ApplicationList::CalculatePages() {
  uint8_t enabledApps = 0;
  for (uint8_t i = 0; i < applications.size(); i++) {
    if (IsShown(i)) {
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

ApplicationList::Overlay::Overlay(const char* icon, uint8_t appId, ApplicationList* parent) : appId(appId), parent(parent) {
  btnOverlay = lv_btn_create(lv_scr_act(), nullptr);
  btnOverlay->user_data = this;
  lv_obj_set_event_cb(btnOverlay, btn_handler);
  lv_obj_set_height(btnOverlay, 180);
  lv_obj_set_width(btnOverlay, 200);
  lv_obj_align(btnOverlay, lv_scr_act(), LV_ALIGN_CENTER, 0, 10);
  lv_obj_t* txtMessage = lv_label_create(btnOverlay, nullptr);
  lv_label_set_text_fmt(txtMessage, "Hide %s?", icon);
  lv_obj_align(txtMessage, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 4);

  btnYes = lv_btn_create(btnOverlay, nullptr);
  btnYes->user_data = this;
  lv_obj_t* btnNo = lv_btn_create(btnOverlay, nullptr);
  btnNo->user_data = this;
  lv_obj_set_event_cb(btnYes, btn_handler);
  lv_obj_set_event_cb(btnNo, btn_handler);
  lv_obj_set_style_local_bg_color(btnYes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
  lv_obj_set_style_local_bg_color(btnNo, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);

  lv_obj_t* txtYes = lv_label_create(btnYes, nullptr);
  lv_label_set_text_static(txtYes, "Yes");
  lv_obj_t* txtNo = lv_label_create(btnNo, nullptr);
  lv_label_set_text_static(txtNo, "No");

  lv_obj_align(btnYes, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_align(btnNo, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
}

void ApplicationList::showOverlay(uint8_t appId) {
  overlay = std::make_unique<ApplicationList::Overlay>(applications[appId].icon, appId, this);
}

void ApplicationList::hideOverlay() {
  overlay.reset(nullptr);
}

ApplicationList::Overlay::~Overlay() {
  NRF_LOG_INFO("No Overlay");
  lv_obj_del(btnOverlay);
  btnOverlay = nullptr;
  btnYes = nullptr;
}

void ApplicationList::Overlay::HandleButtons(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }
  if (obj == btnYes) {
    parent->ToggleApp(appId);
  }
  parent->hideOverlay();
}

