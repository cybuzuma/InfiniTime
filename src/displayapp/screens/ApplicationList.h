#pragma once

#include <array>
#include <memory>
#include <array>

#include "displayapp/screens/Screen.h"
#include "displayapp/widgets/PageIndicator.h"
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "components/battery/BatteryController.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/Symbols.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class ApplicationList : public Screen {
      public:
        class Overlay {
        public:
          Overlay(const char* icon, uint8_t appId, ApplicationList* parent);
          ~Overlay();
          void HandleButtons(lv_obj_t* obj, lv_event_t event);

        private:
          uint8_t appId;
          ApplicationList* parent;
          lv_obj_t* btnOverlay = nullptr;
          lv_obj_t* btnYes = nullptr;
        };

        explicit ApplicationList(DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 Pinetime::Controllers::Battery& batteryController,
                                 Pinetime::Controllers::Ble& bleController,
                                 Controllers::DateTime& dateTimeController,
                                 Controllers::MotorController& motorController,
                                 bool addingApps = false);
        ~ApplicationList() override;

        bool OnTouchEvent(TouchEvents event) override;

        void UpdateScreen();
        void OnValueChangedEvent(lv_obj_t* obj, uint32_t buttonId);
        void OnLongHold();

      private:
        struct Applications {
          const char* icon;
          Pinetime::Applications::Apps application;
        };

        bool longPressed {false};

        bool addingApps;

        uint8_t page;
        uint8_t pages;

        Controllers::Settings& settingsController;
        Pinetime::Controllers::Battery& batteryController;
        Pinetime::Controllers::Ble& bleController;
        Controllers::DateTime& dateTimeController;
        Controllers::MotorController& motorController;

        lv_task_t* taskUpdate;

        lv_obj_t* label_time;
        lv_obj_t* btnm1;

        BatteryIcon batteryIcon;
        Widgets::PageIndicator pageIndicator;
        std::unique_ptr<Overlay> overlay = {nullptr};

        const char* btnmMap[8];

        // TODO can we leave out the 13?
        std::array<Applications, 13> applications {{{Symbols::stopWatch, Apps::StopWatch},
                                                    {Symbols::music, Apps::Music},
                                                    {Symbols::map, Apps::Navigation},
                                                    {Symbols::shoe, Apps::Steps},
                                                    {Symbols::heartBeat, Apps::HeartRate},
                                                    {Symbols::hourGlass, Apps::Timer},
                                                    {Symbols::paintbrush, Apps::Paint},
                                                    {Symbols::paddle, Apps::Paddle},
                                                    {"2", Apps::Twos},
                                                    {Symbols::chartLine, Apps::Motion},
                                                    {Symbols::drum, Apps::Metronome},
                                                    {Symbols::clock, Apps::Alarm},
                                                    {"+", Apps::LauncherAddApp}}};

        void UpdateButtonMap();
        void EnableButtons();

        bool IsShown(uint8_t id) const;

        uint8_t GetAppIdOnButton(uint8_t buttonNr);

        void ToggleApp(uint8_t id);

        uint8_t GetStartAppIndex(uint8_t page);

        void CalculatePages();
      };
    }
  }
}
