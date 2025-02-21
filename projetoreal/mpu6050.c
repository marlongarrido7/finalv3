#include "mpu6050.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

bool mpu6050_init(mpu6050_t *mpu, i2c_inst_t *i2c, uint8_t addr) {
    mpu->i2c = i2c;
    mpu->addr = addr;
    uint8_t buf[2];
    buf[0] = MPU6050_REG_PWR_MGMT_1;
    buf[1] = 0; // Despertar o sensor
    int ret = i2c_write_blocking(mpu->i2c, mpu->addr, buf, 2, false);
    return (ret == 2);
}

bool mpu6050_read_accel(mpu6050_t *mpu, float *ax, float *ay, float *az) {
    uint8_t reg = MPU6050_REG_ACCEL_XOUT_H;
    uint8_t data[6];
    int ret = i2c_write_blocking(mpu->i2c, mpu->addr, &reg, 1, true);
    if (ret != 1) return false;
    ret = i2c_read_blocking(mpu->i2c, mpu->addr, data, 6, false);
    if (ret != 6) return false;
    
    int16_t raw_ax = (data[0] << 8) | data[1];
    int16_t raw_ay = (data[2] << 8) | data[3];
    int16_t raw_az = (data[4] << 8) | data[5];
    
    // Sensibilidade típica: 16384 LSB/g para ±2g
    *ax = raw_ax / 16384.0f;
    *ay = raw_ay / 16384.0f;
    *az = raw_az / 16384.0f;
    return true;
}
