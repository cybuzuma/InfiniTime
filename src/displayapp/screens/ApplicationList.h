#pragma once

#include <array>
#include <memory>
#include <array>

#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "components/battery/BatteryController.h"
#include "displayapp/screens/Symbols.h"


namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class ApplicationList : public Screen {
      public:
        explicit ApplicationList(DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 Pinetime::Controllers::Battery& batteryController,
                                 Pinetime::Controllers::Ble& bleController,
                                 Controllers::DateTime& dateTimeController);
        ~ApplicationList() override;

        bool OnTouchEvent(TouchEvents event) override;

        void UpdateScreen();
        void OnValueChangedEvent(lv_obj_t* obj, uint32_t buttonId);
        void OnLongPressed(lv_obj_t* obj, uint32_t buttonId);

      private:
        struct Applications {
          const char* icon;
          bool enabled;
          Pinetime::Applications::Apps application;
        };

        Controllers::Settings& settingsController;
        Pinetime::Controllers::Battery& batteryController;
        Pinetime::Controllers::Ble& bleController;
        Controllers::DateTime& dateTimeController;

        lv_task_t* taskUpdate;

        lv_obj_t* label_time;
        lv_point_t pageIndicatorBasePoints[2];
        lv_point_t pageIndicatorPoints[2];
        lv_obj_t* pageIndicatorBase;
        lv_obj_t* pageIndicator;
        lv_obj_t* btnm1;

        BatteryIcon batteryIcon;

        const char* btnmMap[8];

        std::array<Applications, 12> applications {{
          {Symbols::stopWatch, true, Apps::StopWatch},
          {Symbols::music, true, Apps::None},
          {Symbols::map, true, Apps::Navigation},
          {Symbols::shoe, true, Apps::Steps},
          {Symbols::heartBeat, true, Apps::HeartRate},
          {Symbols::hourGlass, true, Apps::Timer},
          {Symbols::paintbrush, true, Apps::Paint},
          {Symbols::paddle, true, Apps::Paddle},
          {"2", true, Apps::Twos},
          {Symbols::chartLine, true, Apps::Motion},
          {Symbols::drum, true, Apps::Metronome},
          {Symbols::clock, true, Apps::Alarm}}
        };

        void updateButtonMap();
        void enableButtons();

        bool isShown(const Applications &app) const;

        Applications * getAppOnButton(uint8_t buttonNr);

        uint8_t getStartAppIndex(uint8_t page);
      };
    }
  }
}
