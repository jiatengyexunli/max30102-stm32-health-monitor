#ifndef ESP8266_H
#define ESP8266_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "stm32f10x.h"

/* USART2 中断写入、AT 驱动读取的共享接收帧。 */
typedef struct
{
    char buffer[APP_ESP8266_RX_BUFFER_SIZE];
    volatile uint16_t length;
    volatile uint8_t complete;
    volatile uint8_t overflow;
} esp8266_rx_frame_t;

extern esp8266_rx_frame_t g_esp8266_rx;

typedef enum
{
    ESP8266_STA = 0,
    ESP8266_AP,
    ESP8266_STA_AP
} ESP8266_NetMode_t;

void ESP8266_Init(uint32_t baudrate);
void ESP8266_Rst(void);
void ESP8266_ClearRx(void);
const char *ESP8266_GetRxBuffer(void);

/*
 * 发送一行 AT 命令并等待响应。
 * ack1/ack2 任一出现即成功；两者都为空时只发送不等待。
 */
bool ESP8266_Send_AT_Cmd(const char *cmd,
                         const char *ack1,
                         const char *ack2,
                         uint32_t timeout_ms);

/*
 * 发送一行 AT 命令，不等待响应。用于周期 MQTT 发布，避免服务器响应
 * 阻塞 MAX30102 的 100 Hz 采样。
 */
void ESP8266_WriteLine(const char *line);

bool ESP8266_AT_Test(void);
bool ESP8266_Enable_ATE(bool enabled);
bool ESP8266_Net_Mode_Choose(ESP8266_NetMode_t mode);
bool ESP8266_JoinAP(const char *ssid, const char *password);

bool ESP8266_MQTTUSERCFG(const char *client_id,
                         const char *username,
                         const char *password);
bool ESP8266_MQTTCONN(const char *host, uint16_t port);
bool ESP8266_MQTTSUB(const char *topic);
bool ESP8266_MQTTCLEAN(void);

#endif /* ESP8266_H */
