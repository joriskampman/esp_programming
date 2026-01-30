#include "stream_handler.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "common.h"
#include "display.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "buffer_pool.h"
#include <stdint.h>
#include "webpage.h"
#ifndef NDEBUG
#define tje_log(msg) ESP_LOGI("TJE", "%s", msg)
#else
#define tje_log(msg)  // nothing in release mode
#endif

#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"


static const char* STRMTAG = "stream_handler";

// JPEG encoding context (reusable)
static TJEState* jpeg_state = NULL;
static uint8_t* jpeg_buffer = NULL;
static const size_t JPEG_BUFFER_SIZE = 80 * 1024; // 80KB for worst-case QVGA JPEG
static SemaphoreHandle_t jpeg_mutex = NULL;

static esp_err_t init_jpeg_encoder_safe(void) {
    if (!jpeg_mutex) {
        jpeg_mutex = xSemaphoreCreateMutex();
        if (!jpeg_mutex) {
            return ESP_FAIL;
        }
    }

    if (xSemaphoreTake(jpeg_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(STRMTAG, "JPEG init mutex timeout");
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_OK;
    if (jpeg_buffer == NULL) {
        // Allocate state structure
        jpeg_state = (TJEState*)malloc(sizeof(TJEState));
        if (!jpeg_state) {
            free(jpeg_buffer);
            jpeg_buffer = NULL;
            ESP_LOGE(STRMTAG, "Failed to allocate JPEG state");
            return ESP_FAIL;
        }

        // Initialize state (TinyJPEG uses zero-init)
        memset(jpeg_state, 0, sizeof(TJEState));

        ESP_LOGI(STRMTAG, "TinyJPEG initialized, buffer: %d KB", JPEG_BUFFER_SIZE / 1024);

    }
    xSemaphoreGive(jpeg_mutex);
    return ret;
}

// Initialize JPEG encoder once
static esp_err_t init_jpeg_encoder(void) {
    if (jpeg_buffer == NULL) {
        // Allocate output buffer
        jpeg_buffer = (uint8_t*)heap_caps_malloc(JPEG_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
        if (!jpeg_buffer) {
            // Fallback to internal RAM
            jpeg_buffer = (uint8_t*)malloc(JPEG_BUFFER_SIZE);
            if (!jpeg_buffer) {
                ESP_LOGE(STRMTAG, "Failed to allocate JPEG buffer");
                return ESP_FAIL;
            }
            ESP_LOGW(STRMTAG, "JPEG buffer in internal RAM");
        }

        // Allocate state structure
        jpeg_state = (TJEState*)malloc(sizeof(TJEState));
        if (!jpeg_state) {
            free(jpeg_buffer);
            jpeg_buffer = NULL;
            ESP_LOGE(STRMTAG, "Failed to allocate JPEG state");
            return ESP_FAIL;
        }

        // Initialize state (TinyJPEG uses zero-init)
        memset(jpeg_state, 0, sizeof(TJEState));

        ESP_LOGI(STRMTAG, "TinyJPEG initialized, buffer: %d KB", JPEG_BUFFER_SIZE / 1024);
    }
    return ESP_OK;
}

// Custom write function for TinyJPEG to write to our buffer
static void jpeg_write_func(void* context, void* data, int size) {
    uint8_t** buffer_ptr = (uint8_t**)context;
    memcpy(*buffer_ptr, data, size);
    *buffer_ptr += size;
}

// Convert grayscale to JPEG
static size_t grayscale_to_jpeg(uint8_t* grayscale_data, int width, int height, int quality) {
    if (init_jpeg_encoder_safe() != ESP_OK) {
        return 0;
    }

    // TinyJPEG expects RGB or RGBA. We need to convert grayscale to RGB.
    // Allocate temporary RGB buffer
    uint8_t* rgb_buffer = (uint8_t*)malloc(width * height * 3);
    if (!rgb_buffer) {
        ESP_LOGE(STRMTAG, "Failed to allocate RGB buffer");
        return 0;
    }

    // Convert grayscale to RGB (duplicate to all channels)
    for (int i = 0; i < width * height; i++) {
        uint8_t gray = grayscale_data[i];
        rgb_buffer[i * 3 + 0] = gray;  // R
        rgb_buffer[i * 3 + 1] = gray;  // G
        rgb_buffer[i * 3 + 2] = gray;  // B
    }

    // Setup write context
    uint8_t* buffer_ptr = jpeg_buffer;

    // Encode with TinyJPEG
    int result = tje_encode_with_func(
        jpeg_write_func,     // Our custom write function
        &buffer_ptr,         // Context: pointer to our buffer pointer
        quality,             // Quality (1-3): 3=best, 1=smallest
        width, height,       // Image dimensions
        3,                   // 3 components (RGB)
        rgb_buffer           // Source RGB data
    );

    // Free temporary buffer
    free(rgb_buffer);

    if (!result) {
        ESP_LOGE(STRMTAG, "TinyJPEG encoding failed");
        return 0;
    }

    size_t jpeg_size = buffer_ptr - jpeg_buffer;
    ESP_LOGD(STRMTAG, "JPEG encoded: %d bytes", jpeg_size);

    return jpeg_size;
}

// Cleanup
static void cleanup_jpeg_encoder(void) {
    if (jpeg_buffer) {
        free(jpeg_buffer);
        jpeg_buffer = NULL;
    }
    if (jpeg_state) {
        free(jpeg_state);
        jpeg_state = NULL;
    }
}

// ---------------- stream_mjpeg_handler() -------------------------------------
static esp_err_t frame_handler(httpd_req_t* req) {
    // log the full uri including query params
    ESP_LOGI(STRMTAG, "Request URI: %s", req->uri);

    buffer_pool_t* pool = get_buffer_pool();
    if (!pool) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Get frame with mutex
    camera_fb_t* fb = NULL;
    if (xSemaphoreTake(pool->display_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(STRMTAG, "Mutex timeout");
        httpd_resp_send_408(req); // Request timeout
        return ESP_FAIL;
    }

    fb = get_display_fb();
    xSemaphoreGive(pool->display_mutex);

    if (!fb) {
        ESP_LOGW(STRMTAG, "No frame available");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Convert to JPEG
    int64_t encode_start = esp_timer_get_time();
    size_t jpeg_size = grayscale_to_jpeg(fb->buf, fb->width, fb->height, 2);
    int64_t encode_time = esp_timer_get_time() - encode_start;
    ESP_LOGI(STRMTAG, "JPEG encode took %lld ms", encode_time / 1000);

        if (jpeg_size == 0 || jpeg_buffer == NULL) {
        ESP_LOGE(STRMTAG, "JPEG encoding failed or buffer is NULL");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Send as single JPEG with NO-CACHE headers
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");

    char clen[20];
    snprintf(clen, sizeof(clen), "%d", jpeg_size);
    httpd_resp_set_hdr(req, "Content-Length", clen);

    // send response
    esp_err_t ret = httpd_resp_send(req, (char*)jpeg_buffer, jpeg_size);

    ESP_LOGD(STRMTAG, "Frame sent: %d bytes", jpeg_size);
    return ret;
}

static esp_err_t root_handler(httpd_req_t* req) {

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*)webpage_html, webpage_html_len);
    return ESP_OK;
}


void stream_register_handlers(httpd_handle_t server) {
  // root page
  httpd_uri_t root_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = root_handler,
      .user_ctx = NULL
  };
  httpd_register_uri_handler(server, &root_uri);

  // MJPEG stream
  httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = frame_handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(server, &stream_uri);

  ESP_LOGI(STRMTAG, "root and /stream handlers registered");

}

void stream_unregister_handlers(httpd_handle_t server) {
    httpd_unregister_uri(server, "/");
    httpd_unregister_uri(server, "/stream");
}