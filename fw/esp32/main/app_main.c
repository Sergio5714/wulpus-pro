#include "board.h"
#include "double_reset.h"
#include "esp_log.h"
#include "mdns_manager.h"
#include "provisioner.h"
#include "threads.h"
#include "wulpus_pro_frame_pool.h"
#include "wulpus_pro_session.h"
#include "wulpus_pro_state.h"
#include "wulpus_pro_status.h"

static const char *TAG = "main";

void app_main(void)
{
    bool reset_provisioning = false;
    ESP_ERROR_CHECK(board_init());
#if CONFIG_WP_DOUBLE_RESET
    ESP_ERROR_CHECK(double_reset_start(&reset_provisioning,
                                       CONFIG_WP_DOUBLE_RESET_TIMEOUT));
    if (reset_provisioning) {
        ESP_LOGI(TAG, "Double reset detected; provisioning will be reset");
    }
#endif
    ESP_ERROR_CHECK(provisioner_init());
    ESP_ERROR_CHECK(mdns_manager_init("wulpus"));
    ESP_ERROR_CHECK(mdns_manager_add("wulpus", MDNS_PROTO_TCP,
                                     CONFIG_WP_SOCKET_PORT));
    ESP_ERROR_CHECK(wulpus_pro_frame_pool_init(CONFIG_WP_DATA_RX_LENGTH));
    ESP_ERROR_CHECK(wulpus_pro_status_init());
    ESP_ERROR_CHECK(wulpus_pro_state_init());
    ESP_ERROR_CHECK(wulpus_pro_session_init());
    ESP_ERROR_CHECK(threads_start(reset_provisioning));
    ESP_LOGI(TAG, "WULPUS PRO runtime started");
}
