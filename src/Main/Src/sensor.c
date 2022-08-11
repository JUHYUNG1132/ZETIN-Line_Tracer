/*
 * sensor.c
 *
 *  Created on: Apr 11, 2021
 *      Author: Joonho Gwon
 *  MODIFIED on: 2022 / 08 / 11
 *  	Author: 이주형
 */

#include "main.h"
#include "sensor.h"
#include "custom_delay.h"
#include "custom_gpio.h"
#include "custom_lcd.h"
#include "custom_switch.h"
#include "custom_filesystem.h"
#include "custom_exception.h"

#define SMP0 Sensor_Mux0_Pin
#define SMP1 Sensor_Mux1_Pin
#define SMP2 Sensor_Mux2_Pin

// 플래시에 센서 설정값을 저장할 때 사용할 이름
#define SENSOR_SETTING_FILE "Sensor Setting"

uint32_t SensorOrder[SENSOR_NUM] = {
		0x00 | 0x00 | 0x00,
		0x00 | 0x00 | SMP0,
		0x00 | SMP1 | 0x00,
		0x00 | SMP1 | SMP0,
		SMP2 | 0x00 | 0x00,
		SMP2 | 0x00 | SMP0,
		SMP2 | SMP1 | 0x00,
		SMP2 | SMP1 | SMP0,
};
uint32_t SensorMask = SMP0 | SMP1 | SMP2;

/*
 * 라인트레이서에 사용하는 각 센서 8개는 모두 약간씩 인식 범위의 차이가 있다.
 * 그리고 빛이 아무리 어두워도 노이즈로 인해 0이 나오지 않는 경우가 있으며, 아무리 밝아도 센서의 한계로 최댓값(이 경우 4096)이 나오지 않는다.
 * 그래서 이 인식범위가 모두 0~255가 되도록 맞춰주어야 한다. 이 과정을 normalize(정규화)라 한다.
 * normalize하는 데에는 다양한 방법이 있지만, 우리는 성능상 단순히 선형 변환을 통해 normalize를 수행한다.
 * 즉, x가 ADC에서 읽은 raw data라고 하자. 각 센서에 대해 y = 255*(x-min(x))/(max(x)-min(x))라 하면, y는 0~255의 범위를 가지게 된다.
 *
 * 이때 주의해야 할 점은 반드시 곱하기를 먼저 수행한 후 나누기를 수행해야 한다는 점이다.
 * 정수 곱하기를 수행할 경우 overflow가 나지 않는 이상 데이터의 정확도가 줄어들지는 않는다.
 * 그러나 정수 나누기를 수행할 경우 소숫점 이하 자릿수가 버려지므로 데이터의 정확도가 줄어들기 때문이다.
 * 예를 들어 2*3/4 는 실제로는 1.5이다. 이를 곱하기부터 수행하면 (2*3)/4 = 6/4 = 1이지만,
 * 나누기부터 수행하면 2*(3/4) = 0이 된다.
 *
 * 그리고 아래 변수들을 보면 raw data와 coefficient가 모두 32bit 정수형을 사용하고 있음을 알 수 있다.
 * 이는 오버플로를 방지하고 계산 속도를 향상시키기  위해서다.
 * 언듯 생각하면 자료형의 크기가 작을수록 계산이 빠를 것 같지만, CPU 구조상 워드 크기에 해당하는 연산이 가장 빠르다.
 * STM32F411의 경우 그 이름에서도 알 수 있듯 32bit word 크기를 가지는 CPU이므로 32bit 연산이 가장 고속으로 수행된다.
 *
 * 그리고 인터럽트 내부에서 읽기 / 쓰기가 이뤄질 수 있는 변수들은 반드시 volatile 키워드를 사용하여 선언해야 한다.
 * 왜냐하면 컴파일러는 최적화 과정에서 사용되지 않는 변수를 삭제할 수도 있는데, 인터럽트는 소스코드상에서는 명시적으로 호출되는 부분이 없으므로
 * 컴파일러는 인터럽트에서만 접근되는 변수를 상수로 취급해버릴 수도 있기 때문이다.
 */

volatile int32_t sensorRaw[SENSOR_NUM] = { 0 };			// ADC에서 바로 읽은 데이터. 0~4096
volatile int32_t sensorNormalized[SENSOR_NUM] = { 0 };	// 0~255 범위로 맞춰진 데이터
volatile uint8_t sensorState;							// 0,1로 나타낸 센서 상태

