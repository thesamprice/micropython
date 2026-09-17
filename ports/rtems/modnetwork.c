/* SPDX-License-Identifier: MIT */

/*
 * network.WLAN for the RTEMS port, on the ESP32-C3's WiFi.
 *
 * This is the piece that lets a script bring an interface up.  On the Zynq
 * lane start_networking() does it from C with a static address, because which
 * controller a board has and how it is wired is settled when the BSP is built
 * and there is nothing for a script to choose.  A radio is different: the
 * network to join, the passphrase and whether to use DHCP are exactly the
 * things that belong in configuration rather than in a firmware image.
 *
 * It sits on two layers that already work and adds no new mechanism:
 * esp_wifi_set_config()/esp_wifi_connect() from the WiFi port, and
 * rtems_esp_netif_*() for the netif that joins that driver to rtems-lwip.
 *
 * Deliberately shaped like the esp32 port's network.WLAN rather than a third
 * arrangement, so a script written against MicroPython on an ESP32 mostly
 * works here: active(), connect(), disconnect(), isconnected(), scan(),
 * status(), ifconfig() and config('mac').
 */

#include "py/runtime.h"

#include <rtems/bspIo.h>
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/mphal.h"

#include <string.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <rtems-esp/netif.h>

#include <lwip/netif.h>
#include <lwip/netifapi.h>
#include <lwip/dhcp.h>
#include <lwip/ip4_addr.h>

#include <rtems.h>

#define WLAN_STA_IF 0

typedef struct _wlan_obj_t {
    mp_obj_base_t base;
    int           interface;
} wlan_obj_t;

extern const mp_obj_type_t wlan_type;

static wlan_obj_t wlan_sta_obj = { { &wlan_type }, WLAN_STA_IF };

/*
 * Connection state, kept here rather than asked of the driver.
 *
 * esp_wifi_sta_get_ap_info() answers only while associated, so it cannot
 * distinguish "never tried" from "tried and failed", and status() has to.  The
 * events carry that and nothing else does.
 */
static volatile bool     wlan_started;
static volatile bool     wlan_connected;
static volatile uint8_t  wlan_last_reason;

static void wlan_event(void *arg, const char *base, int32_t id, void *data) {
    (void)arg;
    (void)base;

    #ifdef WLAN_TRACE_EVENTS
    printk("wlan: event base=%s id=%d\n", base ? (const char *)base : "?", (int)id);
#endif
    switch (id) {
        case WIFI_EVENT_STA_CONNECTED:
            wlan_connected = true;
            break;
        case WIFI_EVENT_STA_DISCONNECTED: {
            wlan_connected = false;
            if (data != NULL) {
                const wifi_event_sta_disconnected_t *d = data;
                wlan_last_reason = d->reason;
#ifdef WLAN_TRACE_EVENTS
                /* The poll loop only reports a reason when it times out, and
                 * a station that retries forever never gets there. */
                printk("wlan: disconnected, reason %u\n", (unsigned)d->reason);
#endif
            }
            break;
        }
        default:
            break;
    }
}

static void wlan_check(esp_err_t rv, const char *what) {
    if (rv != ESP_OK) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%s failed: %d"), what, (int)rv);
    }
}

static mp_obj_t wlan_make_new(const mp_obj_type_t *type, size_t n_args,
                              size_t n_kw, const mp_obj_t *args) {
    (void)type;
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    if (n_args > 0 && mp_obj_get_int(args[0]) != WLAN_STA_IF) {
        mp_raise_ValueError(MP_ERROR_TEXT("only STA_IF is supported"));
    }

    return MP_OBJ_FROM_PTR(&wlan_sta_obj);
}

/*
 * active(True) is where the driver actually starts.
 *
 * esp_wifi_init() and the netif are done once by the application before
 * MicroPython runs -- they need a linker script, blob sections and an event
 * loop, none of which belong in a Python call -- so this is set_mode() plus
 * start(), which is the part that is safe to repeat and to undo.
 */
static mp_obj_t wlan_active(size_t n_args, const mp_obj_t *args) {
    if (n_args > 1) {
        bool on = mp_obj_is_true(args[1]);

        if (on && !wlan_started) {
            wlan_check(esp_wifi_set_mode(WIFI_MODE_STA), "esp_wifi_set_mode");
            wlan_check(esp_wifi_start(), "esp_wifi_start");
            /*
             * Power save off.  The default is WIFI_PS_MIN_MODEM, which needs
             * the sleep side of the OS adapter that this port does not have:
             * the station associates and then stops hearing beacons.
             */
            (void)esp_wifi_set_ps(WIFI_PS_NONE);
            wlan_started = true;
        } else if (!on && wlan_started) {
            (void)esp_wifi_disconnect();
            wlan_check(esp_wifi_stop(), "esp_wifi_stop");
            wlan_started = false;
            wlan_connected = false;
        }
    }

    return mp_obj_new_bool(wlan_started);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_active_obj, 1, 2, wlan_active);

