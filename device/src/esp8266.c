#include "esp8266.h"

#include <stdio.h>
#include <string.h>

#include "delay.h"
#include "timebase.h"
#include "esp_usart.h"

esp8266_rx_frame_t g_esp8266_rx;

void ESP8266_ClearRx(void)
{
    g_esp8266_rx.length = 0U;
    g_esp8266_rx.complete = 0U;
    g_esp8266_rx.overflow = 0U;
    g_esp8266_rx.buffer[0] = '\0';
}

const char *ESP8266_GetRxBuffer(void)
{
    g_esp8266_rx.buffer[g_esp8266_rx.length] = '\0';
    return g_esp8266_rx.buffer;
}

static void send_raw(const char *text)
{
    while ((text != 0) && (*text != '\0'))
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
        {
        }
        USART_SendData(USART2, (uint16_t)(uint8_t)*text);
        text++;
    }
}

void ESP8266_WriteLine(const char *line)
{
    ESP8266_ClearRx();
    send_raw(line);
    send_raw("\r\n");
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
    {
    }
}

/*
 * AT 响应经常被拆成多段：每个 UART IDLE 只代表“当前这行结束”。
 * 这里先等待最后一段数据之后再静默 30 ms，然后才拼接检查 ACK。
 */
static bool wait_response(const char *ack1, const char *ack2, uint32_t timeout_ms)
{
    uint32_t start;
    uint32_t quiet_start;
    uint8_t saw_frame;
    const char *response;

    start = Timebase_GetMillis();
    quiet_start = start;
    saw_frame = 0U;

    while (!Timebase_Elapsed(start, timeout_ms))
    {
        if (g_esp8266_rx.complete)
        {
            g_esp8266_rx.complete = 0U;
            saw_frame = 1U;
            quiet_start = Timebase_GetMillis();
        }
        if (saw_frame && Timebase_Elapsed(quiet_start, 30U))
        {
            break;
        }
        delay_us(100U);
    }

    response = ESP8266_GetRxBuffer();
    if (g_esp8266_rx.overflow)
    {
        return false;
    }
    if ((ack1 == 0) && (ack2 == 0))
    {
        return true;
    }
    if ((ack1 != 0) && (strstr(response, ack1) != 0))
    {
        return true;
    }
    if ((ack2 != 0) && (strstr(response, ack2) != 0))
    {
        return true;
    }
    return false;
}

bool ESP8266_Send_AT_Cmd(const char *cmd,
                         const char *ack1,
                         const char *ack2,
                         uint32_t timeout_ms)
{
    if (cmd == 0)
    {
        return false;
    }
    ESP8266_WriteLine(cmd);
    return wait_response(ack1, ack2, timeout_ms);
}

void ESP8266_Rst(void)
{
    GPIO_ResetBits(APP_ESP8266_RST_GPIO, APP_ESP8266_RST_PIN);
    delay_ms(100U);
    GPIO_SetBits(APP_ESP8266_RST_GPIO, APP_ESP8266_RST_PIN);
    delay_ms(1200U);
    ESP8266_ClearRx();
}

void ESP8266_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin = APP_ESP8266_RST_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(APP_ESP8266_RST_GPIO, &gpio);

    gpio.GPIO_Pin = APP_ESP8266_CHPD_PIN;
    GPIO_Init(APP_ESP8266_CHPD_GPIO, &gpio);

    GPIO_SetBits(APP_ESP8266_CHPD_GPIO, APP_ESP8266_CHPD_PIN);
    uart2_Init(baudrate);
    ESP8266_Rst();
}

bool ESP8266_AT_Test(void)
{
    uint8_t retry;

    for (retry = 0U; retry < 3U; retry++)
    {
        if (ESP8266_Send_AT_Cmd("AT", "OK", 0, 1000U))
        {
            return true;
        }
        delay_ms(300U);
    }
    return false;
}

bool ESP8266_Enable_ATE(bool enabled)
{
    return ESP8266_Send_AT_Cmd(enabled ? "ATE1" : "ATE0", "OK", 0, 1000U);
}

bool ESP8266_Net_Mode_Choose(ESP8266_NetMode_t mode)
{
    switch (mode)
    {
        case ESP8266_STA:
            return ESP8266_Send_AT_Cmd("AT+CWMODE=1", "OK", "no change", 2000U);
        case ESP8266_AP:
            return ESP8266_Send_AT_Cmd("AT+CWMODE=2", "OK", "no change", 2000U);
        case ESP8266_STA_AP:
            return ESP8266_Send_AT_Cmd("AT+CWMODE=3", "OK", "no change", 2000U);
        default:
            return false;
    }
}

bool ESP8266_JoinAP(const char *ssid, const char *password)
{
    char command[160];
    uint8_t retry;

    if ((ssid == 0) || (password == 0))
    {
        return false;
    }

    for (retry = 0U; retry < 3U; retry++)
    {
        snprintf(command, sizeof(command),
                 "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
        if (ESP8266_Send_AT_Cmd(command, "WIFI GOT IP", "OK", 20000U))
        {
            return true;
        }
        delay_ms(500U);
    }
    return false;
}

bool ESP8266_MQTTUSERCFG(const char *client_id,
                         const char *username,
                         const char *password)
{
    char command[384];

    if ((client_id == 0) || (username == 0) || (password == 0))
    {
        return false;
    }
    snprintf(command, sizeof(command),
             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
             client_id, username, password);
    return ESP8266_Send_AT_Cmd(command, "OK", 0, 3000U);
}

bool ESP8266_MQTTCONN(const char *host, uint16_t port)
{
    char command[192];

    if (host == 0)
    {
        return false;
    }
    snprintf(command, sizeof(command),
             "AT+MQTTCONN=0,\"%s\",%u,0", host, (unsigned int)port);
    return ESP8266_Send_AT_Cmd(command, "OK", 0, 8000U);
}

bool ESP8266_MQTTSUB(const char *topic)
{
    char command[320];

    if (topic == 0)
    {
        return false;
    }
    snprintf(command, sizeof(command), "AT+MQTTSUB=0,\"%s\",1", topic);
    return ESP8266_Send_AT_Cmd(command, "OK", "ALREADY", 5000U);
}

bool ESP8266_MQTTCLEAN(void)
{
    return ESP8266_Send_AT_Cmd("AT+MQTTCLEAN=0", "OK", 0, 3000U);
}