void Sensor_Start() {
	LL_ADC_Enable(ADC1);
	/*
	 * ADC를 켜고 난 후, ADC 변환을 하기 전  내부 아날로그 안정화 작업을 거치기 위해 딜레이가 필요하다.
	 * 구체적으로 얼마나 요구되는지는 모르겠지만, 많아 봐야 수백 클럭 이내에 끝날 것이 분명하므로 여유롭게 10ms정도 쉬어준다.
	 * 10ms면 사람에게는 찰나이지만 CPU에게는 100만 클럭이나 되는 시간이다.
	 */
	Custom_Delay_ms(10);

	/*
	 * TIM5는 device configuration tool에서 클럭 및 NVIC 등이 설정되어있다.
	 * 그러므로 전원이 공급되고는 있으나, 전원을 공급한다고 해서 카운터가 증가한다거나 인터럽트가 호출되는 것은 아니다.
	 * 타이머를 실제로 동작하게 하려면 아래 두 함수를 추가로 호출해야 한다.
	 *
	 * LL_TIM_EnableCounter 함수는 타이머의 카운터가 증가하도록 설정한다.
	 * 이렇게 하면 타이머의 카운터가 공급한 클럭과 prescaler에 따라 증가한다.
	 *
	 * LL_TIM_EnableTI_UPDATE 함수는 타이머의 인터럽트가 발생하도록 설정한다.
	 * 이 함수를 호출하면 이후 타이머의 카운터가 ARR에 도달하여 overflow가 발생하여 다시 0으로 되돌아갈 때마다 인터럽트가 발생한다.
	 * 만약 이 함수를 호출하지 않는다면 단순히 overflow만 발생하고 인터럽트는 발생하지 않는다.
	 *
	 * 인터럽트가 발생하면 Core/Src/stm32f4xx_it.c 파일 내부에 있는 인터럽트 핸들러 함수가 호출된다.
	 * TIM5의 경우, 그 함수 내부에서 다시 이 파일에 포함된 Sensor_TIM5_IRQ 함수를 호출하도록 해 놓았다.
	 *
	 * TIM5는 prescaler가 0이므로 카운터가 100MHz로 동작하고, ARR이 10000이므로 interrupt는 100us마다 호출된다.
	 */

	LL_TIM_EnableCounter(TIM5);
	LL_TIM_EnableIT_UPDATE(TIM5);
}

void Sensor_Stop() {
	/*
	 * 센서를 끌 때에는 켤 때와 반대로 ADC 및 타이머, 센서와 관련된 GPIO를 모두 끈다.
	 * 끌 때의 순서는 별로 중요하지 않다.
	 */

	LL_ADC_Disable(ADC1);
	LL_TIM_DisableCounter(TIM5);
	LL_TIM_DisableIT_UPDATE(TIM5);

	GPIOC->ODR &= ~(SensorMask | Sensor_MuxX_Pin);
}

