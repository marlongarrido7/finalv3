#ifndef MPU6050_H
#define MPU6050_H

#include "hardware/i2c.h"
#include <stdbool.h>

typedef struct {
    i2c_inst_t *i2c;
    uint8_t addr;
} mpu6050_t;

// Inicializa o MPU6050; retorna true em caso de sucesso.
bool mpu6050_init(mpu6050_t *mpu, i2c_inst_t *i2c, uint8_t addr);
// Lê os valores de aceleração (em g) dos eixos X, Y e Z.
bool mpu6050_read_accel(mpu6050_t *mpu, float *ax, float *ay, float *az);

#endif
