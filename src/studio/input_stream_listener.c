#include <zmk/input_stream/input_stream.pb.h>
#include <pb_encode.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/studio/custom.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_INPUT_STREAM_STUDIO_RPC)

// Owned by input_stream_handler.c (toggled over the Studio RPC).
extern bool zmk_input_stream_is_enabled(void);

static bool encode_notification(pb_ostream_t *stream, const pb_field_t *field,
                                void *const *arg) {
    ARG_UNUSED(field);
    const zmk_input_stream_Notification *notification =
        (const zmk_input_stream_Notification *)(*arg);

    size_t size = 0;
    if (!pb_get_encoded_size(&size, zmk_input_stream_Notification_fields, notification)) {
        return false;
    }

    return pb_encode(stream, zmk_input_stream_Notification_fields, notification);
}

static uint8_t find_subsystem_index(const char *identifier) {
    extern struct zmk_rpc_custom_subsystem _zmk_rpc_custom_subsystem_list_start[];
    extern struct zmk_rpc_custom_subsystem _zmk_rpc_custom_subsystem_list_end[];

    uint8_t index = 0;
    for (struct zmk_rpc_custom_subsystem *subsys = _zmk_rpc_custom_subsystem_list_start;
         subsys < _zmk_rpc_custom_subsystem_list_end; subsys++) {
        if (strcmp(subsys->identifier, identifier) == 0) {
            return index;
        }
        index++;
    }
    return UINT8_MAX;
}

static void send_notification(const zmk_input_stream_Notification *notification) {
    uint8_t subsystem_index = find_subsystem_index("zmk__input_stream");
    if (subsystem_index == UINT8_MAX) {
        return;
    }

    pb_callback_t encode_cb = {.funcs.encode = encode_notification,
                               .arg = (void *)notification};
    raise_zmk_studio_custom_notification((struct zmk_studio_custom_notification){
        .subsystem_index = subsystem_index,
        .encode_payload = encode_cb,
    });
}

static int input_stream_event_listener(const zmk_event_t *eh) {
    if (!zmk_input_stream_is_enabled()) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_position_state_changed *pos_ev = as_zmk_position_state_changed(eh);
    if (pos_ev != NULL) {
        zmk_input_stream_Notification notification = zmk_input_stream_Notification_init_zero;
        notification.which_notification_type = zmk_input_stream_Notification_key_event_tag;

        zmk_input_stream_KeyEventNotification *payload =
            &notification.notification_type.key_event;
        payload->position = pos_ev->position;
        payload->pressed = pos_ev->state;

        // Resolve what the key does on the currently active layer, so the
        // client can label the event without re-fetching the keymap.
        zmk_keymap_layer_id_t active_id =
            zmk_keymap_layer_index_to_id(zmk_keymap_highest_layer_active());
        const struct zmk_behavior_binding *binding =
            zmk_keymap_get_layer_binding_at_idx(active_id, pos_ev->position);
        if (binding != NULL) {
            payload->behavior_id = zmk_behavior_get_local_id(binding->behavior_dev);
            payload->param1 = binding->param1;
            payload->param2 = binding->param2;
        }

        send_notification(&notification);
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_layer_state_changed *layer_ev = as_zmk_layer_state_changed(eh);
    if (layer_ev != NULL) {
        zmk_input_stream_Notification notification = zmk_input_stream_Notification_init_zero;
        notification.which_notification_type = zmk_input_stream_Notification_layer_change_tag;
        notification.notification_type.layer_change.layer_index =
            zmk_keymap_highest_layer_active();

        send_notification(&notification);
        return ZMK_EV_EVENT_BUBBLE;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(input_stream_listener, input_stream_event_listener);
ZMK_SUBSCRIPTION(input_stream_listener, zmk_position_state_changed);
ZMK_SUBSCRIPTION(input_stream_listener, zmk_layer_state_changed);

#endif // IS_ENABLED(CONFIG_ZMK_INPUT_STREAM_STUDIO_RPC)
