/**
 * @file myfi.c
 *
 * Connect to Wi-Fi access point.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"

static const char *TAG = "myfi";

/** FreeRTOS event group to signal when we are connected */
static EventGroupHandle_t myfi_event_group;

/** Event bit indicating a successful connection to the AP with an IP */
#define MYFI_CONNECT_BIT BIT0

/** Event bit indicating failure to connect after the maximum number of attempts */
#define MYFI_FAIL_BIT BIT1

/** Number of retry attempts */
static int myfi_retry_num = 0;

#ifdef CONFIG_MYFI_USE_STATIC_IP
/**
 * Set DNS server.
 */
static void myfi_set_dns(esp_netif_t *netif, esp_netif_dns_type_t dns_type,
                         const char *ip)
{
    uint32_t addr = ipaddr_addr(ip);
    esp_netif_dns_info_t dns = {.ip = {{.ip4 = {addr}}, IPADDR_TYPE_V4}};

    if (addr != 0 && esp_netif_set_dns_info(netif, dns_type, &dns) != ESP_OK) {
        ESP_LOGE(TAG, "unable to set DNS server type %d: %s", dns_type, ip);
    }
}

/**
 * Set static IP address and DNS servers.
 */
static void myfi_set_static_ip(esp_netif_t *netif)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "unable to stop DHCP client");
        return;
    }

    esp_netif_ip_info_t ip = {.ip.addr = ipaddr_addr(CONFIG_MYFI_IP),
                              .netmask.addr = ipaddr_addr(CONFIG_MYFI_NETMASK),
                              .gw.addr = ipaddr_addr(CONFIG_MYFI_GATEWAY)};
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
        ESP_LOGE(TAG, "unable to set static IP");
        return;
    }

    myfi_set_dns(netif, ESP_NETIF_DNS_MAIN, CONFIG_MYFI_DNS_PRIMARY);
    myfi_set_dns(netif, ESP_NETIF_DNS_BACKUP, CONFIG_MYFI_DNS_SECONDARY);
}
#endif

/**
 * Handle WIFI and IP events.
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
#ifdef CONFIG_MYFI_USE_STATIC_IP
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        myfi_set_static_ip(arg);
    }
#endif
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (myfi_retry_num < CONFIG_MYFI_RETRY_MAX) {
            esp_wifi_connect();
            ++myfi_retry_num;
            ESP_LOGI(TAG, "... retry %d to connect", myfi_retry_num);
        } else {
            xEventGroupSetBits(myfi_event_group, MYFI_FAIL_BIT);
	    ESP_LOGI(TAG, "unable to connect to access point");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "connection successful");
        ESP_LOGI(TAG, "  ip: " IPSTR, IP2STR(&event->ip_info.ip));
        myfi_retry_num = 0;
        xEventGroupSetBits(myfi_event_group, MYFI_CONNECT_BIT);
    }
}

esp_err_t myfi_connect(void)
{
    myfi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    assert(netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

#ifdef CONFIG_MYFI_USE_STATIC_IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, netif,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &event_handler, netif,
                                                        &instance_got_ip));
#else
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &event_handler, NULL,
                                                        &instance_got_ip));
#endif

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = CONFIG_MYFI_SSID,
                .password = CONFIG_MYFI_PASSWORD,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {.capable = true, .required = false},
            },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "waiting for connection result...");

    // wait until either the connection is established
    // (MYFI_CONNECT_BIT) or connection failed for the maximum number
    // of retries (MYFI_FAIL_BIT); bits are set by event_handler()
    EventBits_t bits = xEventGroupWaitBits(myfi_event_group,
                                           MYFI_CONNECT_BIT | MYFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    esp_err_t result = ESP_FAIL; // assume the worst

    if (bits & MYFI_CONNECT_BIT) {
        esp_netif_dns_info_t dns;
        ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
        ESP_LOGI(TAG, "  ssid: %s", CONFIG_MYFI_SSID);
        ESP_LOGI(TAG, "  dns: " IPSTR, IP2STR(&(dns.ip.u_addr.ip4)));
        result = ESP_OK;
    } else if (bits & MYFI_FAIL_BIT) {
        ESP_LOGI(TAG, "unable to connect to %s", CONFIG_MYFI_SSID);
        esp_netif_destroy_default_wifi(netif);
        result = ESP_ERR_TIMEOUT;
    } else {
        ESP_LOGE(TAG, "unknown event");
        result = ESP_FAIL;
    }

    // events will not be processed after unregister
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                          instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                          instance_any_id));
    vEventGroupDelete(myfi_event_group);

    return result;
}
