#include <pb_decode.h>
#include <pb_encode.h>
#include <string.h>
#include <zmk/studio/custom.h>
#include <zmk/input_stream/input_stream.pb.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_INPUT_STREAM_STUDIO_RPC)

static struct zmk_rpc_custom_subsystem_meta input_stream_feature_meta = {
    ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS("https://github.com/Nano0nFire/zmk-module-active-layer-notify"),
    // Unsecured: the stream only mirrors which keys are pressed to the
    // already-connected editor, and it is off until explicitly enabled.
    .security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

ZMK_RPC_CUSTOM_SUBSYSTEM(zmk__input_stream, &input_stream_feature_meta,
                         input_stream_rpc_handle_request);

ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(zmk__input_stream, zmk_input_stream_Response);

// Whether key/layer events are currently streamed to the client; consumed by
// input_stream_listener.c. Off by default and after every reboot.
static bool stream_enabled;

bool zmk_input_stream_is_enabled(void) { return stream_enabled; }

static bool input_stream_rpc_handle_request(const zmk_custom_CallRequest *raw_request,
                                            pb_callback_t *encode_response) {
    zmk_input_stream_Response *resp =
        ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER_ALLOCATE(zmk__input_stream, encode_response);

    zmk_input_stream_Request req = zmk_input_stream_Request_init_zero;

    pb_istream_t req_stream =
        pb_istream_from_buffer(raw_request->payload.bytes, raw_request->payload.size);
    if (!pb_decode(&req_stream, zmk_input_stream_Request_fields, &req)) {
        LOG_WRN("Failed to decode input_stream request: %s", PB_GET_ERROR(&req_stream));
        zmk_input_stream_ErrorResponse err = zmk_input_stream_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Failed to decode request");
        resp->which_response_type = zmk_input_stream_Response_error_tag;
        resp->response_type.error = err;
        return true;
    }

    switch (req.which_request_type) {
    case zmk_input_stream_Request_enable_stream_tag:
        stream_enabled = true;
        break;
    case zmk_input_stream_Request_disable_stream_tag:
        stream_enabled = false;
        break;
    default: {
        LOG_WRN("Unsupported input_stream request type: %d", req.which_request_type);
        zmk_input_stream_ErrorResponse err = zmk_input_stream_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Unsupported request");
        resp->which_response_type = zmk_input_stream_Response_error_tag;
        resp->response_type.error = err;
        return true;
    }
    }

    resp->which_response_type = zmk_input_stream_Response_ok_tag;
    resp->response_type.ok = (zmk_input_stream_OkResponse)zmk_input_stream_OkResponse_init_zero;
    return true;
}

#endif // IS_ENABLED(CONFIG_ZMK_INPUT_STREAM_STUDIO_RPC)
