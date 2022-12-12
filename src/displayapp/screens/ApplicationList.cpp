#include "displayapp/screens/ApplicationList.h"
#include <lvgl/lvgl.h>
#include "displayapp/DisplayApp.h"
#include <nrf_log.h>

using namespace Pinetime::Applications::Screens;

// constexpr std::array<ApplicationList::Applications, ApplicationList::applications.size()> ApplicationList::applications;

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

  for (auto application : applicationsStatic) {
    applications.push_back({application.icon, application.application, LV_COLOR_BLUE, false});
  }

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

  // create the buttons
  uint8_t width = (LV_HOR_RES - 8 - 20) / 3;
  uint8_t height = (LV_VER_RES - 30 - 20) / 2;

  for (uint8_t column = 0; column < 3; column++){
    for (uint8_t row = 0; row < 2; row++) {
      appButtons[row * 3 + column] = lv_btn_create(lv_scr_act(), nullptr);
      appButtons[row * 3 + column]->user_data = this;
      lv_obj_set_event_cb(appButtons[row * 3 + column], event_handler);
      lv_obj_set_size(appButtons[row * 3 + column], width, height);
      lv_obj_align(appButtons[row * 3 + column], NULL, LV_ALIGN_IN_BOTTOM_LEFT, column * (width + 10), -(10 + (1 - row) * (height + 10)));
      appLabels[row * 3 + column] = lv_label_create(appButtons[row * 3 + column], nullptr);
    }
  }
  
  UpdateButtons();

  taskUpdate = lv_task_create(lv_update_task, 5000, LV_TASK_PRIO_MID, this);

  UpdateScreen();
}

