# IoT Gateway 项目

《物联网智能网关设计与开发》课程的教学项目（TP），基于 ESP32-C3 实现的多传感器智能网关设备。

版本变化请查看[更新日志](CHANGELOG.md)

---

## 开发板

- **ESP32-C3-DevKitM-1（当前版本使用）
- ESP32-S3-DevKitM-1（后续版本会添加）

### 引脚分配

| 功能 | 引脚 |
|------|------|
| RGB LED (WS2812) | GPIO8 |
| Boot 按键 | GPIO9 |
| DHT11 温湿度 | GPIO4 |
| MICS-5524 气体 | GPIO2 (ADC1_CHANNEL_0) |
| I2C SDA | GPIO6 |
| I2C SCL | GPIO7 |

---

## 整体架构

项目采用 **FreeRTOS 多任务 + 分层架构**，每个功能模块拆分为独立的任务，通过队列/通知机制解耦。

```
┌─────────────────────────────────────────────────────────┐
│                      app_main.c                        │
│  (系统初始化入口：NVS → BSP → 启动各任务        │
└────────────────────┬────────────────────────────────────┘
                     │
     ┌───────────────┼─────────────────────────────────┐
     │               │                                 │
┌────▼────┐   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
│ WiFi    │   │  MQTT   │   │  Sensor  │   │  Button  │
│Manager  │   │  Client │   │  Task    │   │   Task  │
│task_app_wifi_manage │ task_app_mqtt │ task_app_sensor │ task_app_btn │
└────┬────┘   └────┬────┘   └────┬────┘   └────┬────┘
     │               │               │               │
     │               │               │               │
     │               │               │               │
     │               │               │               │
     │           ┌─▼────────────┐                │
     │           │  Cmd Handler │                │
     │           │ task_app_cmd │                │
     │           └────┬────────┘                │
     │                │                         │
     │                ▼                         │
     │         ┌──────────────┐                 │
     │         │  BSP     │                 │
     │         │ (LED, │                 │
     │         │ I2C Bus │                 │
     │         └────┬──────┘                 │
     │              │                         │
     │              │                         │
     │         ┌──▼──────────────┐             │
     │         │  Sensor Data   │             │
     │         │  Manager       │             │
     │         │  (互斥锁保护)      │             │
     │         └────┬──────────┘             │
     │              │                         │
     │              ▼                         │
     │         ┌──────────────┐             │
     │         │ Task Monitor │             │
     │         │ (栈使用率/│             │
     │         │ heap监控)    │             │
     │         └──────────────┘             │
     │                                       │
     │                                       │
     └─────────────────────────────────────────┘
```

---

## 任务模块详解

### 1. WiFi 管理任务 (`task_app_wifi_manage.c`)

- **功能**：管理 WiFi 连接状态机，支持断线自动重连
- **状态机**：`DISCONNECTED → CONNECTING → CONNECTED → ERROR`
- **特性**：
  - 通过 menuconfig 配置 SSID / 密码
  - 指数退避重连机制（500ms ~ 30s）
  - FreeRTOS EventGroup 通知连接状态
  - WiFi 状态查询 API：`app_wifi_is_connected()` / `app_wifi_wait_for_connection()`
  - 注册到任务监控

### 2. MQTT 客户端任务 (`task_app_mqtt.c`)

- **功能**：连接公共 MQTT Broker，上报传感器数据，订阅 LED 控制命令
- **Broker**：`mqtt://broker.emqx.io:1883`
- **Topic**：
  - `qzm/iotgw/sensor/data` → 传感器数据发布（JSON）
  - `qzm/iotgw/led/control` → LED 控制订阅（`1`=开，`0`=关）
  - `qzm/iotgw/status` → 设备在线状态（LWT 遗嘱消息）
- **特性**：
  - MQTT v5 协议，Keepalive 30s
  - 基于 MAC 地址生成唯一 Client ID
  - 断线自动重连（2s 超时）
  - 缓存上次 JSON，重连后自动补发
  - 通过 `TaskNotify` 与传感器任务解耦
  - QoS 1 保证至少一次传输

### 3. 传感器统一任务 (`task_app_sensor.c`)

- **功能**：统一管理所有传感器数据采集，5秒周期轮询
- **支持的传感器**：
  - **DHT11**：温湿度（使用 `esp-idf-lib/dht` 组件库，避免手动操作 GPIO 禁用中断）
  - **MICS-5524**：气体传感器（ADC 单次采样
  - **ICM-20948**：9轴 IMU（加速度计 + 陀螺仪 + 磁力计 AK09916，使用 `cybergear-robotics/icm20948` 组件库
