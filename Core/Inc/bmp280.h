#ifndef BMP280_H
#define BMP280_H

#include "stm32f4xx_hal.h"

// BMP280 I2C addresses (try both if one doesn't work)
#define BMP280_I2C_ADDR1    0x76 << 1  // 0xEC
#define BMP280_I2C_ADDR2    0x77 << 1  // 0xEE

// BMP280/BME280 registers
#define BMP280_REG_ID           0xD0
#define BMP280_REG_CTRL_HUM     0xF2  // BME280 humidity control
#define BMP280_REG_CTRL_MEAS    0xF4
#define BMP280_REG_CONFIG       0xF5
#define BMP280_REG_HUM_MSB      0xFD  // BME280 humidity data
#define BMP280_REG_TEMP_MSB     0xFA
#define BMP280_REG_PRESS_MSB    0xF7

// Calibration data registers
#define BMP280_REG_CALIB_START  0x88
#define BME280_REG_CALIB_HUM1   0xA1
#define BME280_REG_CALIB_HUM2   0xE1

// Structure to hold calibration data
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;  // BME280 humidity calibration
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} BMP280_CalibData;

// Structure to hold sensor data
typedef struct {
    float temperature;  // Temperature in degrees Celsius
    float pressure;     // Pressure in hPa (hectopascals)
    float humidity;     // Relative humidity in %
} BMP280_Data;

// Function prototypes
uint8_t BMP280_Init(I2C_HandleTypeDef *hi2c);
uint8_t BMP280_ReadData(I2C_HandleTypeDef *hi2c, BMP280_Data *data);

#endif // BMP280_H
