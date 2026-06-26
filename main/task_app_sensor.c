#include "task_app_sensor.h"
#include "task_app_mqtt.h"
#include "sensor_data_mgr.h"
#include "bsp.h"
#include "task_monitor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <string.h>
#include <stdio.h>

// ── DHT11 ──
#include "dht.h"
#define DHT11_GPIO        GPIO_NUM_4
#define DHT_MAX_FAIL       3

// ── MICS5524 ──
#include "esp_adc/adc_oneshot.h"
#define MICS5524_ADC_CH   ADC_CHANNEL_0

// ── ICM-20948 ──
#include "driver/i2c_master.h"
#include "icm20948.h"
#define ICM_I2C_ADDR       0x68
#define I2C_CLK_SPEED      100000
#define MAG_I2C_ADDR       0x0C
#define MAG_REG_WIA        0x01
#define MAG_REG_ST1        0x10
#define MAG_REG_HXL        0x11
#define MAG_REG_CNTL2      0x31
#define MAG_REG_CNTL3      0x32

static const char *TAG = "SENSOR";

// ── Per-sensor state ──
static int  s_dht_fail = 0;
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

// ICM-20948 state
static i2c_master_dev_handle_t s_icm_handle = NULL;
static i2c_master_dev_handle_t s_mag_handle = NULL;
static i2c_master_bus_handle_t  s_bus_handle = NULL;
static bool s_imu_present = false;
static icm20948_device_t s_icm_device;   // must outlive init → stored statically

// ==================================================================
//  ICM-20948 — verbatim from task_app_sensor_icm20948.c (DO NOT EDIT)
// ==================================================================

static icm20948_status_e icm_i2c_write(uint8_t reg, uint8_t *data, uint32_t len, void *user)
{
    (void)user;
    uint8_t buf[64];
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    return (i2c_master_transmit(s_icm_handle, buf, len + 1, pdMS_TO_TICKS(100)) == ESP_OK)
        ? ICM_20948_STAT_OK : ICM_20948_STAT_ERR;
}

static icm20948_status_e icm_i2c_read(uint8_t reg, uint8_t *buff, uint32_t len, void *user)
{
    (void)user;
    return (i2c_master_transmit_receive(s_icm_handle, &reg, 1, buff, len, pdMS_TO_TICKS(100)) == ESP_OK)
        ? ICM_20948_STAT_OK : ICM_20948_STAT_ERR;
}

static esp_err_t mag_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_mag_handle, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t mag_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_mag_handle, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

