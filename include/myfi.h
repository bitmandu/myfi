/**
 * @file myfi.h
 *
 * Connect to Wi-Fi access point.
 */

#pragma once

/**
 * Connect to Wi-Fi access point.
 *
 * @return
 *   - ESP_OK: Sucess
 *   - ESP_ERR_TIMEOUT: If unable to connect in CONFIG_MYFI_RETRY_MAX attempts
 *   - ESP_FAIL: Otherwise
 */
esp_err_t myfi_connect(void);
