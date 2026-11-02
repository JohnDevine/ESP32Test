/**
 * @file ESP32Test.c
 * @brief Wi-Fi Access Point scanner for the ESP32.
 *
 * @details
 * This application initializes the ESP-IDF Wi-Fi stack in station mode, performs a
 * blocking scan for nearby access points, and logs information about every AP found
 * (SSID, RSSI, authentication mode, cipher suites, and channel).
 *
 * The scan can optionally be limited to channels 1, 6, and 11 by defining
 * @c CONFIG_EXAMPLE_USE_SCAN_CHANNEL_BITMAP. The maximum number of results stored
 * is controlled by @c CONFIG_EXAMPLE_SCAN_LIST_SIZE (defaults to 10).
 *
 * @note This example is based on the ESP-IDF Wi-Fi scan example and is released
 *       into the Public Domain (or CC0 licensed, at your option).
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "regex.h"

/**
 * @brief Maximum number of AP records that can be stored in the result array.
 *
 * Overridable via @c CONFIG_EXAMPLE_SCAN_LIST_SIZE in sdkconfig; defaults to 10.
 */
#ifndef CONFIG_EXAMPLE_SCAN_LIST_SIZE
#define CONFIG_EXAMPLE_SCAN_LIST_SIZE 10
#endif
#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE

/**
 * @brief Optional channel list used when selective scanning is enabled.
 *
 * Only compiled when @c CONFIG_EXAMPLE_USE_SCAN_CHANNEL_BITMAP is defined.
 */
#ifdef CONFIG_EXAMPLE_USE_SCAN_CHANNEL_BITMAP
#define USE_CHANNEL_BITMAP 1
#define CHANNEL_LIST_SIZE 3
static uint8_t channel_list[CHANNEL_LIST_SIZE] = {1, 6, 11};
#endif /*CONFIG_EXAMPLE_USE_SCAN_CHANNEL_BITMAP*/

/**
 * @brief Log tag identifying messages emitted by this module.
 */
static const char *TAG = "scan";

/**
 * @brief Log a human-readable name for a Wi-Fi authentication mode.
 *
 * @param[in] authmode Authentication mode value from a @c wifi_ap_record_t.
 */
static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_OWE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OWE");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA3_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_ENT_192:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENT_192");
        break;
    case WIFI_AUTH_DPP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_DPP");
        break;
    case WIFI_AUTH_UNKNOWN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

/**
 * @brief Log the pairwise and group cipher suites of an access point.
 *
 * @param[in] pairwise_cipher Pairwise cipher type from a @c wifi_ap_record_t.
 * @param[in] group_cipher    Group cipher type from a @c wifi_ap_record_t.
 */
static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

#ifdef USE_CHANNEL_BITMAP
/**
 * @brief Convert a channel list into a 2.4 GHz channel bitmap.
 *
 * @param[in]  channel_list      Array of 2.4 GHz channel numbers to enable.
 * @param[in]  channel_list_size Number of entries in @p channel_list.
 * @param[out] scan_config       Scan configuration whose bitmap is populated.
 */
static void array_2_channel_bitmap(const uint8_t channel_list[], const uint8_t channel_list_size, wifi_scan_config_t *scan_config) {

    for(uint8_t i = 0; i < channel_list_size; i++) {
        uint8_t channel = channel_list[i];
        scan_config->channel_bitmap.ghz_2_channels |= (1 << channel);
    }
}
#endif /*USE_CHANNEL_BITMAP*/

/**
 * @brief Convert a Wi-Fi channel number to its centre frequency.
 *
 * @param[in] channel Primary channel number.
 * @return Centre frequency in MHz, or 0 if the channel is unknown.
 */
static uint16_t channel_to_freq_mhz(uint8_t channel)
{
    if (channel == 14) {
        return 2484;
    }
    if (channel >= 1 && channel <= 13) {
        return 2407 + (5 * channel);
    }
    if (channel >= 36 && channel <= 177) {
        return 5000 + (5 * channel);
    }
    return 0;
}

