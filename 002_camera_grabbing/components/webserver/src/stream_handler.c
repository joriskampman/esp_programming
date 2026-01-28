#include "stream_handler.h"
#include "esp_log.h"
#include "common.h"
#include "display.h"

static const char* STRMTAG = "stream_handler";

void stream_mjpeg_handler_task(httpd_req_t* req) {
  const char* boundary = "--frame";

  httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

  camera_fb_t* fb_disp = get_display_fb();
  while (1) {
    // check if client still connected
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
      ESP_LOGE(STRMTAG, "The client is disconnected");
      break; // client disconnected
    }

    // send boundary and headers
    httpd_resp_send_chunk(req, boundary, strlen(boundary));
    httpd_resp_send_chunk(req, "\r\n", 2);
    httpd_resp_send_chunk(req, "Content-Type: image/raw\r\n", 25);
    httpd_resp_send_chunk(req, "Content-Length: ", 16);

    // cocnvert frame size to string
    char len_str[16];
    snprintf(len_str, sizeof(len_str), "%d\r\n\r\n", fb_disp->len);

    // send frame size
    httpd_resp_send_chunk(req, len_str, strlen(len_str));

    // send frame data
    httpd_resp_send_chunk(req, (char*)fb_disp->buf, fb_disp->len);
    httpd_resp_send_chunk(req, "\r\n", 2);

    // wait for the next frame
    vTaskDelay(pdMS_TO_TICKS(b2m_f*1.0f/get_display_rate_hz_f()));
  }
}  // stream_mjpeg_handler