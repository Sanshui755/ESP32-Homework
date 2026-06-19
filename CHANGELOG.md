# 更新日志

## [v1.1.0] - 2026-06-17

### 新增功能
- **蜂鸣器温度告警**：当 DHT11 温度超过 32°C 时自动触发蜂鸣器告警，温度恢复正常后自动解除
  - 蜂鸣器引脚：GPIO10（低电平触发）
  - 告警阈值：32°C（定义在 `task_app_mainbusiness.h` 中）
  - 支持日志输出代替硬件蜂鸣器

### 代码重构
- 将 DHT11 传感器驱动代码从 `blink_example_main.c` 迁移至 `task_app_sensor_comm.c`，统一传感器任务管理
- 在 `bsp.h/c` 中新增蜂鸣器控制接口 `bsp_beep_on()` / `bsp_beep_off()`

### 问题修复
- 移除未使用的依赖 `78/uart-uhci`，解决 ESP-IDF v5.5.4 编译错误
- 修复蜂鸣器电平逻辑反了的问题（有源蜂鸣器低电平触发）

---

## [v1.0.0] - 初始版本

### 基础功能
- DHT11 温湿度传感器采集（GPIO4，每 5 秒）
- MICS-5524 气体传感器采集（GPIO2 ADC，每 2 秒）
- BMP280 气压传感器采集（I2C：SDA=GPIO6, SCL=GPIO7，每 2 秒）
- RGB LED 控制（GPIO8，WS2812）
- 按键控制（GPIO9，BOOT 按键切换 LED）
- WiFi 连接管理（自动连接 + 断线重连）
- MQTT 数据上报（JSON 格式，每 3 秒）
- 主业务状态机（INIT → WAIT_CMD → LED_ON/LED_OFF）
- 传感器数据中心（互斥锁保护，线程安全）