static mp_obj_t wlan_connect(size_t n_args, const mp_obj_t *args) {
    wifi_config_t cfg;
    const char   *ssid;
    const char   *key = "";
    size_t        ssid_len;
    size_t        key_len = 0;

    if (!wlan_started) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("interface is not active"));
    }

    ssid = mp_obj_str_get_data(args[1], &ssid_len);

    if (n_args > 2 && args[2] != mp_const_none) {
        key = mp_obj_str_get_data(args[2], &key_len);
    }

    memset(&cfg, 0, sizeof(cfg));

    if (ssid_len >= sizeof(cfg.sta.ssid) || key_len >= sizeof(cfg.sta.password)) {
        mp_raise_ValueError(MP_ERROR_TEXT("SSID or key too long"));
    }

    memcpy(cfg.sta.ssid, ssid, ssid_len);
    memcpy(cfg.sta.password, key, key_len);

    /*
     * Strongest access point, not the first acceptable one.  The default sorts
     * by security, and with several BSSes sharing an SSID that reliably picks
     * a distant one -- which associates and then cannot pass data, because
     * management frames survive a weak link and data frames do not.
     */
    cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

    wlan_check(esp_wifi_set_config(WIFI_IF_STA, &cfg), "esp_wifi_set_config");
    wlan_last_reason = 0;
    wlan_check(esp_wifi_connect(), "esp_wifi_connect");

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_connect_obj, 2, 3, wlan_connect);

static mp_obj_t wlan_disconnect(mp_obj_t self_in) {
    (void)self_in;
    wlan_check(esp_wifi_disconnect(), "esp_wifi_disconnect");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(wlan_disconnect_obj, wlan_disconnect);

static mp_obj_t wlan_isconnected(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(wlan_connected);
}
static MP_DEFINE_CONST_FUN_OBJ_1(wlan_isconnected_obj, wlan_isconnected);

/*
 * status() reports association, and status('rssi') the signal.
 *
 * The bare form returns the last disconnect reason when not associated, which
 * is the number worth having: 201 is "no access point of that name", 15 is a
 * wrong passphrase, and they call for different fixes.
 */
static mp_obj_t wlan_status(size_t n_args, const mp_obj_t *args) {
    if (n_args > 1) {
        wifi_ap_record_t ap;

        if (mp_obj_str_get_qstr(args[1]) != MP_QSTR_rssi) {
            mp_raise_ValueError(MP_ERROR_TEXT("unknown status parameter"));
        }

        if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("not connected"));
        }

        return mp_obj_new_int(ap.rssi);
    }

    if (wlan_connected) {
        return mp_obj_new_int(1);
    }

    return mp_obj_new_int(-(int)wlan_last_reason);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_status_obj, 1, 2, wlan_status);

/*
 * scan() returns the esp32 port's 6-tuple:
 * (ssid, bssid, channel, RSSI, security, hidden).
 */
static mp_obj_t wlan_scan(mp_obj_t self_in) {
    wifi_ap_record_t *records;
    mp_obj_t          list;
    uint16_t          found = 0;
    uint16_t          n;
    uint16_t          i;

    (void)self_in;

    if (!wlan_started) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("interface is not active"));
    }

    wlan_check(esp_wifi_scan_start(NULL, true), "esp_wifi_scan_start");
    wlan_check(esp_wifi_scan_get_ap_num(&found), "esp_wifi_scan_get_ap_num");

    list = mp_obj_new_list(0, NULL);

    if (found == 0) {
        return list;
    }

    records = m_new(wifi_ap_record_t, found);
    n = found;

    if (esp_wifi_scan_get_ap_records(&n, records) == ESP_OK) {
        for (i = 0; i < n; i++) {
            mp_obj_t tuple[6] = {
                mp_obj_new_str((const char *)records[i].ssid,
                               strlen((const char *)records[i].ssid)),
                mp_obj_new_bytes(records[i].bssid, 6),
                mp_obj_new_int(records[i].primary),
                mp_obj_new_int(records[i].rssi),
                mp_obj_new_int(records[i].authmode),
                mp_obj_new_bool(records[i].ssid[0] == '\0'),
            };
            mp_obj_list_append(list, mp_obj_new_tuple(6, tuple));
        }
    }

    m_del(wifi_ap_record_t, records, found);

    return list;
}
static MP_DEFINE_CONST_FUN_OBJ_1(wlan_scan_obj, wlan_scan);

