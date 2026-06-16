# 更新日志
所有本项目的重要变更都会记录在此文件中。
本更新日志遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/) 规范，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/) 规范。

---

## [0.2.0] - 2026-06-16

### ESP-IDF 版本升级：v5.5.3 → v6.0.1

#### 1. 组件系统变更（核心技术调整）
- **MQTT 组件**：从 ESP-IDF 核心组件变为托管组件（Component Registry）
  - `main/idf_component.yml` 新增声明：`espressif/mqtt: "^1.0"`
  - 通过 `idf.py component-update` 或 Trae IDE "设置乐鑫设备目标" 触发下载
  - 下载产物位置：`managed_components/espressif__mqtt/`
- **I2C 驱动**：从 `driver` 元组件拆分为独立组件 `esp_driver_i2c`
  - 新头文件路径：`driver/i2c_master.h`（New Master API）
  - `main/CMakeLists.txt` 的 `PRIV_REQUIRES` 需显式声明 `esp_driver_i2c`
  - 原 `driver/i2c.h`（Legacy API）在 v6.0.1 中已标记为弃用
- **GPIO 驱动**：从 `driver` 元组件拆分为独立组件 `esp_driver_gpio`
  - `main/CMakeLists.txt` 的 `PRIV_REQUIRES` 需显式声明 `esp_driver_gpio`
- **顶层构建配置**：`CMakeLists.txt` 设置 `idf_build_set_property(MINIMAL_BUILD ON)`
  - 只编译显式声明的组件，显著加快构建速度
  - 因此 `PRIV_REQUIRES` 必须完整声明所有用到的组件（`esp_netif`、`heap`、`esp_system` 等）
  - 当前 `PRIV_REQUIRES`：`esp_adc esp_driver_i2c esp_driver_gpio esp_wifi esp_event nvs_flash mqtt esp_netif heap esp_system`

#### 2. 构建验证结果
- **编译验证**：✅ 通过（`idf.py build` 成功生成 3 个产物：bootloader.bin、partition-table.bin、IotGateway_prj.bin）
- **应用固件大小**：1,020,178 字节（~996 KB）
- **factory 分区**：1,500 KB，已用 66%，剩余 34%（~503 KB）
- **DRAM 使用**：33.88%（108,850 / 321,296 字节）
- **Flash Code**：786,124 字节
- **Flash Data**：145,224 字节

### 多 ESP-IDF 版本开发环境支持

#### 1. 在 Trae IDE 终端中激活
- **推荐方式**：使用 ESP-IDF 官方 `export.ps1`
  - `. "C:\Esp\v6.0.1\esp-idf\export.ps1"` 一键激活 v6.0.1
  - 自动设置 `IDF_PATH`、工具链路径、Python 虚拟环境
  - 在当前 shell 注册 `idf.py` 函数
- **备选方式**：使用项目内脚本 `activate_idf.ps1`
  - `. .\activate_idf.ps1 v6.0.1` 激活指定版本
  - `. .\activate_idf.ps1 v5.5.3` 切换版本
  - 功能与官方 `export.ps1` 一致，便于在 IDE 内统一管理
- **关键注意**：必须使用 dot-source 语法（开头的 `. ` + 空格），否则 `idf.py` 无法在当前 shell 使用

#### 2. ESP-IDF 各版本 PowerShell 快捷方式
- **新增** `IDF_vX.X.X_Powershell.Ink/` 目录，收集 ESP-IDF 安装管理器自动创建的快捷方式
  - `IDF_v5.5.3_Powershell.lnk` → v5.5.3 开发环境（对应 Python 虚拟环境 `idf5.5_py3.13_env`）
  - `IDF_v5.5.4_Powershell.lnk` → v5.5.4 开发环境
  - `IDF_v6.0.1_Powershell.lnk` → v6.0.1 开发环境（当前项目使用，对应 `idf6.0_py3.14_env`）
  - `ESP-IDF安装管理器安装好每个版本会生成一个对应的快捷方式.md` → 说明文档
  - 双击 `.lnk` 可直接打开已配置好环境变量的 PowerShell 窗口

