#include "mqtt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "esp8266.h"
#include "timebase.h"

static char s_payload[384];
static char s_escaped[448];
static char s_command[576];

/* AT 命令中的双引号和反斜杠需要再转义一层。 */
static void escape_at_string(const char *source, char *destination, uint16_t capacity)
{
    uint16_t out;

    out = 0U;
    while ((*source != '\0') && ((out + 2U) < capacity))
    {
        if ((*source == '"') || (*source == '\\'))
        {
            destination[out++] = '\\';
        }
        destination[out++] = *source++;
    }
    destination[out] = '\0';
}

bool MQTT_Connect(void)
{
    ESP8266_Init(APP_ESP8266_BAUDRATE);
    if (!ESP8266_AT_Test())
    {
        return false;
    }

    (void)ESP8266_Enable_ATE(false);
    if (!ESP8266_Net_Mode_Choose(ESP8266_STA))
    {
        return false;
    }
    if (!ESP8266_JoinAP(APP_WIFI_SSID, APP_WIFI_PASSWORD))
    {
        return false;
    }
    if (!ESP8266_MQTTUSERCFG(APP_MQTT_CLIENT_ID,
                             APP_MQTT_USERNAME,
                             APP_MQTT_PASSWORD))
    {
        return false;
    }
    if (!ESP8266_MQTTCONN(APP_MQTT_HOST, (uint16_t)APP_MQTT_PORT))
    {
        return false;
    }
#if APP_MQTT_ENABLE_SUBSCRIBE
    if (!ESP8266_MQTTSUB(APP_MQTT_TOPIC_PROPERTY_SET))
    {
        return false;
    }
#endif
    return true;
}

bool MQTT_PublishHealth(const HealthData_t *data)
{
    int pitch_x100;
    int roll_x100;
    int yaw_x100;

    if (data == 0)
    {
        return false;
    }

    pitch_x100 = (int)(data->pitch_deg * 100.0f);
    roll_x100 = (int)(data->roll_deg * 100.0f);
    yaw_x100 = (int)(data->yaw_deg * 100.0f);

    /*
     * JSON 全部使用 ASCII 标点，避免原工程中的全角逗号使云端解析失败。
     * 字段名采用阿里云物模型属性名，同时保留 valid 标志。
     */
    snprintf(s_payload, sizeof(s_payload),
             "{\"id\":\"%lu\",\"version\":\"1.0\",\"params\":{"
             "\"heartRate\":%ld,\"heartRateValid\":%u,"
             "\"spo2\":%ld,\"spo2Valid\":%u,"
             "\"pressureAdc\":%u,\"pressureMv\":%u,"
             "\"pitchX100\":%d,\"rollX100\":%d,\"yawX100\":%d,"
             "\"temperatureCentiC\":%d,\"digitalInput\":%u,"
             "\"alarm\":%u,\"powerSwitch\":%u}}",
             (unsigned long)data->timestamp_ms,
             (long)data->heart_rate_bpm,
             (unsigned int)data->heart_rate_valid,
             (long)data->spo2_percent,
             (unsigned int)data->spo2_valid,
             (unsigned int)data->pressure_adc,
             (unsigned int)data->pressure_mv,
             pitch_x100,
             roll_x100,
             yaw_x100,
             (int)data->temperature_centi_c,
             (unsigned int)data->digital_input,
             (unsigned int)data->alarm_active,
             (unsigned int)data->remote_output);

    escape_at_string(s_payload, s_escaped, sizeof(s_escaped));
    snprintf(s_command, sizeof(s_command),
             "AT+MQTTPUB=0,\"%s\",\"%s\",1,0",
             APP_MQTT_TOPIC_PROPERTY_POST,
             s_escaped);

    if (strlen(s_command) >= (sizeof(s_command) - 1U))
    {
        return false;
    }
    ESP8266_WriteLine(s_command);
    return true;
}

int MQTT_PollPowerSwitch(void)
{
    static uint32_t last_poll_ms = 0U;
    const char *message;
    const char *cursor;
    int value;

    if (!Timebase_Elapsed(last_poll_ms, APP_MQTT_POLL_INTERVAL_MS))
    {
        return -1;
    }
    last_poll_ms = Timebase_GetMillis();

    if (!g_esp8266_rx.complete)
    {
        return -1;
    }

    message = ESP8266_GetRxBuffer();
    value = -1;
    cursor = strstr(message, "PowerSwitch");
    if (cursor != 0)
    {
        cursor = strchr(cursor, ':');
        if (cursor != 0)
        {
            cursor++;
            while ((*cursor == ' ') || (*cursor == '\t') || (*cursor == '"'))
            {
                cursor++;
            }
            if ((*cursor == '0') || (*cursor == '1'))
            {
                value = *cursor - '0';
            }
        }
    }

    ESP8266_ClearRx();
    return value;
}