/*
 * ifconfig() with no argument reports; with a 4-tuple it sets a static
 * address; with the string 'dhcp' it starts DHCP.
 *
 * netifapi_dhcp_start() rather than dhcp_start(): that call rewrites the
 * netif's addresses and arms timeouts, which is core work, and this runs on
 * whichever task the interpreter is on.
 */
static mp_obj_t wlan_ifconfig(size_t n_args, const mp_obj_t *args) {
    struct netif *netif = rtems_esp_netif_get();

    if (netif == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("no interface"));
    }

    if (n_args == 1) {
        char     buf[4][IP4ADDR_STRLEN_MAX];
        mp_obj_t tuple[4];
        const ip4_addr_t *a[4];

        a[0] = netif_ip4_addr(netif);
        a[1] = netif_ip4_netmask(netif);
        a[2] = netif_ip4_gw(netif);
        a[3] = netif_ip4_gw(netif);

        for (int i = 0; i < 4; i++) {
            ip4addr_ntoa_r(a[i], buf[i], sizeof(buf[i]));
            tuple[i] = mp_obj_new_str(buf[i], strlen(buf[i]));
        }

        return mp_obj_new_tuple(4, tuple);
    }

    if (mp_obj_is_str(args[1])) {
        if (mp_obj_str_get_qstr(args[1]) != MP_QSTR_dhcp) {
            mp_raise_ValueError(MP_ERROR_TEXT("expected a 4-tuple or 'dhcp'"));
        }

        if (netifapi_dhcp_start(netif) != ERR_OK) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("dhcp_start failed"));
        }

        return mp_const_none;
    }

    {
        mp_obj_t *items;
        ip4_addr_t addr, mask, gw;

        mp_obj_get_array_fixed_n(args[1], 4, &items);

        if (!ip4addr_aton(mp_obj_str_get_str(items[0]), &addr) ||
            !ip4addr_aton(mp_obj_str_get_str(items[1]), &mask) ||
            !ip4addr_aton(mp_obj_str_get_str(items[2]), &gw)) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad address"));
        }

        (void)netifapi_dhcp_stop(netif);
        netifapi_netif_set_addr(netif, &addr, &mask, &gw);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_ifconfig_obj, 1, 2, wlan_ifconfig);

static mp_obj_t wlan_config(size_t n_args, const mp_obj_t *args) {
    uint8_t mac[6];

    if (n_args != 2 || mp_obj_str_get_qstr(args[1]) != MP_QSTR_mac) {
        mp_raise_ValueError(MP_ERROR_TEXT("only config('mac') is supported"));
    }

    wlan_check(esp_wifi_get_mac(WIFI_IF_STA, mac), "esp_wifi_get_mac");

    return mp_obj_new_bytes(mac, 6);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_config_obj, 1, 2, wlan_config);

static const mp_rom_map_elem_t wlan_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_active),      MP_ROM_PTR(&wlan_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect),     MP_ROM_PTR(&wlan_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect),  MP_ROM_PTR(&wlan_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&wlan_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_status),      MP_ROM_PTR(&wlan_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan),        MP_ROM_PTR(&wlan_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig),    MP_ROM_PTR(&wlan_ifconfig_obj) },
    { MP_ROM_QSTR(MP_QSTR_config),      MP_ROM_PTR(&wlan_config_obj) },
};
static MP_DEFINE_CONST_DICT(wlan_locals, wlan_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    wlan_type,
    MP_QSTR_WLAN,
    MP_TYPE_FLAG_NONE,
    make_new, wlan_make_new,
    locals_dict, &wlan_locals
    );

/*
 * Registers the event handler the module needs.
 *
 * Called once from the application rather than on import, because the event
 * loop has to exist first and creating it is the application's job.
 */
void rtems_micropython_network_init(void) {
    (void)esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wlan_event, NULL);
}

static const mp_rom_map_elem_t mp_module_network_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_network) },
    { MP_ROM_QSTR(MP_QSTR_WLAN),     MP_ROM_PTR(&wlan_type) },
    { MP_ROM_QSTR(MP_QSTR_STA_IF),   MP_ROM_INT(WLAN_STA_IF) },
};
static MP_DEFINE_CONST_DICT(mp_module_network_globals, mp_module_network_globals_table);

const mp_obj_module_t mp_module_network = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_network_globals,
};

MP_REGISTER_MODULE(MP_QSTR_network, mp_module_network);
