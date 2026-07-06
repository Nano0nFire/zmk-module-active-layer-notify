#include <nano0nfire/active_layer_notify/active_layer_notify.pb.h>
#include <pb_encode.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/studio/custom.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_ACTIVE_LAYER_NOTIFY_STUDIO_RPC)

static bool encode_notification(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    nano0nfire_active_layer_notify_Notification *notification =
        (nano0nfire_active_layer_notify_Notification *)*arg;
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    size_t size;
    if (!pb_get_encoded_size(&size, nano0nfire_active_layer_notify_Notification_fields,
                             notification)) {
        return false;
    }

    if (!pb_encode_varint(stream, size)) {
        return false;
    }

    return pb_encode(stream, nano0nfire_active_layer_notify_Notification_fields, notification);
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

    // Not found: return a sentinel so callers can skip emitting rather than
    // misdirecting the notification to subsystem index 0.
    return UINT8_MAX;
}

static int active_layer_notify_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    nano0nfire_active_layer_notify_Notification notification =
        nano0nfire_active_layer_notify_Notification_init_zero;
    notification.which_notification_type =
        nano0nfire_active_layer_notify_Notification_active_layer_changed_tag;

    nano0nfire_active_layer_notify_ActiveLayerChangedNotification *payload =
        &notification.notification_type.active_layer_changed;

    zmk_keymap_layer_index_t active_index = zmk_keymap_highest_layer_active();
    zmk_keymap_layer_id_t active_id = zmk_keymap_layer_index_to_id(active_index);
    const char *active_name = zmk_keymap_layer_name(active_id);

    // nanopb requires has_<field> for optional sub-messages, otherwise the
    // entire `layer` payload is silently omitted from the encoded notification.
    payload->has_layer = true;
    payload->layer.index = active_index;
    payload->layer.id = active_id;
    if (active_name != NULL) {
        strncpy(payload->layer.name, active_name, sizeof(payload->layer.name) - 1);
        payload->layer.name[sizeof(payload->layer.name) - 1] = '\0';
    }
    payload->changed_layer_id = ev->layer;
    payload->state = ev->state;

    uint8_t subsystem_index = find_subsystem_index("nano0nfire__active_layer_notify");
    if (subsystem_index == UINT8_MAX) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    pb_callback_t encode_cb = {.funcs.encode = encode_notification, .arg = &notification};
    raise_zmk_studio_custom_notification((struct zmk_studio_custom_notification){
        .subsystem_index = subsystem_index,
        .encode_payload = encode_cb,
    });

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(active_layer_notify_listener, active_layer_notify_listener);
ZMK_SUBSCRIPTION(active_layer_notify_listener, zmk_layer_state_changed);

#endif