- **特性**：
  - 每个传感器独立失败重试机制
  - ICM-20948 I2C BYPASS 模式支持磁力计
  - 磁力计自动恢复机制
  - 通过 `sensor_data_mgr` 更新数据后通知 MQTT 任务
  - 任务看门狗（Task WDT）保护
  - 统一 JSON 格式化输出

### 4. 按键任务 (`task_app_btn.c`)

- **功能**：处理 Boot 按键事件
- **特性**：
  - GPIO ISR 中断驱动（上升沿触发）
  - FreeRTOS Queue 传递事件到任务层（避免在 ISR 中做复杂操作）
  - 50ms 软件消抖
  - 按键释放后切换 RGB LED（金色）

### 5. 命令处理任务 (`task_app_cmd.c`)

- **功能**：处理来自 MQTT 的命令，基于显式状态机
- **状态机**：`WAIT_CMD → LED_ON / LED_OFF`
- **特性**：
  - 命令队列（Queue，容量 10 条
  - 阻塞式 API：`app_send_command()` / `app_send_command_nonblock()`
  - 支持的命令：`LED_ON`、`LED_OFF`
  - 与 MQTT 解耦：MQTT 只发送命令字符串→ 状态机负责执行

### 6. 任务监控 (`task_monitor.c`)

- **功能**：周期性监控各任务栈使用率与系统堆内存
- **特性**：
  - 每 30 秒输出一次报告
  - 任务注册机制：`task_monitor_register(name, handle, stack_total)
  - 报告内容：自由堆 / 最小堆 / 每个任务的栈总大小、最小剩余、使用百分比
  - 栈单位转换说明（Word ↔ Byte）

---

## BSP 层 (`bsp.c`)

- **功能**：板级支持包，抽象硬件细节
- **内容**：
  - RGB LED 控制（WS2812，通过 `led_strip` 组件）
  - I2C Master 总线初始化与设备管理（共享总线
  - LED 颜色常量：红、金、绿、蓝
  - 统一 API：`bsp_led_on/off/toggle/set_color`
  - `bsp_i2c_get_bus()` 获取 I2C 总线句柄

---

## 传感器数据管理 (`sensor_data_mgr.c`)

- **功能**：统一数据中心，线程安全的传感器数据存储与 JSON 序列化
- **数据结构**：
  - 温度、湿度、气体 ADC 值
  - IMU（加速度、陀螺仪、磁力计、温度
  - 每个字段独立有效标志位 + 时间戳
- **特性**：
  - FreeRTOS Mutex 互斥锁保护，支持多任务并发读写
  - `sensor_data_mgr_update_*() 系列更新 API
  - `sensor_data_mgr_get_json()` 生成 JSON 字符串
  - 30 秒过期机制：超过 30 秒未更新的数据视为无效，不输出到 JSON

---

## 架构设计要点

### 1. 任务解耦
- 每个功能模块独立任务，职责单一
- 任务间通过 FreeRTOS Queue / TaskNotify / EventGroup 通信
- 避免全局变量直接访问，统一通过模块 API

### 2. 分层设计
- **BSP 层**：硬件抽象（LED、I2C）
- **数据管理层**：传感器数据集中管理
- **业务逻辑层**：状态机、命令处理
- **通信层**：WiFi、MQTT

### 3. 组件优先使用 ESP Component Registry 中的成熟组件：
- `esp-idf-lib/dht` → DHT11 驱动
- `cybergear-robotics/icm20948` → ICM-20948 驱动
- `espressif/led_strip` → RGB LED 驱动
避免重复造轮子

### 4. 鲁棒性设计
- 任务看门狗（Task WDT）
- WiFi 指数退避重连
- MQTT 断线自动重连 + 数据缓存补发
- 传感器读取失败重试
- 磁力计 BYPASS 模式自动恢复
- 数据过期自动标记

---

## 功能清单

- ✅ DHT11 温湿度采集（5s 周期）
- ✅ MICS-5524 气体采集（5s 周期）
- ✅ ICM-20948 9轴 IMU 采集（5s 周期）
- ✅ 传感器数据 JSON 格式化
- ✅ RGB LED 控制（远程 + 本地按键）
- ✅ WiFi 自动连接 + 断线重连
- ✅ MQTT 数据上报（JSON）
- ✅ MQTT 远程 LED 控制
- ✅ 命令状态机
- ✅ 任务栈 / 堆内存监控
- ✅ 任务看门狗保护