### 项目文件结构整理

#### 1. 新增 Doc/ 文档目录
- **新增** `Doc/` 目录，统一管理项目文档与图片资源
  - `Doc/images/`：开发板照片、ESP-IDF 安装截图等（`README.md` 的图片路径同步更新：`images/` → `Doc/images/`）
  - `Doc/操作.Prj从idf-v5.5.3移值到idf-v6.0.1.md`：ESP-IDF 版本迁移完整指南（安装管理器、组件变更、Python 环境问题解决）
  - `Doc/操作1 把本地项目推送到一个新建的仓库.md`：Git 推送指南
  - `Doc/V0.0 260609分析架构-DeepSeekV4Pro.md`、`-Doubuo-Seed-2-Code.md`、`-Qwen3.6-Plus.md`：AI 辅助的项目架构分析文档
  - `Doc/task_app_mainbusiness任务中的状态机V0.0说明.md`
  - `Doc/人岗和AI岗会话-备份-260608 11.39.md`

#### 2. 聊天记录备份文件
- **根目录新增** `Agent聊天记录_260613-2255_项目无BUG后清空对话前的备份.md`
- **根目录新增** `Agent聊天记录_260614-1320_项目无BUG后清空对话前的备份.md`
- 保留 AI 开发过程中的关键对话上下文，便于后续回溯与参考

