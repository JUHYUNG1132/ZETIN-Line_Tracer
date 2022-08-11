/*
 * motor.c
 *
 *  Created on: Apr 12, 2021
 *      Author: Joonho Gwon
 */

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "motor.h"
#include "custom_delay.h"
#include "custom_gpio.h"
#include "custom_lcd.h"
#include "custom_switch.h"

/*
 * 이 변수들은 왼쪽 모터와 오른쪽 모터의 틱수를 기록한다.
 * uint32_t 형식으로 선언했으므로 대략 42억 9천만정도까지 기록이 가능하다.
 * 모터가 1틱 주행할 때 거리를 계산해보면 한 바퀴가 400틱이고 바퀴의 반지름이 대략 2.5cm정도 되므로
 * 25mm*2*pi/400 = 0.392mm정도가 된다.
 * 여기에 2^32를 곱해보면 1686km나 된다. 그러므로 적절히 초기화만 잘 해 준다면 변수 overflow는 걱정할 필요가 없다.
 */
volatile uint32_t motorTickL = 0;
volatile uint32_t motorTickR = 0;

Custom_GPIO_t motorL[4] = {
		{ Motor_L1_GPIO_Port, Motor_L1_Pin },
		{ Motor_L3_GPIO_Port, Motor_L3_Pin },
		{ Motor_L2_GPIO_Port, Motor_L2_Pin },
		{ Motor_L4_GPIO_Port, Motor_L4_Pin },
};

Custom_GPIO_t motorR[4] = {
		{ Motor_R1_GPIO_Port, Motor_R1_Pin },
		{ Motor_R3_GPIO_Port, Motor_R3_Pin },
		{ Motor_R2_GPIO_Port, Motor_R2_Pin },
		{ Motor_R4_GPIO_Port, Motor_R4_Pin },
};

volatile static uint8_t phases[8] = { 0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08,
		0x09 };

void Motor_Start() {
	LL_TIM_EnableCounter(TIM3);
	LL_TIM_EnableIT_UPDATE(TIM3);

	LL_TIM_EnableCounter(TIM4);
	LL_TIM_EnableIT_UPDATE(TIM4);
}

void Motor_Stop() {
	LL_TIM_DisableIT_UPDATE(TIM3);
	LL_TIM_DisableCounter(TIM3);

	LL_TIM_DisableIT_UPDATE(TIM4);
	LL_TIM_DisableCounter(TIM4);

	for (int i = 0; i < 4; i++) {
		Custom_GPIO_Set_t(motorL + i, 0);
		Custom_GPIO_Set_t(motorR + i, 0);
	}
}

/*
 * 아래 모터 인터럽트 두 개의 내용은 거의 흡사해서, 함수로 만들어서 따로 뺄 수도 있고 Custom_GPIO_Set_t 부분을 for문으로 할 수도 있다.
 * 그러나 그렇게 하지 않은 것은 속도 향상을 위해서이다. for문을 사용하면 인덱스 변수 초기화, 비교, 인덱스 변수 증가 등에서 클럭이 낭비된다.
 * 또 함수 호출은 스택 포인터를 변화시키는 등 많은 연산을 요구한다.
 * Custom_GPIO_Set_t 함수 역시 inline 함수로, 컴파일될 때에는 함수 호출 없이 직접 코드가 삽입될 것이다.
 * 이렇게 실행 속도를 극도로 신경쓰는 이유는 이들이 인터럽트 핸들러이기 때문이다.
 * 인터럽트 핸들러가 너무 길게 실행되면 우선순위에 따라 다른 인터럽트가 무시되거나 지연될 수 있다. 따라서 인터럽트 핸들러는 최대한 빠르게 실행된 후 종료되어야 한다.
 */

void Motor_L_TIM3_IRQ() {
	static uint8_t index = 0;
	static uint8_t phase;
	phase = phases[index];
	Custom_GPIO_Set_t(motorL + 0, phase & 0x01);
	Custom_GPIO_Set_t(motorL + 1, phase & 0x02);
	Custom_GPIO_Set_t(motorL + 2, phase & 0x04);
	Custom_GPIO_Set_t(motorL + 3, phase & 0x08);
	index++;
	index &= 0x07;
	motorTickL++;
}

void Motor_R_TIM4_IRQ() {
	static uint8_t index = 0;
	static uint8_t phase;
	phase = phases[index];
	Custom_GPIO_Set_t(motorR + 0, phase & 0x01);
	Custom_GPIO_Set_t(motorR + 1, phase & 0x02);
	Custom_GPIO_Set_t(motorR + 2, phase & 0x04);
	Custom_GPIO_Set_t(motorR + 3, phase & 0x08);
	index++;
	index &= 0x07;
	motorTickR++;
}