// Direct ICM register access (bypasses library's bank caching).
// Writes REG_BANK_SEL first, then the target register.
static esp_err_t icm_direct_write_reg(uint8_t bank, uint8_t reg, uint8_t val)
{
    uint8_t bank_byte = (bank << 4) & 0x30;
    uint8_t buf[2] = {0x7F, bank_byte};  // REG_BANK_SEL = 0x7F
    if (i2c_master_transmit(s_icm_handle, buf, 2, pdMS_TO_TICKS(100)) != ESP_OK)
        return ESP_FAIL;
    buf[0] = reg;
    buf[1] = val;
    return i2c_master_transmit(s_icm_handle, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t icm_direct_read_reg(uint8_t bank, uint8_t reg, uint8_t *val)
{
    uint8_t bank_byte = (bank << 4) & 0x30;
    uint8_t buf[2] = {0x7F, bank_byte};
    if (i2c_master_transmit(s_icm_handle, buf, 2, pdMS_TO_TICKS(100)) != ESP_OK)
        return ESP_FAIL;
    return i2c_master_transmit_receive(s_icm_handle, &reg, 1, val, 1, pdMS_TO_TICKS(100));
}

// Enable I2C BYPASS with read-back verification & retries.
// Directly drives ICM registers — avoids library's bank caching issues.
static bool mag_enable_bypass(void)
{
    uint8_t int_pin_cfg;

    for (int attempt = 1; attempt <= 5; attempt++) {

        // Step 1: select bank 0
        if (icm_direct_write_reg(0, 0x0F, 0x02) != ESP_OK) {
            ESP_LOGW(TAG, "  bypass[%d]: can't write bank+reg (step1)", attempt);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(2));

        // Step 2: read current INT_PIN_CFG (bank 0, reg 0x0F)
        if (icm_direct_read_reg(0, 0x0F, &int_pin_cfg) != ESP_OK) {
            ESP_LOGW(TAG, "  bypass[%d]: can't read INT_PIN_CFG", attempt);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // Step 3: set BYPASS_EN (bit 1), preserve other bits
        int_pin_cfg |= 0x02;
        if (icm_direct_write_reg(0, 0x0F, int_pin_cfg) != ESP_OK) {
            ESP_LOGW(TAG, "  bypass[%d]: can't write BYPASS_EN", attempt);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(5));

        // Step 4: READ BACK to verify
        uint8_t verify = 0;
        if (icm_direct_read_reg(0, 0x0F, &verify) != ESP_OK) {
            ESP_LOGW(TAG, "  bypass[%d]: can't read-back (verify)", attempt);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (verify & 0x02) {
            ESP_LOGI(TAG, "  bypass[%d]: OK (INT_PIN_CFG=0x%02X)", attempt, verify);
            return true;
        }
        ESP_LOGW(TAG, "  bypass[%d]: read-back=0x%02X (bit1 not set), retry...", attempt, verify);
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGE(TAG, "  bypass: FAILED after 5 attempts");
    return false;
}

static bool mag_init(icm20948_device_t *device)
{
    (void)device;  // we drive ICM directly via s_icm_handle

    // ── Step 1: Enable BYPASS (direct reg access with verification) ──
    ESP_LOGI(TAG, "mag_init: enabling I2C BYPASS ...");
    if (!mag_enable_bypass()) {
        ESP_LOGE(TAG, "mag_init: BYPASS enable failed");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // ── Step 2: Add AK09916 device on the same bus (addr 0x0C) ──
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = MAG_I2C_ADDR,
        .scl_speed_hz    = I2C_CLK_SPEED,
    };
    esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &cfg, &s_mag_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mag_init: failed to add AK09916 (0x0C): %s", esp_err_to_name(err));
        return false;
    }

    // ── Step 3: Verify AK09916 WHO_AM_I (should be 0x09), try a few times ──
    uint8_t wia = 0;
    bool wia_ok = false;
    for (int attempt = 1; attempt <= 8; attempt++) {
        err = mag_read(MAG_REG_WIA, &wia, 1);
        ESP_LOGI(TAG, "  WIA[%d]: err=%s val=0x%02X",
                 attempt, esp_err_to_name(err), wia);
        if (err == ESP_OK && wia == 0x09) {
            wia_ok = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!wia_ok) {
        ESP_LOGE(TAG, "mag_init: AK09916 WHO_AM_I not 0x09 (got 0x%02X) — magnetometer unavailable", wia);
        i2c_master_bus_rm_device(s_mag_handle);
        s_mag_handle = NULL;
        return false;
    }
    ESP_LOGI(TAG, "mag_init: AK09916 WHO_AM_I OK (0x09)");

    // ── Step 4: Soft reset + set continuous mode 100Hz ──
    mag_write(MAG_REG_CNTL3, 0x01);
    vTaskDelay(pdMS_TO_TICKS(20));

    mag_write(MAG_REG_CNTL2, 0x08);
    ESP_LOGI(TAG, "mag_init: AK09916 continuous mode 100Hz");

    return true;
}

static bool mag_get_data(float *x, float *y, float *z)
{
    // Auto-recovery: if mag was never initialized or got disconnected
    // (e.g. BYPASS got clobbered by ICM library activity), try to init again
    if (s_mag_handle == NULL) {
        static uint32_t s_last_retry_ms = 0;
        uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if (now - s_last_retry_ms > 10000) {  // throttle: at most once every 10s
            ESP_LOGW(TAG, "  mag: s_mag_handle NULL, attempting recovery init");
            if (mag_init(&s_icm_device)) {
                ESP_LOGI(TAG, "  mag: recovery init OK");
            } else {
                ESP_LOGW(TAG, "  mag: recovery init failed, will retry later");
                s_last_retry_ms = now;
                return false;
            }
            s_last_retry_ms = now;
        } else {
            return false;
        }
    }

    uint8_t st1 = 0;
    for (int retry = 0; retry < 4; retry++) {
        if (mag_read(MAG_REG_ST1, &st1, 1) == ESP_OK && (st1 & 0x01))
            break;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (!(st1 & 0x01)) {
        // DRDY never asserted — maybe BYPASS got silently lost (ICM register corruption).
        // Try to re-enable BYPASS once; if that works, try reading again.
        ESP_LOGW(TAG, "  mag: DRDY timeout (st1=0x%02X), re-checking BYPASS...", st1);
        if (mag_enable_bypass()) {
            ESP_LOGI(TAG, "  mag: BYPASS re-enabled, retrying data read");
            vTaskDelay(pdMS_TO_TICKS(10));
            st1 = 0;
            for (int retry = 0; retry < 4; retry++) {
                if (mag_read(MAG_REG_ST1, &st1, 1) == ESP_OK && (st1 & 0x01))
                    break;
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        if (!(st1 & 0x01)) {
            ESP_LOGW(TAG, "  mag: still no DRDY after BYPASS fix — marking stale for full recovery");
            i2c_master_bus_rm_device(s_mag_handle);
            s_mag_handle = NULL;
            return false;
        }
    }

    uint8_t raw[8];
    if (mag_read(MAG_REG_HXL, raw, 8) != ESP_OK) {
        ESP_LOGW(TAG, "  mag: I2C read of HXL~ST2 failed");
        return false;
    }

    if (raw[7] & 0x08) {
        ESP_LOGW(TAG, "  mag: HOFL overflow (magnetic field too strong)");
        return false;
    }

    *x = (float)((int16_t)((raw[1] << 8) | raw[0])) * 0.15f;
    *y = (float)((int16_t)((raw[3] << 8) | raw[2])) * 0.15f;
    *z = (float)((int16_t)((raw[5] << 8) | raw[4])) * 0.15f;
    return true;
}

// ==================================================================
//  Sensor read helpers
// ==================================================================

static bool read_dht11(float *temp, float *humi)
{
    for (int try_num = 0; try_num < 3; try_num++) {
        esp_err_t ret = dht_read_float_data(DHT_TYPE_DHT11, DHT11_GPIO, humi, temp);
        if (ret == ESP_OK) {
            ESP_LOGD(TAG, "DHT11: %.1f°C, %.1f%%", *temp, *humi);
            return true;
        }
        if (try_num < 2) vTaskDelay(pdMS_TO_TICKS(100));
    }
    return false;
}

static bool read_mics5524(int *value)
{
    if (!s_adc_handle) return false;
    int v = 0;
    esp_err_t ret = adc_oneshot_read(s_adc_handle, MICS5524_ADC_CH, &v);
    if (ret == ESP_OK) {
        *value = v;
        ESP_LOGD(TAG, "MICS5524: %d", v);
        return true;
    }
    ESP_LOGW(TAG, "MICS5524 ADC read failed: %s", esp_err_to_name(ret));
    return false;
}

static bool read_icm20948(icm20948_device_t *device,
                          float *ax, float *ay, float *az,
                          float *gx, float *gy, float *gz,
                          float *t, bool *mag_ok,
                          float *mx, float *my, float *mz)
{
    *mag_ok = false;
    *ax = *ay = *az = *gx = *gy = *gz = *t = 0.0f;

    // ICM20948 read = 23 bytes I2C burst read.
    // Easy to fail under WiFi interrupt contention.  Try a few times.
    icm20948_agmt_t agmt;
    icm20948_status_e st = ICM_20948_STAT_ERR;
    for (int attempt = 1; attempt <= 3; attempt++) {
        st = icm20948_get_agmt(device, &agmt);
        if (st == ICM_20948_STAT_OK) {
            if (attempt > 1)
                ESP_LOGI(TAG, "  ICM: recovered on attempt %d", attempt);
            break;
        }
        if (attempt < 3) vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (st != ICM_20948_STAT_OK) {
        // ICM read failed (probably I2C contention under WiFi activity).
        // We can still try to read mag directly via the BYPASS path —
        // it only needs 8 bytes, much less likely to fail.
        ESP_LOGW(TAG, "  ICM: icm20948_get_agmt failed %d (3 attempts), "
                 "trying direct mag read anyway", st);
        *mag_ok = mag_get_data(mx, my, mz);
        if (*mag_ok) {
            ESP_LOGI(TAG, "  ICM: mag recovered after ICM failure "
                     "(X=%.1f Y=%.1f Z=%.1f μT)", *mx, *my, *mz);
        } else {
            ESP_LOGW(TAG, "  ICM: mag also unavailable");
        }
        // still mark overall as "read failed" — Acc/Gyro aren't valid
        // so the caller will use 0 for Acc/Gyro and the *mag_ok flag
        // for Mag.  We return 'false' to indicate the ICM data is bad
        // but the caller should still call sensor_data_mgr_update_imu
        // with the (possibly valid) mag values.
        //
        // To keep it simple:  return true only when Acc/Gyro are good.
        // The caller checks *mag_ok independently for Mag display.
        return false;
    }

    *ax = (float)agmt.acc.axes.x / 16384.0f * 9.80665f;
    *ay = (float)agmt.acc.axes.y / 16384.0f * 9.80665f;
    *az = (float)agmt.acc.axes.z / 16384.0f * 9.80665f;
    *gx = (float)agmt.gyr.axes.x / 131.0f;
    *gy = (float)agmt.gyr.axes.y / 131.0f;
    *gz = (float)agmt.gyr.axes.z / 131.0f;
    *t  = (float)agmt.tmp.val / 333.87f + 21.0f;

    *mag_ok = mag_get_data(mx, my, mz);
    return true;
}

// ==================================================================
//  Unified sensor task
// ==================================================================

static void sensor_task(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "Unified sensor task started");

    // ── MICS5524 init ──
    {
        adc_oneshot_unit_init_cfg_t adc_init = { .unit_id = ADC_UNIT_1 };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init, &s_adc_handle));
        adc_oneshot_chan_cfg_t adc_chan = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, MICS5524_ADC_CH, &adc_chan));
        ESP_LOGI(TAG, "MICS5524 ADC ready");
    }

    // ── ICM-20948 init ──
    {
        s_bus_handle = bsp_i2c_get_bus();
        if (!s_bus_handle) {
            ESP_LOGE(TAG, "BSP I2C bus not initialized — IMU disabled");
            s_imu_present = false;
            goto skip_imu;
        }

        i2c_device_config_t icm_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = ICM_I2C_ADDR,
            .scl_speed_hz    = I2C_CLK_SPEED,
        };
        esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &icm_cfg, &s_icm_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ICM device add failed: %s — IMU disabled", esp_err_to_name(err));
            s_imu_present = false;
            goto skip_imu;
        }

        icm20948_init_struct(&s_icm_device);
        icm20948_serif_t serif = { .write = icm_i2c_write, .read = icm_i2c_read };
        icm20948_link_serif(&s_icm_device, &serif);

        if (icm20948_check_id(&s_icm_device) != ICM_20948_STAT_OK) {
            ESP_LOGE(TAG, "ICM-20948 WHO_AM_I mismatch — IMU disabled");
            i2c_master_bus_rm_device(s_icm_handle);
            s_icm_handle = NULL;
            s_imu_present = false;
            goto skip_imu;
        }
        ESP_LOGI(TAG, "ICM-20948 WHO_AM_I OK (0xEA)");

        icm20948_sleep(&s_icm_device, false);
        icm20948_set_clock_source(&s_icm_device, CLOCK_AUTO);

        icm20948_fss_t fss = { .a = GPM_2, .g = DPS_250 };
        icm20948_set_full_scale(&s_icm_device, ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR, fss);

        icm20948_dlpcfg_t dlpcfg = { .a = ACC_D246BW_N265BW, .g = GYR_D361BW4_N376BW5 };
        icm20948_set_dlpf_cfg(&s_icm_device, ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR, dlpcfg);
        icm20948_enable_dlpf(&s_icm_device, ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR, true);

        icm20948_smplrt_t smplrt = { .a = 9, .g = 9 };
        icm20948_set_sample_rate(&s_icm_device, ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR, smplrt);
        icm20948_set_sample_mode(&s_icm_device, ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR,
            SAMPLE_MODE_CONTINUOUS);

        // Let ICM internal state machine settle before enabling BYPASS
        // (prevents cold-start race: mag_init immediately after set_sample_mode
        //  can fail because ICM hasn't finished switching to continuous mode yet)
        vTaskDelay(pdMS_TO_TICKS(20));

        // Magnetometer init (BYPASS mode)
        bool mag_avail = mag_init(&s_icm_device);
        ESP_LOGI(TAG, "ICM-20948 running (%s mag)", mag_avail ? "with" : "without");
        s_imu_present = true;
    }
skip_imu:

    esp_task_wdt_add(NULL);

    // ── Main sensor loop ──
    uint32_t cycle = 0;
    int      gas_val;
    float    temp, humi;
    float    ax, ay, az, gx, gy, gz, icm_temp, mx, my, mz;
    bool     mag_ok;

    while (1) {
        cycle++;
        esp_task_wdt_reset();

        // 1. DHT11
        temp = humi = 0;
        if (read_dht11(&temp, &humi)) {        // internal 3-retry
            sensor_data_mgr_update_temperature(temp);
            sensor_data_mgr_update_humidity(humi);
            s_dht_fail = 0;
        } else {
            s_dht_fail++;
            if (s_dht_fail >= DHT_MAX_FAIL) {
                // silent skip — don't delete, just stop trying
            }
        }
        esp_task_wdt_reset();

        // 2. MICS5524
        gas_val = 0;
        if (read_mics5524(&gas_val)) {
            sensor_data_mgr_update_gas(gas_val);
        }
        esp_task_wdt_reset();

        // 3. ICM-20948
        if (s_imu_present) {
            ax = ay = az = gx = gy = gz = icm_temp = mx = my = mz = 0;
            mag_ok = false;
            bool icm_ok = read_icm20948(&s_icm_device, &ax, &ay, &az, &gx, &gy, &gz, &icm_temp, &mag_ok, &mx, &my, &mz);
            if (icm_ok) {
                sensor_data_mgr_update_imu(ax, ay, az, gx, gy, gz, mx, my, mz, icm_temp, mag_ok);
            } else if (mag_ok) {
                ESP_LOGI(TAG, "  ICM: mag-only update this cycle");
                sensor_data_mgr_update_mag_only(mx, my, mz);
            }
        }
        esp_task_wdt_reset();

        // 4. Notify MQTT task to publish (JSON generation + TCP send
        //    happen in mqtt_task, decoupled from sensor read timing)
        app_mqtt_notify_new_data();

        // ── Human-readable sensor snapshot ──
        ESP_LOGI(TAG, "══ Cycle #%lu ═══════════════════════════════════", (unsigned long)cycle);
        if (s_dht_fail >= DHT_MAX_FAIL) {
            ESP_LOGI(TAG, "  DHT11   : not present");
        } else if (temp != 0 || humi != 0) {
            ESP_LOGI(TAG, "  DHT11   : Temp=%5.1f°C  Humi=%5.1f%%", temp, humi);
        } else {
            ESP_LOGI(TAG, "  DHT11   : no data this cycle");
        }
        ESP_LOGI(TAG, "  MICS5524: Gas=%4d (ADC)", gas_val);
        if (s_imu_present) {
            ESP_LOGI(TAG, "  ICM20948 : Accel  X=%+7.3f  Y=%+7.3f  Z=%+7.3f  m/s²", ax, ay, az);
            ESP_LOGI(TAG, "           : Gyro   X=%+7.2f  Y=%+7.2f  Z=%+7.2f  °/s", gx, gy, gz);
            ESP_LOGI(TAG, "           : Temp   %.1f °C", icm_temp);
            if (mag_ok) {
                ESP_LOGI(TAG, "           : Mag    X=%+7.1f  Y=%+7.1f  Z=%+7.1f  µT", mx, my, mz);
            } else {
                ESP_LOGI(TAG, "           : Mag    (no data)");
            }
        } else {
            ESP_LOGI(TAG, "  ICM20948 : not present");
        }
        ESP_LOGI(TAG, "  Notified MQTT task");
        ESP_LOGI(TAG, "══════════════════════════════════════════════════");

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ==================================================================
//  Public API
// ==================================================================

void task_app_sensor_start(void)
{
    BaseType_t ret = xTaskCreate(
        sensor_task,
        "task_sensor",
        8192,
        NULL,
        2,
        NULL
    );

    if (ret == pdPASS) {
        ESP_LOGI(TAG, "Unified sensor task created");
        task_monitor_register("sensor", xTaskGetHandle("task_sensor"), 8192);
    } else {
        ESP_LOGE(TAG, "Failed to create unified sensor task");
    }
}
