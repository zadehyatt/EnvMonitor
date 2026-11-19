#include "bmp280.h"

static uint8_t bmp280_addr;
static BMP280_CalibData calib;
static int32_t t_fine;  // Used for temperature compensation

// Initialize BMP280/BME280
uint8_t BMP280_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t id;
    uint8_t config[2];
    uint8_t calib_data[24];
    uint8_t calib_hum[7];
    
    // Try to find the sensor at either address
    if(HAL_I2C_IsDeviceReady(hi2c, BMP280_I2C_ADDR1, 1, 100) == HAL_OK) {
        bmp280_addr = BMP280_I2C_ADDR1;
    } else if(HAL_I2C_IsDeviceReady(hi2c, BMP280_I2C_ADDR2, 1, 100) == HAL_OK) {
        bmp280_addr = BMP280_I2C_ADDR2;
    } else {
        return 0;  // Sensor not found
    }
    
    // Read chip ID (0x58 for BMP280, 0x60 for BME280)
    HAL_I2C_Mem_Read(hi2c, bmp280_addr, BMP280_REG_ID, 1, &id, 1, HAL_MAX_DELAY);
    if(id != 0x58 && id != 0x60) {
        return 0;  // Wrong chip ID
    }
    
    // Read temperature and pressure calibration data
    HAL_I2C_Mem_Read(hi2c, bmp280_addr, BMP280_REG_CALIB_START, 1, calib_data, 24, HAL_MAX_DELAY);
    
    // Parse calibration data
    calib.dig_T1 = (calib_data[1] << 8) | calib_data[0];
    calib.dig_T2 = (calib_data[3] << 8) | calib_data[2];
    calib.dig_T3 = (calib_data[5] << 8) | calib_data[4];
    calib.dig_P1 = (calib_data[7] << 8) | calib_data[6];
    calib.dig_P2 = (calib_data[9] << 8) | calib_data[8];
    calib.dig_P3 = (calib_data[11] << 8) | calib_data[10];
    calib.dig_P4 = (calib_data[13] << 8) | calib_data[12];
    calib.dig_P5 = (calib_data[15] << 8) | calib_data[14];
    calib.dig_P6 = (calib_data[17] << 8) | calib_data[16];
    calib.dig_P7 = (calib_data[19] << 8) | calib_data[18];
    calib.dig_P8 = (calib_data[21] << 8) | calib_data[20];
    calib.dig_P9 = (calib_data[23] << 8) | calib_data[22];
    
    // If BME280, read humidity calibration data
    if(id == 0x60) {
        // Read H1 (1 byte at 0xA1)
        HAL_I2C_Mem_Read(hi2c, bmp280_addr, BME280_REG_CALIB_HUM1, 1, &calib.dig_H1, 1, HAL_MAX_DELAY);
        
        // Read H2-H6 (7 bytes starting at 0xE1)
        HAL_I2C_Mem_Read(hi2c, bmp280_addr, BME280_REG_CALIB_HUM2, 1, calib_hum, 7, HAL_MAX_DELAY);
        
        calib.dig_H2 = (calib_hum[1] << 8) | calib_hum[0];
        calib.dig_H3 = calib_hum[2];
        calib.dig_H4 = (calib_hum[3] << 4) | (calib_hum[4] & 0x0F);
        calib.dig_H5 = (calib_hum[5] << 4) | (calib_hum[4] >> 4);
        calib.dig_H6 = calib_hum[6];
        
        // Configure humidity oversampling x1
        config[0] = 0x01;  // ctrl_hum: osrs_h=1
        HAL_I2C_Mem_Write(hi2c, bmp280_addr, BMP280_REG_CTRL_HUM, 1, config, 1, HAL_MAX_DELAY);
    }
    
    // Configure sensor: normal mode, temp and pressure oversampling x2
    config[0] = 0x57;  // ctrl_meas: osrs_t=2, osrs_p=2, mode=normal
    HAL_I2C_Mem_Write(hi2c, bmp280_addr, BMP280_REG_CTRL_MEAS, 1, config, 1, HAL_MAX_DELAY);
    
    config[0] = 0x00;  // config: standby=0.5ms, filter=off
    HAL_I2C_Mem_Write(hi2c, bmp280_addr, BMP280_REG_CONFIG, 1, config, 1, HAL_MAX_DELAY);
    
    HAL_Delay(100);  // Wait for sensor to stabilize
    
    return 1;  // Success
}

// Compensate temperature (from BMP280 datasheet)
static int32_t compensate_temp(int32_t adc_T) {
    int32_t var1, var2, T;
    
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// Compensate pressure (from BMP280 datasheet)
static uint32_t compensate_press(int32_t adc_P) {
    int64_t var1, var2, p;
    
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    
    if(var1 == 0) {
        return 0;  // Avoid division by zero
    }
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    
    return (uint32_t)p;
}

// Compensate humidity (from BME280 datasheet)
static uint32_t compensate_humidity(int32_t adc_H) {
    int32_t v_x1_u32r;
    
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - 
                 (((int32_t)calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * 
                 (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) * 
                 (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + 
                 ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                 ((int32_t)calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    
    return (uint32_t)(v_x1_u32r >> 12);
}

// Read temperature, pressure, and humidity
uint8_t BMP280_ReadData(I2C_HandleTypeDef *hi2c, BMP280_Data *data) {
    uint8_t raw_data[8];
    int32_t adc_T, adc_P, adc_H;
    int32_t temp_comp;
    uint32_t press_comp, hum_comp;
    
    // Read raw data (pressure MSB, LSB, XLSB, temp MSB, LSB, XLSB, hum MSB, LSB)
    if(HAL_I2C_Mem_Read(hi2c, bmp280_addr, BMP280_REG_PRESS_MSB, 1, raw_data, 8, HAL_MAX_DELAY) != HAL_OK) {
        return 0;
    }
    
    // Parse raw pressure and temperature
    adc_P = ((int32_t)raw_data[0] << 12) | ((int32_t)raw_data[1] << 4) | ((int32_t)raw_data[2] >> 4);
    adc_T = ((int32_t)raw_data[3] << 12) | ((int32_t)raw_data[4] << 4) | ((int32_t)raw_data[5] >> 4);
    
    // Compensate temperature (must be done first for t_fine)
    temp_comp = compensate_temp(adc_T);
    data->temperature = (float)temp_comp / 100.0f;  // Convert to degrees C
    
    // Compensate pressure
    press_comp = compensate_press(adc_P);
    data->pressure = (float)press_comp / 25600.0f;  // Convert to hPa
    
    // Parse and compensate humidity (BME280 only)
    adc_H = ((int32_t)raw_data[6] << 8) | (int32_t)raw_data[7];
    hum_comp = compensate_humidity(adc_H);
    data->humidity = (float)hum_comp / 1024.0f;  // Convert to %RH
    
    return 1;
}
