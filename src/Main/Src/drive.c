/*
 * drive.c
 *
 *  Created on: May 23, 2021
 *      Author: Joonho Gwon
 */

#include <stdint.h>
#include <stdbool.h>
#include "drive.h"
#include "motor.h"
#include "sensor.h"
#include "custom_lcd.h"
#include "custom_switch.h"
#include "custom_exception.h"

// 500us마다 호출됨.
void Drive_TIM9_IRQ() {
	/*
	 * 이 인터럽트가 필요한 이유는 가속도 제어 때문이다.
	 * while문을 사용하면 특정 시간마다 한 번씩 제어하기가 어려운 반면(불가능한 것은 아님)
	 * 인터럽트를 사용하면 그런 작업이 쉽게 가능하다.
	 * 그러나 인터럽트 함수인 만큼 너무 많은 작업을 하면 안 된다.
	 */
}
