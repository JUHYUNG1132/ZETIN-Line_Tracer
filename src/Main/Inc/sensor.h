/*
 * sensor.h
 *
 *  Created on: Apr 12, 2021
 *      Author: Joonho Gwon
 */

// μΌμ κ°μ
#define SENSOR_NUM 8

void Sensor_TIM5_IRQ();

void Sensor_Start();
void Sensor_Stop();
void Sensor_RAW();
void Sensor_CALI();
void Sensor_NORM();
