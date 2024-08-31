#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define FLASH_TARGET_OFFSET_A 0x00060000
#define FLASH_TARGET_OFFSET_B (FLASH_TARGET_OFFSET_A + FLASH_SECTOR_SIZE)
#define FLASH_TARGET_OFFSET_C (FLASH_TARGET_OFFSET_B + FLASH_SECTOR_SIZE)
#define TCP_PORT 4242
#define BUF_SIZE 1024
#define POWER_CONTROL_PIN 20
#define AUTO_EXTENSION_PIN 16
#define MAX_REQUESTS_PER_SECOND 1

typedef enum
{
    POWER_CONTROL = 0,
    AUTO_EXTENSION = 1,
    AE_PUSH_SPAN_MINUTE = 2
} DEVICE_STATE_LIST_T;

typedef struct ALL_STATES_T_
{
    bool is_on;
    uint32_t offset;
} STATE_CONTROL_T;

static STATE_CONTROL_T power_control_s = {
    .is_on = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET_A),
    .offset = FLASH_TARGET_OFFSET_A,
};
static STATE_CONTROL_T auto_extension_s = {
    .is_on = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET_B),
    .offset = FLASH_TARGET_OFFSET_B,
};
static STATE_CONTROL_T ae_push_span_minute_s = {
    .is_on = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET_C),
    .offset = FLASH_TARGET_OFFSET_C,
};

uint8_t read_flash_data(STATE_CONTROL_T state_control)
{
    return *(const uint8_t *)(XIP_BASE + state_control.offset);
}

void write_flash_data(STATE_CONTROL_T state_control, uint8_t data)
{
    uint32_t ints = save_and_disable_interrupts();
    uint8_t buffer[FLASH_PAGE_SIZE] = {0xFF};
    buffer[0] = data;
    flash_range_erase(state_control.offset, FLASH_SECTOR_SIZE);
    flash_range_program(state_control.offset, buffer, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

void set_data(DEVICE_STATE_LIST_T state, uint8_t value)
{
    switch (state)
    {
    case POWER_CONTROL:
        return write_flash_data(power_control_s, value);
    case AUTO_EXTENSION:
        return write_flash_data(auto_extension_s, value);
    case AE_PUSH_SPAN_MINUTE:
        return write_flash_data(ae_push_span_minute_s, value);
    default:
        return 0xFF;
    };
}

uint8_t get_data(DEVICE_STATE_LIST_T state)
{
    switch (state)
    {
    case POWER_CONTROL:
        return read_flash_data(power_control_s);
    case AUTO_EXTENSION:
        return read_flash_data(auto_extension_s);
    case AE_PUSH_SPAN_MINUTE:
        return read_flash_data(ae_push_span_minute_s);
    default:
        return 0xFF;
    };
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    static int request_count = 0;
    static absolute_time_t last_reset_time;

    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_reset_time, now) > 1000000)
    {
        printf("Resetting request count\n");
        request_count = 0;
        last_reset_time = now;
    }
    if (request_count >= MAX_REQUESTS_PER_SECOND)
    {
        printf("Too many requests, closing connection.\n");
        tcp_close(tpcb);
        return ERR_OK;
    }
    request_count++;
    printf("Request count: %d\n", request_count);
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *data = memcpy(malloc(p->tot_len + 1), p->payload, p->tot_len);
    data[p->tot_len] = '\0';
    printf("Received data: %s\n", data);
    if (strncmp(data, "setAE:", 6) == 0)
    {
        uint8_t value = atoi(data + 6);
        set_data(AUTO_EXTENSION, value);
    }
    else if (strncmp(data, "setPC:", 6) == 0)
    {
        uint8_t value = atoi(data + 6);
        set_data(POWER_CONTROL, value);
    }
    else if (strncmp(data, "set_AE_MINUTE:", 14) == 0)
    {
        uint8_t value = atoi(data + 14);
        set_data(AE_PUSH_SPAN_MINUTE, value);
    }
    else if (strncmp(data, "getAll", 6) == 0)
    {
        uint8_t a_value = get_data(POWER_CONTROL);
        uint8_t b_value = get_data(AUTO_EXTENSION);
        uint8_t c_value = get_data(AE_PUSH_SPAN_MINUTE);
        char response[50];
        snprintf(response, sizeof(response), "{\"PC\"=%d,\"AE\"=%d,\"AE_MINUTE\"=%d}", a_value, b_value, c_value);
        tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    }
    else
    {
        printf("Unknown command received\n");
    }

    pbuf_free(p);
    free(data);
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    tcp_recv(client_pcb, tcp_server_recv);
    return ERR_OK;
}

static void start_tcp_server(void)
{
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (pcb)
    {
        tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
        pcb = tcp_listen(pcb);
        tcp_accept(pcb, tcp_server_accept);
    }
}

int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        printf("Wi-Fi initialization failed!\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    gpio_init(AUTO_EXTENSION_PIN);
    gpio_set_dir(AUTO_EXTENSION_PIN, GPIO_OUT);
    gpio_put(AUTO_EXTENSION_PIN, false);

    while (true)
    {
        printf("Connecting to Wi-Fi...\n");

        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
        {
            printf("Connection failed, retrying...\n");
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // LED off
            sleep_ms(1000);
        }
        else
        {
            printf("Connected to Wi-Fi!\n");
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // LED on
            start_tcp_server();
            break;
        }
    }

    while (true)
    {
        printf("read_flash_data(auto_extension_s) = %d\n", read_flash_data(auto_extension_s));
        if (read_flash_data(auto_extension_s))
        {
            gpio_put(AUTO_EXTENSION_PIN, true);
            sleep_ms(2 * 1000);
        }
        gpio_put(AUTO_EXTENSION_PIN, false);
        sleep_ms(1000 * 60 * read_flash_data(ae_push_span_minute_s));
    }
    cyw43_arch_deinit();
    return 0;
}
