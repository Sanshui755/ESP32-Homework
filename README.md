# 项目介绍
- 《物联网智能网关设计与开发》课程的教学项目（TP），偏智能设备设计与开发。
- 版本变化请查看[更新日志](CHANGELOG.md)

# 项目结构
- 以ESP-IDF的例子“get-started/blink”作为模板搭建
- 项目以CBSE为指导，优先使用ESP Component Registry中已有的组件，避免重复造轮子。
  - 需要添加组件，可以先在ESP Component Registry中搜索，然后添加到项目根目录下的.idf_component.yml文件中。重新编译项目即可。esp-idf框架会自动下载并编译组件。

# 项目使用的开发板
- ESP32-C3-DevKitM-1（当前版本使用） 
- ESP32-S3-DevKitM-1（后面会添加）
  
# ESP32-C3-DevKitM-1开发板的引脚分配
  RGB灯连接引脚：GPIO8
  Boot按键连接引脚：GPIO9
  DHT11传感器连接引脚：GPIO4
  MICS-5524气体传感器连接引脚：GPIO2 (ADC1_CHANNEL_0)
  BMP280传感器连接引脚：I2C SDA=GPIO6, SCL=GPIO7
  蜂鸣器连接引脚：GPIO10 (低电平触发)

# 智能设备已实现的功能
- **传感器数据采集**
  - DHT11温湿度传感器：每5秒读取一次温度和湿度
  - MICS-5524气体传感器：每2秒读取一次ADC值
  - BMP280气压传感器：每2秒读取一次温度和气压（I2C通信）
- **传感器数据管理**
  - 统一的数据中心(`sensor_data_mgr`)，带互斥锁保护，支持多任务并发访问
  - JSON格式化输出，支撑MQTT上报
- **RGB LED 控制**
  - 支持地址可编程LED（WS2812等），通过RMT或SPI驱动
  - 按键(BOOT/GPIO9)按下松开时，LED切换开/关状态
  - 任务监控指示：系统空闲时LED显示绿色/黄色
  - BSP层(`bsp.c/h`)提供LED统一操作接口
- **WiFi 连接管理**
  - 自动连接WiFi热点
  - 断线自动重连
  - 状态机跟踪连接状态（DISCONNECTED/CONNECTING/CONNECTED/ERROR）
- **MQTT 通信**
  - 连接公共MQTT Broker，上报传感器数据(JSON格式，每3秒)
  - 订阅灯控Topic，支持远程APP控制LED开关
- **主业务状态机**
  - 基于显式状态机模式（INIT → WAIT_CMD → LED_ON/LED_OFF）
  - 通过FreeRTOS Queue接收命令，解耦MQTT与业务逻辑
- **温度告警**
  - DHT11温度超过32°C时自动触发蜂鸣器告警
  - 温度恢复正常后自动解除告警
  - 支持日志输出代替硬件蜂鸣器