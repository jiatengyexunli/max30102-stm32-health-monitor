#ifndef MQTT_H
#define MQTT_H

#include <stdbool.h>
#include <stdint.h>

#include "health_monitor.h"

/*
 * 初始化 ESP8266、连接 Wi-Fi、配置 MQTT 客户端并订阅属性设置主题。
 * 全部步骤都有超时；返回 false 后本地采集仍可继续。
 */
bool MQTT_Connect(void);

/*
 * 异步发送一次健康数据。函数只把命令写入 USART2，不等待服务器。
 * 若启用 MQTT，最终与阿里云物模型字段对齐。
 */
bool MQTT_PublishHealth(const HealthData_t *data);

/*
 * 解析云端 WebSocket/物模型设置消息中的 PowerSwitch。
 * 返回 0/1 表示收到命令；-1 表示当前没有新命令。
 */
int MQTT_PollPowerSwitch(void);

#endif /* MQTT_H */