__STATIC_INLINE uint16_t Sensor_ADC_Read() {
	/*
	 * STM32F411의 ADC에는 Regular group과 injected group이라는 것이 있다.
	 * 이것은 그냥 ADC를 두 개 그룹으로 분할할 수 있도록 구성해놓은 것으로 크게 중요하지는 않다.
	 * 그런 게 있다는 것만 알면 되며 우리는 그중 Regular group만을 사용한다.
	 *
	 * conversion 과정에는 single conversion과 continuous conversion 방식이 있다.
	 * ADC는 하드웨어적으로 여러 개의 핀(채널이라고 부름)에 연결되어있어서 한번에 한 개의 채널에서 전압값을 읽어올 수 있다.
	 * single conversion은 한번에 한 개의 채널에서만 값을 읽어오는 방식이며, continuous conversion은
	 * 미리 지정된 순서대로  여러 개의 채널에서 값을 읽어오는 것이다.
	 * 우리는 여러 개의 채널에서 값을 읽을 일이 없으므로 single conversion만 사용할 것이다.
	 * main.c 파일의 초기화 부분(자동으로 생성된)에서 이미 single conversion을 사용하도록 설정되어 있다.
	 *
	 * 지금은 IR LED 8개짜리 8초 트레이서를 제작하지만 추후 16조 트레이서를 제작할 때에는 ADC가 2개 이상 필요하다.
	 * 이때는  continuous conversion을 사용하면 편리할 것이다.
	 *
	 * 마지막으로 ADC conversion을 시작하는 데 있어서도 software에 의한 시작과 external event에 의한 시작이 있다.
	 * software는 말 그대로 코드를 사용해서 conversion을 시작하는 것을 말하고, external은 타이머나 와치독 등 다른 주변기기나
	 * GPIO 핀의 신호에 의해 conversion이 발생하는 것을 말한다.
	 *
	 * 아래 LL_ADC_REG_StartConversionSWStart 함수를 사용하면 Conversion이 시작된다.
	 * REG는 Regular, SWStart는 software start를 의미힌다.
	 *
	 * LL_ADC_ClearFlag_EOCS 함수를 호출하여 EOC(End Of Conversion) flag를 0으로 설정한 후
	 * ADC conversion을 수행하면 conversion이 끝날 때 EOC Flag가 1로 바뀐다.
	 * ADC conversion이 끝났으면 이를 다시 0으로 돌려 주어야 한다.
	 * 사실 conversion이 끝난 후에만 clear를 해 주면 되며, 시작 전에 clear를 해 주는 것은 혹시 모를 에러를 방지하기 위함이다.
	 *
	 * __disable_irq(), __enable_irq() 함수는 인터럽트를 차단하고 복구하는 함수이다.
	 * 이는 이 인터럽트에서 ADC를 읽는 도중 ADC를 읽어야 하는 또다른 인터럽트가 발생하면 문제가 생기기 때문이다.
	 * 스텝모터 라인트레이서의 경우 오직 센서를 읽기 위해서 ADC를 사용하지만, DC모터의 경우 배터리 전압을 읽는 데에도 ADC를 사용해야 한다.
	 * 배터리 전압을 읽는 것 역시 주기적으로 인터럽트 내에서 이루어진다.
	 * 그러므로 ADC를 사용할 때에는 이러한 문제를 막기 위해서 반드시 인터럽트를 끄는 것이 바람직하다.
	 *
	 */

	__disable_irq();
	LL_ADC_ClearFlag_EOCS(ADC1);
	LL_ADC_REG_StartConversionSWStart(ADC1);
	while (!LL_ADC_IsActiveFlag_EOCS(ADC1));
	uint16_t adcValue = LL_ADC_REG_ReadConversionData12(ADC1);
	LL_ADC_ClearFlag_EOCS(ADC1);
	__enable_irq();
	return adcValue;
}

void Sensor_TIM5_IRQ() {
	/*
	 * 언듯 생각하기에 지역 변수보다 전역 변수를 사용하는 것이 효율적일 것처럼 보일 수 있다.
	 * 왜냐하면 전역 변수는 한번 만들어놓고 업데이트만 하면 되지만, 지역 변수는 매번 함수가 호출될 때마다 변수를 만들어야 하기 때문이다.
	 * 1. 일반적인 현대 CPU 아키텍쳐 상에서
	 * 2. C언어를 사용할 때
	 * 3. 변수가 배열이나 구조체가 아닌 정수형 변수라면
	 * 지역 변수 접근이 전역 변수 접근보다 빠르다.
	 * 왜냐햐면 그런 경우에는 컴파일러는 최대한 메모리 대신 레지스터에 지역 변수를 할당하려고 시도하기 때문이다.
	 * 따라서 꼭 필요한 경우가 아니라면 굳이 전역 변수를 사용할 필요가 없다.
	 */

	static uint32_t i = 0; 	// 현재 값을 읽을 센서 인덱스
	int32_t raw;			// 센서에서 바로 읽은 raw data

	// Mux를 사용하여 IR LED 및 수광 센서 선택
	GPIOC->ODR = (GPIOC->ODR & ~SensorMask) | SensorOrder[i];

	GPIOC->ODR |= Sensor_MuxX_Pin;	// IR LEC 켜기
	raw = Sensor_ADC_Read() >> 4;	// ADC 읽기
	GPIOC->ODR &= ~Sensor_MuxX_Pin;	// IR LED 끄기

	// 다른 곳에서 쓸 수 있게 전역 배열에 할당
	sensorRaw[i] = raw;

	// 인덱스 증가
	i = (i + 1) & 0x07;
}

void Sensor_RAW(){
	Sensor_Start();
	while(!(Custom_Switch_Read() == CUSTOM_SW_BOTH)){
		Custom_LCD_Printf("/0%02x%02x%02x%02x/1%02x%02x%02x%02x", sensorRaw[0], sensorRaw[1], sensorRaw[2], sensorRaw[3], sensorRaw[4],
												sensorRaw[5], sensorRaw[6], sensorRaw[7]);
	}
	Sensor_Stop();
}