#### 3. ESP-IDF 多版本 Python 环境说明
- 每个 ESP-IDF 版本有独立的 Python 虚拟环境（位于 `%USERPROFILE%\.espressif\python_env\`）
  - `idf6.0_py3.14_env` → v6.0.1 使用（Python 3.14.6，✅ 正常）
  - `idf5.5_py3.13_env` → v5.5.3 / v5.5.4 使用
- 各版本虚拟环境相互独立，**无需也不应该**将 Python 加入系统 PATH
- 激活对应版本时由 `export.ps1` 自动选择正确的 Python 环境

### README 文档完善
- **更新** 项目顶部信息：添加当前版本号 0.2.0、ESP-IDF v6.0.1、编译状态、Flash 使用摘要
- **新增** "项目结构" 章节：树形展示 `main/`、`managed_components/`、`Doc/`、`IDF_vX.X.X_Powershell.Ink/` 等目录
- **更新** "ESP-IDF 开发环境说明" 章节：区分官方 `export.ps1` 与项目内 `activate_idf.ps1` 两种激活方式，说明 Python 虚拟环境独立机制
- **新增** "编译、烧录与监控" 章节：包含完整操作流程、Flash/RAM 使用数据、常用命令速查表
- **更新** "ESP-IDF v5.5.3 → v6.0.1 迁移要点"：添加 Python 环境对比、`main/idf_component.yml`、`main/CMakeLists.txt`、`CMakeLists.txt`、`sdkconfig` 等关键配置文件的实际内容对照
- **修复** 图片路径：`images/` → `Doc/images/`（图片资源已迁移到 `Doc/images/` 目录）

---

## [0.1.0] - 2026-06-14

### 核心架构重构

#### 1. 主入口文件重构
- **删除** `main/blink_example_main.c`（原文件将所有逻辑混在一起）
- **新增** `main/app_main.c`：清晰的分层启动顺序
  - 第一步：NVS Flash 初始化 + ESP Event Loop 创建
  - 第二步：BSP 硬件初始化（LED + I2C 总线）
  - 第三步：任务监控启动
  - 第四步：WiFi / MQTT / Command Handler / Sensor / Button 各任务依次启动
- 避免了旧版本中将 DHT11 手动 GPIO 操作、按钮轮询等所有逻辑混在一个文件中的问题

#### 2. 传感器任务统一
- **删除** `main/task_app_sensor_comm.c`
- **删除** 原 `blink_example_main.c` 中的 DHT11 / MICS5524 内联实现
- **新增** `main/task_app_sensor.c`：统一的传感器任务
  - **DHT11**：改用 `esp-idf-lib/dht` 组件库（替代手动 GPIO 位操作 + 禁用中断的实现，更加可靠）
  - **MICS-5524**：ADC 单次采样，统一周期
  - **ICM-20948**（新增）：9轴 IMU，加速度计 + 陀螺仪 + 磁力计 AK09916（I2C BYPASS 模式）
  - 5 秒统一采集周期，任务看门狗保护
  - `app_mqtt_notify_new_data()` 通知 MQTT 任务发布（解耦采集与传输）
  - 每个传感器独立的失败重试 / 降级机制
  - ICM-20948 I2C 通信失败时，磁力计可通过 BYPASS 路径独立读取

#### 3. 按键任务重写
- **删除** 原 `blink_example_main.c` 中简单的轮询实现（每 10ms 读取一次 GPIO）
- **新增** `main/task_app_btn.c`：ISR 驱动的按键处理
  - GPIO 上升沿中断触发
  - FreeRTOS Queue 将事件从 ISR 传递到任务层
  - 50ms 软件消抖
  - 按键释放后切换 RGB LED（金色）
  - 注册到任务监控

#### 4. 命令处理任务
- **删除** `main/task_app_mainbusiness.c`
- **新增** `main/task_app_cmd.c`：命令处理状态机
  - 显式状态机：`WAIT_CMD → LED_ON / LED_OFF`
  - 命令队列（Queue，容量 10 条）
  - 提供 `app_send_command()` / `app_send_command_nonblock()` API
  - 与 MQTT 完全解耦：MQTT 只发送命令字符串，不直接操作 LED

#### 5. 任务监控系统
- **新增** `main/task_monitor.c` 和 `main/task_monitor.h`
  - 任务注册机制：`task_monitor_register(name, handle, stack_total)`
  - 每 30 秒输出报告
  - 报告内容：自由堆 / 最小堆 / 每个任务的栈总大小、最小剩余、使用百分比
  - 正确处理 Word ↔ Byte 的栈单位转换
  - WiFi / MQTT / Sensor / Button / Cmd 任务均已注册

#### 6. MQTT 任务增强
- `main/task_app_mqtt.c` 新增功能：
  - MQTT v5 协议
  - 基于 MAC 地址生成唯一 Client ID（`qzm_iotgw_<MAC>`）
  - LWT（遗嘱消息）：设备离线时自动发布 `{"status":"offline"}`
  - 设备上线时发布 `{"status":"online"}`（Retain）
  - JSON 数据缓存：重连后自动补发上次传感器数据
  - 通过 `TaskNotify` 与传感器任务解耦（替代旧版本的耦合调用）
  - 详细的事件日志（PUB / SUB / DATA / ERROR）
  - QoS 1 保证至少一次传输
  - LED 控制命令：`'1'` 开灯、`'0'` 关灯（非阻塞发送到命令队列）

#### 7. WiFi 管理任务增强
- `main/task_app_wifi_manage.c` 新增功能：
  - 指数退避重连机制（500ms ~ 30s，最大 6 次位移）
  - FreeRTOS EventGroup 通知连接状态
  - WiFi 状态查询 API：`app_wifi_is_connected()` / `app_wifi_wait_for_connection()`
  - 注册到任务监控
  - 详细的连接状态日志

#### 8. BSP 层增强
- `main/bsp.c` 新增功能：
  - **I2C Master 总线初始化**：创建共享总线（SDA=GPIO6, SCL=GPIO7, 100kHz）
  - `bsp_i2c_get_bus()`：提供 I2C 总线句柄给其他模块（ICM-20948 等）
  - LED 颜色常量：`BSP_LED_COLOR_RED` / `BSP_LED_COLOR_GOLD` / `BSP_LED_COLOR_GREEN` / `BSP_LED_COLOR_BLUE`
  - `bsp_led_toggle(red, green, blue)`：支持指定颜色的切换
  - 多种 LED 颜色开关 API：`bsp_led_on_red/gold/green/blue`

#### 9. 传感器数据管理增强
- `main/sensor_data_mgr.c` 新增功能：
  - **ICM-20948 数据字段**：加速度（ax/ay/az）、陀螺仪（gx/gy/gz）、磁力计（mx/my/mz）、IMU 温度
  - `sensor_data_mgr_update_imu()`：完整 IMU 数据更新
  - `sensor_data_mgr_update_mag_only()`：仅磁力计更新（ICM 失败降级时使用）
  - JSON 输出扩展：包含 IMU 子对象，仅输出有效且未过期的数据（30 秒过期）
  - 每个数据字段独立的时间戳 + 有效标志位

#### 10. 鲁棒性增强（全面提升）
- **任务看门狗（Task WDT）**：WiFi / MQTT / Sensor 任务均注册并周期性喂狗
- **传感器失败重试**：DHT11 3 次重试、ICM-20948 3 次重试
- **磁力计自动恢复**：BYPASS 模式丢失时自动重新启用
- **MQTT 缓存补发**：断线重连后自动补发上次传感器数据
- **数据过期标记**：30 秒未更新的数据不输出到 JSON
- **非阻塞命令发送**：`app_send_command_nonblock()` 避免 MQTT 事件回调阻塞

#### 11. ESP Component Registry 组件使用
- **新增** `managed_components/esp-idf-lib__dht/`：DHT11 驱动（替代手动 GPIO 位操作）
- **新增** `managed_components/cybergear-robotics__icm20948/`：ICM-20948 驱动
- **继续使用** `managed_components/espressif__led_strip/`：RGB LED 驱动

#### 12. 文件结构变更
| 操作 | 文件 | 说明 |
|------|------|------|
| 新增 | `main/app_main.c` | 清晰分层的系统启动入口 |
| 新增 | `main/task_app_btn.c` | ISR 驱动的按键任务 |
| 新增 | `main/task_app_btn.h` | 按键任务头文件 |
| 新增 | `main/task_app_cmd.c` | 命令处理状态机任务 |
| 新增 | `main/task_app_cmd.h` | 命令任务头文件 |
| 新增 | `main/task_app_sensor.c` | 统一的传感器采集任务（含 ICM-20948） |
| 新增 | `main/task_app_sensor.h` | 传感器任务头文件 |
| 新增 | `main/task_monitor.c` | 任务栈/堆内存监控 |
| 新增 | `main/task_monitor.h` | 任务监控头文件 |
| 删除 | `main/blink_example_main.c` | 旧版混合逻辑的主文件 |
| 删除 | `main/task_app_mainbusiness.c` | 旧版主业务状态机 |
| 删除 | `main/task_app_sensor_comm.c` | 旧版传感器任务 |
| 删除 | `main/task_app_sensor_icm20948.c` | 旧版独立的 ICM-20948 任务（已合并到 sensor 任务） |
| 删除 | `main/task_app_sensor_icm20948.h` | 旧版独立的 ICM-20948 头文件 |

---

## [0.0.1] - 2026-06-12

### 优化 DHT11 传感器数据采集与架构初步优化
- DHT11 读取逻辑改进
- 初步的架构分层设计

---

## [0.0.0] - 2026-06-09

### 初始版本发布
- 核心功能基本完成
  - RTOS 的任务分配类似于微服务架构，将功能模块拆分成多个独立的服务，每个服务负责特定的功能
  - 硬编码连接指定 WiFi 路由（已实现手机一键配网功能，但考虑到学生学习环境，删除了一键配网，只保留直接连接 WiFi 路由的功能）
  - 实现了 MQTT 客户端连接公共 EMQX Broker 服务器
  - 定时把智能设备的数据发布到 MQTT 服务器
  - 能接收 MQTT 主题并执行开、关灯操作
  - 板载按键封装到一个任务中，用于处理按键事件
  - 板载 RGB LED 有效，可以控制 RGB 灯颜色
  - 实现了多种传感器的数据采集，并把数据发布到 MQTT 服务器（DHT11、MICS-5524、BMP280）
