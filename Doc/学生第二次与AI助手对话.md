# 学生第二次与 AI 助手对话记录

> 日期：2026-06-17
> 项目：ESP32 物联网智能网关 (TP_IotGateway-master)
> 主题：添加蜂鸣器温度告警功能

---

## 一、任务目标

在现有项目中添加蜂鸣器告警功能：
1. 在 `bsp.h` 中定义蜂鸣器引脚
2. 在 `bsp.c` 中实现 `bsp_beep_on()` / `bsp_beep_off()` 函数
3. 在主业务逻辑中添加告警判断：当温度 > 阈值时，触发蜂鸣器 + 上报告警

---

## 二、修改计划

### 涉及文件

| 文件 | 改动 |
|------|------|
| `bsp.h` | 新增蜂鸣器引脚定义 + 2 个函数声明 |
| `bsp.c` | 新增蜂鸣器初始化 + `beep_on/off` 实现 |
| `task_app_mainbusiness.h` | 新增告警阈值宏定义 |
| `task_app_mainbusiness.c` | 新增温度告警判断逻辑 |

### 引脚选择

选用 GPIO10 作为蜂鸣器引脚，原因：GPIO2/4/6/7/8/9 已被其他外设占用，GPIO10 空闲可用。

### 告警阈值

温度告警阈值设为 **32°C**，超过即触发告警。

---

## 三、具体修改

### 3.1 bsp.h — 新增蜂鸣器接口

```c
// 蜂鸣器引脚定义
#define BSP_BUZZER_GPIO 10

// 蜂鸣器控制（无硬件时用日志输出代替）
void bsp_beep_on(void);
void bsp_beep_off(void);
```

### 3.2 bsp.c — 实现蜂鸣器控制

新增 `#include "driver/gpio.h"`。

在 `bsp_init()` 中追加蜂鸣器 GPIO 初始化：

```c
// 初始化蜂鸣器 GPIO
gpio_reset_pin(BSP_BUZZER_GPIO);
gpio_set_direction(BSP_BUZZER_GPIO, GPIO_MODE_OUTPUT);
gpio_set_level(BSP_BUZZER_GPIO, 1);   // 默认高电平，蜂鸣器不响
```

新增两个函数（GPIO 控制 + 日志输出双保险）：

```c
void bsp_beep_on(void)
{
    gpio_set_level(BSP_BUZZER_GPIO, 0);   // 低电平触发（有源蜂鸣器）
    ESP_LOGW(BSP_TAG, "BEEP ON  [警报]");
}

void bsp_beep_off(void)
{
    gpio_set_level(BSP_BUZZER_GPIO, 1);   // 高电平关闭
    ESP_LOGI(BSP_TAG, "BEEP OFF [警报解除]");
}
```

### 3.3 task_app_mainbusiness.h — 新增阈值

```c
// 温度告警阈值
#define ALARM_TEMP_THRESHOLD  32.0f
```

### 3.4 task_app_mainbusiness.c — 新增告警逻辑

在 `process_state_machine()` 中，`update_sensor_data()` 之后加入告警判断：

```c
// 温度告警检查
static bool alarm_active = false;

if (s_sensor_data.temperature_valid
    && s_sensor_data.temperature > ALARM_TEMP_THRESHOLD) {
    if (!alarm_active) {
        bsp_beep_on();
        ESP_LOGW(APP_TAG, "高温告警！当前温度: %.1f°C (阈值: %.1f°C)",
                 s_sensor_data.temperature, ALARM_TEMP_THRESHOLD);
        alarm_active = true;
    }
} else {
    if (alarm_active) {
        bsp_beep_off();
        ESP_LOGI(APP_TAG, "告警解除，温度恢复正常: %.1f°C",
                 s_sensor_data.temperature);
        alarm_active = false;
    }
}
```

---

## 四、告警逻辑流程

```
每 10ms 循环一次：
    ↓
读取传感器数据
    ↓
温度 > 32°C 且 之前没告警？
    ├── 是 → bsp_beep_on() → 日志 "BEEP ON [警报]"
    │        日志 "高温告警！当前温度: 33°C"
    │        标记 alarm_active = true
    │
    └── 否 → 温度正常 且 之前有告警？
              └── 是 → bsp_beep_off() → 日志 "BEEP OFF [警报解除]"
                        标记 alarm_active = false
```

---

## 五、遇到问题：蜂鸣器电平反了

### 现象

还没到温度阈值（32°C），蜂鸣器就一直响。

### 原因

蜂鸣器是**有源蜂鸣器（低电平触发）**，但代码初始化和控制逻辑写成了高电平触发：

| 函数 | 错误写法 | 正确写法 | 说明 |
|------|---------|---------|------|
| `bsp_init()` | GPIO=0 | GPIO=1 | 上电默认不响 |
| `bsp_beep_on()` | GPIO=1 | GPIO=0 | 低电平触发鸣响 |
| `bsp_beep_off()` | GPIO=0 | GPIO=1 | 高电平关闭 |

### 解决方案

将 `bsp_beep_on()` 和 `bsp_beep_off()` 中的 GPIO 电平互换，同时将 `bsp_init()` 中默认电平改为高电平。

---

## 六、监视器预期效果

```
W BSP: BEEP ON  [警报]
W APP_MAIN: 高温告警！当前温度: 33.0°C (阈值: 32.0°C)
...（温度降到 32°C 以下后）...
I BSP: BEEP OFF [警报解除]
I APP_MAIN: 告警解除，温度恢复正常: 31.0°C
```

---

## 七、经验总结

1. **有源蜂鸣器 vs 无源蜂鸣器**：有源蜂鸣器内部自带振荡电路，通电即响；无源蜂鸣器需要 PWM 信号驱动
2. **电平触发方式**：有源蜂鸣器通常是低电平触发（NPN 三极管驱动），代码中初始状态应设为高电平（不响）
3. **告警防抖**：使用 `static bool alarm_active` 避免重复触发，只在状态变化时执行动作
4. **日志双保险**：即使没有硬件蜂鸣器，也能通过日志输出看到告警信息
5. **阈值设计**：温度阈值应略高于正常环境温度，避免频繁触发/解除（本实验设为 32°C）