ApplicationList::~ApplicationList() {
  lv_task_del(taskUpdate);
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

const std::list<ApplicationList::Applications>& ApplicationList::getList() {
  if (addingApps) {
    return hiddenApplications;
  } else {
    return applications;
  }
}

void ApplicationList::UpdateButtons() {
  auto appList = getList();
  auto app = std::next(appList.begin(), 6 * page);

  for (uint8_t buttonIndex = 0; buttonIndex < 6; buttonIndex++) {
    // we ran out of apps, fill with empty buttons
    if (app == appList.end()) {
      lv_label_set_text_static(appLabels[buttonIndex], " ");
      lv_obj_set_hidden(appButtons[buttonIndex], true);
      continue;
    }
    lv_label_set_text_static(appLabels[buttonIndex], app->icon);
    lv_obj_set_style_local_bg_color(appButtons[buttonIndex], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, app->color);
    lv_obj_set_hidden(appButtons[buttonIndex], false);
    app++;
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
      UpdateButtons();
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
      UpdateButtons();
      pageIndicator.Update(page, pages);
      return true;
  }
  return false;
}

bool ApplicationList::IsShown(uint8_t id) const {
  uint64_t currentState = settingsController.GetAppDisabled();

  if (std::next(applications.begin(), id)->application == Apps::LauncherAddApp) {
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
  return (std::next(applications.begin(), id)->application != Apps::None) && (!disabled);
}

uint8_t ApplicationList::GetAppIdOnButton(uint8_t buttonNr) {
  return page * 6 + buttonNr;
}

void ApplicationList::OnLongHold() {
  longPressed = true;
  motorController.RunForDuration(50);
}

void ApplicationList::hideApp(uint8_t id) {
  if (std::next(applications.begin(), id)->application == Apps::LauncherAddApp) {
    return;
  }
  auto element = std::next(applications.begin(), id);
  hiddenApplications.splice(hiddenApplications.end(), applications, element);

  //TODO settings

  CalculatePages();
  UpdateButtons();
}

void ApplicationList::showApp(uint8_t id) {
  if (std::next(applications.begin(), id)->application == Apps::LauncherAddApp) {
    return;
  }
  auto element = std::next(hiddenApplications.begin(), id);
  applications.splice(applications.end(), hiddenApplications, element);

  //TODO settings

  CalculatePages();
  UpdateButtons();
}

void ApplicationList::OnValueChangedEvent(lv_obj_t* obj, lv_event_t event) {
  // if overlay is shown, do not react to touch but hide overlay
  if (overlay.get() != nullptr) {
    hideOverlay();
    return;
  }

  uint8_t buttonId = 0;

  for (buttonId = 0; buttonId < 6; buttonId++) {
    if (obj == appButtons[buttonId]) {
      HandleButton(buttonId);
    }
  }
}

void ApplicationList::HandleButton(uint8_t buttonId){
  uint8_t appId = buttonId + 6 * page;
  if (longPressed && !addingApps) {
    // Adding apps is a special app which has no context menu
    if (std::next(applications.begin(), appId)->application == Apps::LauncherAddApp) {
      longPressed = false;
      return;
    }
    showOverlay(appId);
  } else {
    if (addingApps) {
      showApp(appId);
    } else {
      Screen::app->StartApp(std::next(applications.begin(), appId)->application, DisplayApp::FullRefreshDirections::Up);
      running = false;
    }
  }
  longPressed = false;
}


void ApplicationList::CalculatePages() {
  uint8_t enabledApps = getList().size();
  if (enabledApps == 0) {
    page = 0;
    pages = 1;
    pageIndicator.Update(page, 1);
    return;
  }
  pages = (enabledApps + 5) / 6;
  if (page >= pages) {
    page = pages - 1;
    app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
  }
  pageIndicator.Update(page, pages);
}

uint8_t ApplicationList::moveForward(uint8_t appId) {
  auto element = std::next(applications.begin(), appId);
  auto position = std::next(element, 2);
  if (position == applications.end()) {
    // second to last element
    // last element is addApp
    // that means this is already the last "normal" app
    // and we do not move
    return appId;
  }
  applications.splice(position, applications, element);
  CalculatePages();
  UpdateButtons();
  return ++appId;
}

lv_color_t ApplicationList::rotateColor(uint8_t appId, lv_color_t currentColor) {
  for (auto iter = colors.begin(); iter != colors.end(); iter++) {
    if (iter->full == currentColor.full) {
      iter++;
      if (iter == colors.end()) {
        return *colors.begin();
      } else {
        return *iter;
      }
    }
  }
  return *colors.begin();
}

void ApplicationList::showOverlay(uint8_t appId) {
  auto app = std::next(applications.begin(), appId);
  overlay = std::make_unique<ApplicationList::Overlay>(app->icon, appId, app->color, this);
}

void ApplicationList::hideOverlay() {
  overlay.reset(nullptr);
}

ApplicationList::Overlay::Overlay(const char* icon, uint8_t appId, lv_color_t color, ApplicationList* parent)
  : appId(appId), parent(parent) {
  lvOverlay = lv_obj_create(lv_scr_act(), nullptr);
  lvOverlay->user_data = this;

  lv_obj_set_event_cb(lvOverlay, btn_handler);
  lv_obj_set_height(lvOverlay, LV_VER_RES - 16);
  lv_obj_set_width(lvOverlay, LV_HOR_RES - 16);
  lv_obj_set_style_local_bg_color(lvOverlay, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  lv_obj_set_style_local_bg_opa(lvOverlay, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_90);
  lv_obj_align(lvOverlay, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 16);
  lv_obj_t* txtMessage = lv_label_create(lvOverlay, nullptr);
  lv_label_set_text_fmt(txtMessage, "%s Menu", icon);
  lv_obj_align(txtMessage, nullptr, LV_ALIGN_IN_TOP_MID, 0, 8);

  // Color
  btnColor = lv_btn_create(lvOverlay, nullptr);
  btnColor->user_data = this;
  lv_obj_set_event_cb(btnColor, btn_handler);
  lv_obj_set_style_local_bg_color(btnColor, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
  lv_obj_set_height(btnColor, 50);
  lv_obj_set_width(btnColor, 50);
  lv_obj_set_style_local_bg_opa(btnColor, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_90);
  lv_obj_align(btnColor, nullptr, LV_ALIGN_IN_BOTTOM_MID, -45, -135);

  // Contrast
  btnContrast = lv_btn_create(lvOverlay, nullptr);
  btnContrast->user_data = this;
  lv_obj_set_event_cb(btnContrast, btn_handler);
  lv_obj_set_style_local_bg_color(btnContrast, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
  lv_obj_set_height(btnContrast, 50);
  lv_obj_set_width(btnContrast, 50);
  lv_obj_set_style_local_bg_opa(btnContrast, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_90);
  lv_obj_align(btnContrast, nullptr, LV_ALIGN_IN_BOTTOM_MID, 45, -135);
  txtContrast = lv_label_create(btnContrast, nullptr);
  lv_label_set_text_static(txtContrast, icon);
  lv_obj_set_style_local_text_color(txtContrast, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);

  // Move
  snprintf(positionText, sizeof(positionText), "%i", appId + 1);
  moveMap[0] = "<";
  moveMap[1] = positionText;
  moveMap[2] = ">";
  moveMap[3] = "";
  btnmMove = lv_btnmatrix_create(lvOverlay, nullptr);
  btnmMove->user_data = this;
  lv_obj_set_event_cb(btnmMove, btn_handler);
  lv_btnmatrix_set_map(btnmMove, moveMap);
  lv_obj_set_height(btnmMove, 50);
  lv_obj_set_width(btnmMove, LV_HOR_RES - 32);
  lv_obj_align(btnmMove, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -70);
  lv_obj_set_style_local_bg_opa(btnmMove, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, LV_OPA_0);
  lv_obj_set_style_local_pad_all(btnmMove, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_inner(btnmMove, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 10);

  // Hide
  btnHide = lv_btn_create(lvOverlay, nullptr);
  btnHide->user_data = this;
  lv_obj_set_event_cb(btnHide, btn_handler);
  lv_obj_set_height(btnHide, 40);
  lv_obj_set_width(btnHide, LV_HOR_RES - 32);

  lv_obj_t* txtHide = lv_label_create(btnHide, nullptr);
  lv_label_set_text_static(txtHide, "Hide");
  lv_obj_align(btnHide, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

  NRF_LOG_INFO("1");
}

ApplicationList::Overlay::~Overlay() {
  lv_obj_del(lvOverlay);
}

void ApplicationList::Overlay::HandleButtons(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }

  if (obj == btnColor) {
    lv_color_t newColor = parent->rotateColor(appId, lv_obj_get_style_bg_color(btnColor, LV_LABEL_PART_MAIN));
    lv_obj_set_style_local_bg_color(btnColor, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, newColor);
    return;
  }

  if (obj == btnContrast) {
    lv_color_t color = lv_obj_get_style_bg_color(btnContrast, LV_LABEL_PART_MAIN);
    lv_obj_set_style_local_bg_color(btnContrast, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_obj_get_style_text_color(txtContrast, LV_LABEL_PART_MAIN));
    lv_obj_set_style_local_text_color(txtContrast, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
    //parent->toggleContrast(appId);
    return;
  }

  if (obj == btnmMove) {
    appId = parent->moveForward(appId);
    snprintf(positionText, sizeof(positionText), "%i", appId + 1);
    lv_btnmatrix_set_map(btnmMove, moveMap);
    return;
  }

  if (obj == btnHide) {
    parent->hideApp(appId);
  }
  parent->hideOverlay();
}
