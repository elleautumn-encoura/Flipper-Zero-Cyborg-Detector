#include "cyborg_detector_app.h"
#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <furi_hal_nfc.h>
#include <core/log.h>
#include <notification/notification_messages.h>


#define TAG "CyborgDetectorApp"
#define NFC_ACTIVE_HOLD_MS 500

static void cyborg_detector_set_nfc_active(CyborgDetectorApp* app, bool active) {
    if(app->nfc_active == active) return;

    app->nfc_active = active;

    if(active) {
        // Choose any notification sequence you prefer for “active”
        notification_message(app->notifications, &sequence_set_red_255);
        app->led_active = true;
    } else {
        notification_message(app->notifications, &sequence_reset_red);
        app->led_active = false;
    }
}




static void cyborg_detector_draw_callback(Canvas* canvas, void* context) {
    CyborgDetectorApp* app = context;
    (void)app;

    // Clear the canvas
    canvas_clear(canvas);

    // Draw the "CYBORG DETECTOR" text at the top, centered
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "CYBORG DETECTOR");

    // Draw a target in the center
    canvas_draw_circle(canvas, 64, 32, 10); // Outer circle
    canvas_draw_circle(canvas, 64, 32, 5); // Inner circle
    canvas_draw_line(canvas, 54, 32, 74, 32); // Horizontal line
    canvas_draw_line(canvas, 64, 22, 64, 42); // Vertical line

        // Feedback window when LED is being lit
    canvas_set_font(canvas, FontSecondary);

    if(app->field_active) {
        // Example window centered above bottom text area
        canvas_draw_frame(canvas, 18, 44, 92, 16);
        canvas_draw_str_aligned(canvas, 64, 46, AlignCenter, AlignTop, "CYBORG DETECTED!");
    } else {
        canvas_draw_str_aligned(canvas, 64, 54, AlignCenter, AlignBottom, "Scanning...");
    }
}
 
    const uint32_t now = furi_get_tick();
    const uint32_t hold_ticks = furi_ms_to_ticks(NFC_ACTIVE_HOLD_MS);
    const bool show_active = app->nfc_active || ((now - app->last_activity_tick) < hold_ticks);

   canvas_set_font(canvas, FontSecondary);

if(app->led_active) {
    canvas_draw_frame(canvas, 18, 44, 92, 16);
    canvas_draw_str_aligned(canvas, 64, 46, AlignCenter, AlignTop, "CYBORG DETECTED!");
} else {
    canvas_draw_str_aligned(canvas, 64, 54, AlignCenter, AlignBottom, "Scanning...");
}


static void cyborg_detector_input_callback(InputEvent* input_event, void* context) {
    furi_assert(context);
    CyborgDetectorApp* app = context;
    furi_message_queue_put(app->event_queue, input_event, FuriWaitForever);
}

static bool cyborg_detector_activate_nfc_field(CyborgDetectorApp* app) {
    FURI_LOG_I(TAG, "Activating NFC field");

    furi_hal_nfc_acquire();
    furi_hal_nfc_low_power_mode_stop();
    FuriHalNfcError error = furi_hal_nfc_poller_field_on();

    if(error != FuriHalNfcErrorNone) {
        FURI_LOG_E(TAG, "Failed to turn on NFC field: %d", error);
        furi_hal_nfc_release();
        return false;
    }

    app->field_active = true;
    FURI_LOG_I(TAG, "NFC field activated");
    return true;
}

static void cyborg_detector_deactivate_nfc_field(CyborgDetectorApp* app) {
    FURI_LOG_I(TAG, "Deactivating NFC field");
    furi_hal_nfc_low_power_mode_start();
    furi_hal_nfc_release();
    app->field_active = false;
    FURI_LOG_I(TAG, "NFC field deactivated");
}

CyborgDetectorApp* cyborg_detector_app_alloc() {
    CyborgDetectorApp* app = malloc(sizeof(CyborgDetectorApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->running = true;
    app->field_active = false;
    app->led_active = false;
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->nfc_active = false;
    app->last_activity_tick = 0;
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->led_active = false;



    view_port_draw_callback_set(app->view_port, cyborg_detector_draw_callback, app);
    view_port_input_callback_set(app->view_port, cyborg_detector_input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    return app;
}

void cyborg_detector_app_free(CyborgDetectorApp* app) {
    furi_assert(app);

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);


    free(app);
}

int32_t cyborg_detector_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Starting Cyborg Detector app");

    CyborgDetectorApp* app = cyborg_detector_app_alloc();

    if(!cyborg_detector_activate_nfc_field(app)) {
        FURI_LOG_E(TAG, "Failed to activate NFC field, exiting");
        cyborg_detector_app_free(app);
        return 255;
    }

    InputEvent event;
    while(app->running) {
       if(furi_message_queue_get(app->event_queue, &event, 100) == FuriStatusOk) {
    if(event.type == InputTypeShort && event.key == InputKeyBack) {
        app->running = false;
    }
        
        else if(event.type == InputTypeLong && event.key == InputKeyBack) {
            app->led_active = !app->led_active;

        if(app->led_active) {
        notification_message(app->notifications, &sequence_set_blue_255);
    } else {
        notification_message(app->notifications, &sequence_reset_blue);
        }
    }
}

    } else if(event.type == InputTypeShort && event.key == InputKeyOk) {
        // Toggle LED + banner state (test harness)
        app->led_active = !app->led_active;

        if(app->led_active) {
            notification_message(app->notifications, &sequence_set_blue_255);
        } else {
            notification_message(app->notifications, &sequence_reset_blue);
        }
    }
}

        }

    bool tag_present = false; // TODO: implement real check/poller callback
        if(tag_present) {
        app->last_activity_tick = furi_get_tick();
        cyborg_detector_set_nfc_active(app, true);
    }     else {
        cyborg_detector_set_nfc_active(app, false);
    }

    view_port_update(app->view_port);
}

        view_port_update(app->view_port);
    }

    cyborg_detector_deactivate_nfc_field(app);
    cyborg_detector_app_free(app);
    FURI_LOG_I(TAG, "Cyborg Detector app finished");
    return 0;
}