/**
 * @brief Return the frequency band that owns a channel.
 *
 * @param[in] channel Primary channel number.
 * @return Human-readable band name.
 */
static const char *channel_to_band(uint8_t channel)
{
    if (channel >= 1 && channel <= 14) {
        return "2.4 GHz";
    }
    if (channel >= 36 && channel <= 177) {
        return "5 GHz";
    }
    return "unknown";
}

/**
 * @brief Describe the HT40 secondary channel setting.
 *
 * @param[in] second Secondary channel type from a @c wifi_ap_record_t.
 * @return Human-readable description.
 */
static const char *second_chan_to_str(wifi_second_chan_t second)
{
    switch (second) {
    case WIFI_SECOND_CHAN_NONE:
        return "none (HT20)";
    case WIFI_SECOND_CHAN_ABOVE:
        return "above (HT40+)";
    case WIFI_SECOND_CHAN_BELOW:
        return "below (HT40-)";
    default:
        return "unknown";
    }
}

/**
 * @brief Describe the AP bandwidth.
 *
 * @param[in] bandwidth Bandwidth value from a @c wifi_ap_record_t.
 * @return Human-readable bandwidth string.
 */
static const char *bandwidth_to_str(wifi_bandwidth_t bandwidth)
{
    switch (bandwidth) {
    case WIFI_BW20:
        return "20 MHz";
    case WIFI_BW40:
        return "40 MHz";
    case WIFI_BW80:
        return "80 MHz";
    case WIFI_BW160:
        return "160 MHz";
    case WIFI_BW80_BW80:
        return "80+80 MHz";
    default:
        return "unknown";
    }
}

/**
 * @brief Describe the receive antenna used for the AP.
 *
 * @param[in] ant Antenna value from a @c wifi_ap_record_t.
 * @return Human-readable antenna name.
 */
static const char *antenna_to_str(wifi_ant_t ant)
{
    switch (ant) {
    case WIFI_ANT_ANT0:
        return "ANT0";
    case WIFI_ANT_ANT1:
        return "ANT1";
    default:
        return "unknown";
    }
}

/**
 * @brief Append a string to a fixed-size buffer without overflowing it.
 *
 * @param[in,out] dst      Destination buffer.
 * @param[in]     dst_size Total size of @p dst.
 * @param[in]     src      String to append.
 */
static void append_str(char *dst, size_t dst_size, const char *src)
{
    size_t len = strlen(dst);
    if (len < dst_size - 1) {
        strncat(dst, src, dst_size - 1 - len);
    }
}

/**
 * @brief Log the PHY modes advertised by an access point.
 *
 * @param[in] ap Access point record.
 */
static void print_phy_modes(const wifi_ap_record_t *ap)
{
    char modes[80] = "";

    if (ap->phy_11b) {
        append_str(modes, sizeof(modes), "11b ");
    }
    if (ap->phy_11g) {
        append_str(modes, sizeof(modes), "11g ");
    }
    if (ap->phy_11n) {
        append_str(modes, sizeof(modes), "11n ");
    }
    if (ap->phy_lr) {
        append_str(modes, sizeof(modes), "LR ");
    }
    if (ap->phy_11a) {
        append_str(modes, sizeof(modes), "11a ");
    }
    if (ap->phy_11ac) {
        append_str(modes, sizeof(modes), "11ac ");
    }
    if (ap->phy_11ax) {
        append_str(modes, sizeof(modes), "11ax ");
    }

    ESP_LOGI(TAG, "PHY modes \t%s", modes[0] ? modes : "none");
}

/**
 * @brief Initialize the Wi-Fi stack in station mode, scan, and log the results.
 *
 * Performs a blocking scan (or a channel-limited scan when
 * @c CONFIG_EXAMPLE_USE_SCAN_CHANNEL_BITMAP is enabled) and prints up to
 * @c DEFAULT_SCAN_LIST_SIZE access points.
 */
static void wifi_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

