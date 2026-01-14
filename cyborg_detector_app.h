#pragma once

#include <gui/gui.h>
#include <gui/view_port.h>
#include <furi_hal_nfc.h>
#include <notification/notification_app.h>

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    
    bool running;
    bool field_active;
    bool led_active;
} CyborgDetectorApp;

CyborgDetectorApp* cyborg_detector_app_alloc();
void cyborg_detector_app_free(CyborgDetectorApp* app);
int32_t cyborg_detector_app(void* p);
