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
      /**
       * This class is used to select an app to be started
       *
       * It features a context action (long press) to hide
       * apps.
       *
       * To reshow apps, it has a second mode (selected by
       * addingApps ctor parameter), which only shows hidden
       * apps and reshows them on click.
       *
       * This second mode is dynamically shown as additional app
       * if any other app is hidden
       */
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

        static constexpr std::array applications {Applications {Symbols::stopWatch, Apps::StopWatch},
                                                  Applications {Symbols::music, Apps::Music},
                                                  Applications {Symbols::map, Apps::Navigation},
                                                  Applications {Symbols::shoe, Apps::Steps},
                                                  Applications {Symbols::heartBeat, Apps::HeartRate},
                                                  Applications {Symbols::hourGlass, Apps::Timer},
                                                  Applications {Symbols::paintbrush, Apps::Paint},
                                                  Applications {Symbols::paddle, Apps::Paddle},
                                                  Applications {"2", Apps::Twos},
                                                  Applications {Symbols::chartLine, Apps::Motion},
                                                  Applications {Symbols::drum, Apps::Metronome},
                                                  Applications {Symbols::clock, Apps::Alarm},
                                                  Applications {"+", Apps::LauncherAddApp}};

        /**
         * Updates btnmMap according to current page and enabled apps
         * This has immediate effect on the buttons, no other action needed
         */
        void UpdateButtonMap();

        /**
         * Sets buttons to either enabled or disabled, depending on the label
         * calculated by UpdateButtonMap()
         */
        void EnableButtons();

        /**
         * Returns wether an app is to be shown
         * is sensitiv to addingApps flag
         */
        bool IsShown(uint8_t id) const;

        /**
         * Calculates which app is placed on which button
         * Follows calculation in UpdateButtonMap() and as such
         * is resistant to duplicate icons
         */
        uint8_t GetAppIdOnButton(uint8_t buttonNr);

        /**
         * toogles the hide flag of an app
         */
        void ToggleApp(uint8_t id);

        /**
         * returns the index of the first app to be shown on certain page
         */
        uint8_t GetStartAppIndex(uint8_t page);

        /**
         * calculates and sets pages to the number of pages needed to show all enabled apps
         */
        void CalculatePages();

        void showOverlay(uint8_t appId);
        void hideOverlay();
      };
    }
  }
}