#ifdef USE_CHANNEL_BITMAP
    wifi_scan_config_t *scan_config = (wifi_scan_config_t *)calloc(1,sizeof(wifi_scan_config_t));
    if (!scan_config) {
        ESP_LOGE(TAG, "Memory Allocation for scan config failed!");
        return;
    }
    array_2_channel_bitmap(channel_list, CHANNEL_LIST_SIZE, scan_config);
    esp_wifi_scan_start(scan_config, true);
    free(scan_config);

#else
    esp_wifi_scan_start(NULL, true);
#endif /*USE_CHANNEL_BITMAP*/

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

#if CONFIG_SOC_WIFI_SUPPORT_5G
    ESP_LOGI(TAG, "Note: this chip supports both 2.4 GHz and 5 GHz scans.");
#else
    ESP_LOGI(TAG, "Note: this chip is 2.4 GHz only; 5 GHz results will appear on newer dual-band chips.");
#endif

    for (int i = 0; i < number; i++) {
        const wifi_ap_record_t *ap = &ap_info[i];
        uint16_t freq = channel_to_freq_mhz(ap->primary);

        ESP_LOGI(TAG, "----- AP %d -----", i + 1);
        ESP_LOGI(TAG, "SSID \t\t%s", ap->ssid);
        ESP_LOGI(TAG, "BSSID \t\t%02x:%02x:%02x:%02x:%02x:%02x",
                 ap->bssid[0], ap->bssid[1], ap->bssid[2],
                 ap->bssid[3], ap->bssid[4], ap->bssid[5]);
        ESP_LOGI(TAG, "Band \t\t%s", channel_to_band(ap->primary));
        ESP_LOGI(TAG, "Frequency \t%u MHz", freq);
        ESP_LOGI(TAG, "Channel \t%d (secondary %s)", ap->primary, second_chan_to_str(ap->second));
        ESP_LOGI(TAG, "Bandwidth \t%s", bandwidth_to_str(ap->bandwidth));
        ESP_LOGI(TAG, "RSSI \t\t%d dBm", ap->rssi);
        ESP_LOGI(TAG, "Antenna \t%s", antenna_to_str(ap->ant));
        print_auth_mode(ap->authmode);
        if (ap->authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap->pairwise_cipher, ap->group_cipher);
        }
        print_phy_modes(ap);
        ESP_LOGI(TAG, "WPS \t\t%s", ap->wps ? "yes" : "no");
        ESP_LOGI(TAG, "FTM \t\tresponder %s, initiator %s",
                 ap->ftm_responder ? "yes" : "no",
                 ap->ftm_initiator ? "yes" : "no");
        ESP_LOGI(TAG, "DPP \t\t%s",
                 ap->akm_dpp ? ((ap->authmode != WIFI_AUTH_DPP) ? "supported (mixed mode)" : "supported (DPP-only)") : "no");
        ESP_LOGI(TAG, "Country \t%.2s (2.4 GHz ch %u-%u, max tx %d dBm)",
                 ap->country.cc, ap->country.schan,
                 (ap->country.nchan > 0) ? (ap->country.schan + ap->country.nchan - 1) : ap->country.schan,
                 ap->country.max_tx_power);
        ESP_LOGI(TAG, "HE AP \t\tBSS color %u%s, partial %u, disabled %u, BSSID index %u",
                 ap->he_ap.bss_color,
                 ap->he_ap.partial_bss_color ? " (partial)" : "",
                 ap->he_ap.partial_bss_color,
                 ap->he_ap.bss_color_disabled,
                 ap->he_ap.bssid_index);
        if (ap->bandwidth >= WIFI_BW80) {
            ESP_LOGI(TAG, "VHT center \t%u / %u", ap->vht_ch_freq1, ap->vht_ch_freq2);
        }
        ESP_LOGI(TAG, "-----------------");
    }
}

/**
 * @brief Application entry point.
 *
 * Initializes NVS (erasing and retrying if the partition is full or from an
 * incompatible version) and then starts the Wi-Fi scan.
 */
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_scan();
}
