# myfi

Fork of the [ESP-IDF/examples/protocols/static_ip][1] component. I
find it easier to include this as a git submodule in my own projects —
maybe you will too.


## Installation

From the `components/` directory of your ESP-IDF project:

    $ git submodule add https://github.com/bitmandu/myfi.git

## Configuration

    $ idf.py menuconfig

In the `MyFi Configuration` menu, update the authentication
information for your Wi-Fi access point.

## Usage

    void app_main(void)
    {
        // initialize non-volatile storage
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        myfi_connect();

        /* ... the rest of your project ... */
    }

## Contributing

[Pull requests][pulls] and [issue/bug reports][issues] are very much
encouraged!

## License

[MIT](LICENSE)


[issues]: https://github.com/bitmandu/myfi/issues
[pulls]: https://github.com/bitmandu/myfi/pulls
[1]: https://github.com/espressif/esp-idf/tree/master/examples/protocols/static_ip
