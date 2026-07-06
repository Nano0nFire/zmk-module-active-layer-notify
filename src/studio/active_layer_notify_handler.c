#include <pb_decode.h>
#include <pb_encode.h>
#include <string.h>
#include <zmk/keymap.h>
#include <zmk/studio/custom.h>
#include <nano0nfire/active_layer_notify/active_layer_notify.pb.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_rpc_custom_subsystem_meta active_layer_notify_feature_meta = {
    ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS("https://github.com/Nano0nFire/zmk-module-active-layer-notify"),
    // Unsecured is suggested by default to avoid unlocking in un-reliable
    // environments.
    .security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

ZMK_RPC_CUSTOM_SUBSYSTEM(nano0nfire__active_layer_notify, &active_layer_notify_feature_meta, active_layer_notify_rpc_handle_request);

ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(nano0nfire__active_layer_notify, nano0nfire_active_layer_notify_Response);

static int handle_get_active_layer(const nano0nfire_active_layer_notify_GetActiveLayerRequest *req,
                                   nano0nfire_active_layer_notify_Response *resp);

static bool active_layer_notify_rpc_handle_request(const zmk_custom_CallRequest *raw_request,
                                        pb_callback_t *encode_response) {
    nano0nfire_active_layer_notify_Response *resp =
        ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER_ALLOCATE(nano0nfire__active_layer_notify, encode_response);

    nano0nfire_active_layer_notify_Request req = nano0nfire_active_layer_notify_Request_init_zero;

    pb_istream_t req_stream =
        pb_istream_from_buffer(raw_request->payload.bytes, raw_request->payload.size);
    if (!pb_decode(&req_stream, nano0nfire_active_layer_notify_Request_fields, &req)) {
        LOG_WRN("Failed to decode active_layer_notify request: %s", PB_GET_ERROR(&req_stream));
        nano0nfire_active_layer_notify_ErrorResponse err = nano0nfire_active_layer_notify_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Failed to decode request");
        resp->which_response_type = nano0nfire_active_layer_notify_Response_error_tag;
        resp->response_type.error = err;
        return true;
    }

    int rc = 0;
    switch (req.which_request_type) {
    case nano0nfire_active_layer_notify_Request_get_active_layer_tag:
        rc = handle_get_active_layer(&req.request_type.get_active_layer, resp);
        break;
    default:
        LOG_WRN("Unsupported active_layer_notify request type: %d", req.which_request_type);
        rc = -1;
    }

    if (rc != 0) {
        nano0nfire_active_layer_notify_ErrorResponse err = nano0nfire_active_layer_notify_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Failed to process request");
        resp->which_response_type = nano0nfire_active_layer_notify_Response_error_tag;
        resp->response_type.error = err;
    }
    return true;
}

static int handle_get_active_layer(const nano0nfire_active_layer_notify_GetActiveLayerRequest *req,
                                   nano0nfire_active_layer_notify_Response *resp) {
    ARG_UNUSED(req);

    nano0nfire_active_layer_notify_GetActiveLayerResponse result =
        nano0nfire_active_layer_notify_GetActiveLayerResponse_init_zero;
    zmk_keymap_layer_index_t layer_index = zmk_keymap_highest_layer_active();
    zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer_index);
    const char *layer_name = zmk_keymap_layer_name(layer_id);

    // nanopb requires has_<field> for optional sub-messages, otherwise the
    // entire `layer` payload is silently omitted from the encoded response.
    result.has_layer = true;
    result.layer.index = layer_index;
    result.layer.id = layer_id;
    if (layer_name != NULL) {
        strncpy(result.layer.name, layer_name, sizeof(result.layer.name) - 1);
        result.layer.name[sizeof(result.layer.name) - 1] = '\0';
    }

    resp->which_response_type = nano0nfire_active_layer_notify_Response_get_active_layer_tag;
    resp->response_type.get_active_layer = result;
    return 0;
}
