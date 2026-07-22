# KS203 + ESP32-P4-WIFI6 SoftAP 可视化

ESP32-P4 通过 RS485（MAX485）读取 KS203 超声距离，开启 WiFi SoftAP，手机浏览器实时显示测距。

## 硬件接线

### KS203（默认 HX-20020-6P）

| KS203 | 接到 |
| --- | --- |
| 红 Pin1 | **5V**（不要用 3.3V 给探头供电） |
| 黑 Pin2 | GND（与 P4 共地） |
| 白 Pin6 RS485-A | MAX485 **A** |
| 棕 Pin5 RS485-B | MAX485 **B** |

### 自动 TTL?RS485 模块 ? ESP32-P4-WIFI6（当前固件）

| 模块侧 | P4 |
| --- | --- |
| **RX**（进模块） | **GPIO27**（UART TX） |
| **TX**（出模块） | **GPIO26**（UART RX） |
| VCC | 3.3V 或 5V（按模块丝印） |
| GND | **必须与 P4 共地** |

**重要：** KS203 若用独立直流电源供电，其 **GND 必须再接到 P4 的 GND**（电源负极 = 转换模块 GND = P4 GND）。只接到电源、不接 P4，串口无法可靠通信。

DE 脚可不接（自动流向）。

### 带 DE 的 MAX485（可选）

| MAX485 | P4 |
| --- | --- |
| DI | GPIO27（UART TX） |
| RO | GPIO26（UART RX） |
| DE+RE | GPIO25 |
| VCC | 3.3V |
| GND | GND（与 P4 共地） |

## 使用

1. 打开 ESP-IDF 终端（或 `D:\esp\export.bat`）
2. 编译烧录：

```bat
cd /d d:\esp32-p4-test\ks203_softap_viz
idf.py set-target esp32p4
idf.py build -- -j1
idf.py -p COMx flash monitor
```

内存紧张时务必加 `-- -j1`。

## 芯片版本

本工程默认按 **ESP32-P4 rev v0.x / v1.x**（pre-v3）编译。  
你的板子若是 `revision v1.3`，请保持现状；若是量产 **v3.0/v3.1**，需要改掉 `CONFIG_ESP32P4_SELECTS_REV_LESS_V3` 再重新 `fullclean` + `build`。

3. 手机连接 WiFi：
   - SSID：`KS203-P4`
   - 密码：`12345678`
4. 浏览器打开：`http://192.168.4.1/`

## 协议说明（本版极简）

- 波特率 115200 8N1，地址 `0xEA`
- 只发：`EA 02 01`
- 收到的原始字节原样显示在网页（`/api/frame`），**不做距离解析**
- 固件引脚：TX=GPIO27，RX=GPIO26

## 配置

`idf.py menuconfig` → **KS203 SoftAP Viz Configuration**：

- UART / DE GPIO
- SoftAP SSID / 密码
- 轮询周期、是否尝试 BCC
