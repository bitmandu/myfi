# myfi

This is an [ESP-IDF][1] component to connect to a Wi-Fi access point.
It is basically a combination of the [DHCP Wi-Fi Station][2] and
[static IP][3] examples.

## Features

- Works with all ESP32 targets
- Connect using either DHCP or a static IP address
- Uses the ESP-NETIF API that is recommended for ESP-IDF v4.1 and above

## Installation

From the `components/` directory of your ESP-IDF project:

    $ git submodule add https://github.com/bitmandu/myfi.git

## Configuration

    $ idf.py menuconfig

In the `MyFi Configuration` menu, update your network settings.

### Static IP DNS Server

By default, no DNS server is configured if you assign a static IP
address. No problem if you only use numeric IP addresses in your
project. However, if you want to resolve hostnames like pool.ntp.org
you will need to add at least a primary DNS server. Your ISP likely
provides DNS servers, but if in doubt, you can try the Google Public
DNS servers 8.8.8.8 and 8.8.4.4.

## Usage

    void app_main(void)
    {
        // initialize non-volatile storage
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);

        if (myfi_connect() != ESP_OK) {

            /* handle error... maybe do stuff that doesn't need Wi-Fi? */

        } else {

            /* ... the rest of your project ... */
        }
    }

## Contributing

[Pull requests][pulls] and [issue/bug reports][issues] are very much
encouraged!

## License

[MIT](LICENSE)


[issues]: https://github.com/bitmandu/myfi/issues
[pulls]: https://github.com/bitmandu/myfi/pulls
[1]: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html
[2]: https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station
[3]: https://github.com/espressif/esp-idf/tree/master/examples/protocols/static_ip
[4]: https://git-scm.com/book/en/v2/Git-Tools-Submodules
[5]: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html
