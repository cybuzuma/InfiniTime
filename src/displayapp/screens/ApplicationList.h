#pragma once

#include <memory>
#include <list>

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
          Overlay(const char* icon, uint8_t appId, lv_color_t color, ApplicationList* parent);
          ~Overlay();
          void HandleButtons(lv_obj_t* obj, lv_event_t event);

        private:
          uint8_t appId;
          ApplicationList* parent;
          char positionText[3];
          lv_obj_t* lvOverlay = nullptr;
          lv_obj_t* btnColor = nullptr;
          lv_obj_t* btnContrast = nullptr;
          lv_obj_t* txtContrast = nullptr;
          lv_obj_t* btnmMove = nullptr;
          lv_obj_t* btnHide = nullptr;

          const char* moveMap[4];
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
        struct ApplicationsStatic {
          const char* icon;
          Pinetime::Applications::Apps application;
        };

        struct Applications: public ApplicationsStatic {
          lv_color_t color;
          bool colorinverted;
          const uint8_t originalIndex;
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
        lv_obj_t* appButtons[6];
        lv_obj_t* appLabels[6];

        BatteryIcon batteryIcon;
        Widgets::PageIndicator pageIndicator;
        std::unique_ptr<Overlay> overlay = {nullptr};

        const char* btnmMap[8];

        std::list<Applications> applications;
        std::list<Applications> hiddenApplications;

        static constexpr std::array<lv_color_t, 6> colors {
          { LV_COLOR_RED, LV_COLOR_GREEN, LV_COLOR_BLUE, LV_COLOR_GRAY, LV_COLOR_CYAN, LV_COLOR_ORANGE }
        };

        static constexpr std::array<ApplicationsStatic, 13> applicationsStatic {{{Symbols::stopWatch, Apps::StopWatch},
                                                                           {Symbols::clock, Apps::Alarm},
                                                                           {Symbols::hourGlass, Apps::Timer},
                                                                           {Symbols::shoe, Apps::Steps},
                                                                           {Symbols::heartBeat, Apps::HeartRate},
                                                                           {Symbols::music, Apps::Music},
                                                                           {Symbols::paintbrush, Apps::Paint},
                                                                           {Symbols::paddle, Apps::Paddle},
                                                                           {"2", Apps::Twos},
                                                                           {Symbols::chartLine, Apps::Motion},
                                                                           {Symbols::drum, Apps::Metronome},
                                                                           {Symbols::map, Apps::Navigation},
                                                                           {"+", Apps::LauncherAddApp}}};

        void UpdateButtons();

        const std::list<Applications>& getList();

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
         * hide an app
         */
        void hideApp(uint8_t id);

        /**
         * show an app
         */
        void showApp(uint8_t id);

        /**
         * calculates and sets pages to the number of pages needed to show all enabled apps
         */
        void CalculatePages();

        uint8_t moveForward(uint8_t appId);

        lv_color_t rotateColor(uint8_t appId, lv_color_t currentColor);

        void HandleButton(uint8_t buttonId);

        void showOverlay(uint8_t appId);
        void hideOverlay();
      };
    }
  }
}