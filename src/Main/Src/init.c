/*
 * init.c
 *
 *  Created on: Apr 11, 2021
 *      Author: Write your name
 *  MODIFIED on: 2022 / 08 / 11
 *  	Author: 이주형
 */

#include "init.h"
#include "motor.h"
#include "drive.h"
#include "sensor.h"
#include "custom_lcd.h"
#include "custom_switch.h"
#include "custom_filesystem.h"

void Init() {
	/*
	 * LCD를 사용하기 전에는 Custom_LCD_Init 함수를 호출하여 여러가지 초기화를 수행해야 한다.
	 * 이 함수는 LCD를 처음 쓰기 전에 딱 한 번만 호출하면 된다.
	 */
	Custom_LCD_Init();

	/*
	 * 플래시를 사용하기 전에는 Custom_FileSystem_Load 함수를 호출하여 플래시 정보를 불러와야 한다.
	 * 이 함수는 플래시를 처음 쓰기 전에 딱 한 번만 호출하면 된다.
	 */
	Custom_FileSystem_Load();

	/**
	 * Custom_LCD_Printf 함수는 C언어에서 printf와 동일하게 동작한다. (실제로 stdio.h의 vsprint함수를 이용하여 구현되어있다.)
	 * 즉, %d, %f 등을 사용하여 숫자를 출력할 수 있다.
	 * 다만 특수한 기능이 하나 추가되어있는데, /0이라는 부분이 있으면 첫 번째 줄의 첫 번째 칸으로 돌아가고,
	 * /1이라는 부분이 있으면 두 번째 줄의 첫 번째 칸으로 돌아간다.
	 * 즉, 아래 예제에서는 첫 번째 줄에 Hello를 출력한 후, 두 번째 줄의 첫 번째 칸으로 커서가 이동하여 ZETIN! 을 쓴다.
	 */
	Custom_LCD_Backlight(true);
	Custom_LCD_Clear();
	Custom_LCD_Printf(" Hello/1 ZETIN!");
	Custom_Delay_ms(500);
	Custom_LCD_Clear();

	/*
	 * 아래는 스위치를 사용하는 예제다.
	 *
	 * 스위치의 경우 이미 device configuration tool에서 필요한 초기화를 모두 마쳐 놓았기 때문에 별도의 초기화가 필요없다.
	 * Custom_Switch_Read 함수는 GPIO를 읽어 적절한 처리를 한 후 결과를 반환한다.
	 * 이 함수가 정상적으로 동작하려면 딜레이가 없는 while문 안에 이 함수를 집어넣어야 한다.
	 * Custom_Switch_Read 함수는 내부적으로 1ms의 딜레이를 만들기 때문에 이 함수를 주행 알고리즘 내부에 집어넣거나 하면 성능이 크게 떨어진다.
	 * 이 함수를 사용하지 않고 바로 GPIO를 읽을 경우 bouncing이라는 문제가 발생한다. 버튼이 눌리거나 떼질 때 버튼이 수 밀리세컨드 내에 On-Off를 수십번 이상 반복하는 현상이다.
	 * 또한 두 버튼을 동시에 누를 때, 한쪽이 수십 마이크로초라도 일찍 눌러진다면 두 버튼을 순차적으로 누른 것으로 인식할 것이다.
	 * 이 함수는 그런 문제를 해결해준다. 함수를 클릭한 후 F3, 혹은 F12키를 누르면 함수 내부를 볼 수 있다.
	 * 주석으로 어떤 알고리즘을 사용했는지 적어 두었으므로 궁금한 사람은 읽어 보면 좋을 것이다.
	 */

	int8_t number = 0;

	while (true) {
		Custom_LCD_Clear();
		switch(number){
			case 0:
				Custom_LCD_Printf(" SENSOR/1 CALI");
				break;
			case 1:
				Custom_LCD_Printf(" SENSOR/1 NORMAL");
				break;
			case 2:
				Custom_LCD_Printf(" SENSOR/1 STATE");
				break;
			case 3:
				Custom_LCD_Printf(" SENSOR/1POSITION");
				break;
			case 4:
				Custom_LCD_Printf(" MOTOR/1 TEST");
				break;
			case 5:
				Custom_LCD_Printf(" DRIVE/1 FIRST");
				break;
			case 6:
				Custom_LCD_Printf(" DRIVE/1 SECOND");
				break;
		}
		//Custom_LCD_Printf("%d", number);

		uint8_t sw;
		while (!(sw = Custom_Switch_Read()));

		if (sw == CUSTOM_SW_1) number--;
		if (sw == CUSTOM_SW_2) number++;
		if (number > 6) number = 0;
		if (number < 0) number = 6;
		if (sw == CUSTOM_SW_BOTH) {
			switch(number){
				case 0:

					break;
				case 1:

					break;
				case 2:
					Custom_LCD_Clear();
					Sensor_RAW();
					break;
				case 3:

					break;
				case 4:

					break;
				case 5:

					break;
				case 6:

					break;
			}

			}
	}
}
