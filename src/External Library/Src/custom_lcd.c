/*
 * motor.h
 *
 *  Created on: Apr 22, 2021
 *      Author: Joonho Gwon
 *
 * Based on Mohamed Yaqoob's LCD 16X2 library. (04/12/2017)
 */

#include "custom_lcd.h"
#include "custom_gpio.h"
#include <stdarg.h>

#define WRITE_TIME 10

//================================
//	Private Functions
//================================

static void Custom_LCD_RS(bool state) {
	if (state) Custom_GPIO_Set(LCD_RS_GPIO_Port, LCD_RS_Pin, 1);
	else Custom_GPIO_Set(LCD_RS_GPIO_Port, LCD_RS_Pin, 0);
}

static void Custom_LCD_EnablePulse(void) {
	Custom_GPIO_Set(LCD_E1_GPIO_Port, LCD_E1_Pin, 1);
	Custom_Delay_us(WRITE_TIME);
	Custom_GPIO_Set(LCD_E1_GPIO_Port, LCD_E1_Pin, 0);
	Custom_Delay_us(60);
}

static void Custom_LCD_Write(uint8_t byte) {
	uint8_t LSB_nibble = byte & 0x0f, MSB_nibble = (byte >> 4) & 0x0f;

	//write data to output pins
	//MSB data
	__disable_irq();
	Custom_GPIO_Set(LCD_D4_GPIO_Port, LCD_D4_Pin, MSB_nibble & 0x1);
	Custom_GPIO_Set(LCD_D5_GPIO_Port, LCD_D5_Pin, MSB_nibble & 0x2);
	Custom_GPIO_Set(LCD_D6_GPIO_Port, LCD_D6_Pin, MSB_nibble & 0x4);
	Custom_GPIO_Set(LCD_D7_GPIO_Port, LCD_D7_Pin, MSB_nibble & 0x8);
	//Write the Enable pulse
	Custom_LCD_EnablePulse();

	//LSB data
	Custom_GPIO_Set(LCD_D4_GPIO_Port, LCD_D4_Pin, LSB_nibble & 0x1);
	Custom_GPIO_Set(LCD_D5_GPIO_Port, LCD_D5_Pin, LSB_nibble & 0x2);
	Custom_GPIO_Set(LCD_D6_GPIO_Port, LCD_D6_Pin, LSB_nibble & 0x4);
	Custom_GPIO_Set(LCD_D7_GPIO_Port, LCD_D7_Pin, LSB_nibble & 0x8);
	//Write the Enable pulse
	Custom_LCD_EnablePulse();
	__enable_irq();
}

static void Custom_LCD_WriteCommand(uint8_t command) {
	//Set RS to 0
	Custom_LCD_RS(false);
	//Call low level write parallel function
	Custom_LCD_Write(command);
}

static void Custom_LCD_WriteCommand4bit(uint8_t nibble) {
	uint8_t LSB_nibble = nibble & 0x0f;
	//Set RS to 0
	__disable_irq();
	Custom_LCD_RS(false);
	//LSB data
	Custom_GPIO_Set(LCD_D4_GPIO_Port, LCD_D4_Pin, LSB_nibble & 0x1);
	Custom_GPIO_Set(LCD_D5_GPIO_Port, LCD_D5_Pin, LSB_nibble & 0x2);
	Custom_GPIO_Set(LCD_D6_GPIO_Port, LCD_D6_Pin, LSB_nibble & 0x4);
	Custom_GPIO_Set(LCD_D7_GPIO_Port, LCD_D7_Pin, LSB_nibble & 0x8);
	//Write the Enable pulse
	Custom_LCD_EnablePulse();
	__enable_irq();
}

static void Custom_LCD_WriteData(uint8_t data) {
	//Set RS to 1
	Custom_LCD_RS(true);
	//Call low level write parallel function
	Custom_LCD_Write(data);
}

//================================
//	Public functions
//================================

void Custom_LCD_Init() {
	//Initialize LCD

	//1. Wait at least 15ms
	Custom_Delay_ms(20);

	//2. Attentions sequence
	Custom_LCD_WriteCommand4bit(0x3);
	Custom_Delay_ms(5);
	Custom_LCD_WriteCommand4bit(0x3);
	Custom_Delay_ms(1);
	Custom_LCD_WriteCommand4bit(0x3);
	Custom_Delay_ms(1);
	Custom_LCD_WriteCommand4bit(0x2); //4 bit mode
	Custom_Delay_ms(1);

	//3. Display control (Display ON)
	Custom_LCD_WriteCommand(LCD_DISPLAY_CONTROL | LCD_DISPLAY_D);

	//4. Clear LCD and return home
	Custom_LCD_WriteCommand(LCD_CLEAR_DISPLAY);
	Custom_Delay_ms(3);

	//5. Function set; Enable 2 lines, Data length to 8 bits
	Custom_LCD_WriteCommand(LCD_FUNCTION_SET | LCD_FUNCTION_N);
	Custom_Delay_ms(3);
}

void Custom_LCD_SetCursor(uint8_t row, uint8_t col) {
	uint8_t maskData = 0x80 | (!!row << 6) | (col & 0x0f);
	Custom_LCD_WriteCommand(maskData);
}

void Custom_LCD_Clear(void) {
	Custom_LCD_WriteCommand(LCD_CLEAR_DISPLAY);
	Custom_Delay_ms(3);
}

void Custom_LCD_ShiftToRight(uint8_t num) {
	for (uint8_t i = 0; i < num; i++) {
		Custom_LCD_WriteCommand(0x1c);
	}
}

void Custom_LCD_ShiftToLeft(uint8_t num) {
	for (uint8_t i = 0; i < num; i++) {
		Custom_LCD_WriteCommand(0x18);
	}
}

void Custom_LCD_Printf(const char *format, ...) {
	/*
	 * buffer??? ?????? ??????????????? static?????? ???????????? ????????????, ?????? ??????????????? 4GB??? ???????????? ????????? ?????? ???????????? ?????????
	 * STM32F411??? ?????? ?????? ????????? ????????? ?????? 128kB??? ??????????????? ???????????? ????????? ????????? ?????? ?????? ??????.
	 */
	char buffer[32] = { 0 };

	// ?????? ????????? ????????? buffer??? ??????.
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	// ????????? ????????? /0, /1??? ????????? ?????? ????????? ??????????????? ??????.
	for (char *c = buffer; *c; c++) {
		if (*c == '/' && (*(c + 1) == '0' || *(c + 1) == '1')) {
			Custom_LCD_SetCursor(*(c + 1) - '0', 0);
			c += 1;
			continue;
		}

		// \n, \r ?????? ????????? ??????????????? ????????????.
		if (*c == '\r') continue;
		if (*c == '\n') continue;
		Custom_LCD_WriteData(*c);
	}
}

void Custom_LCD_Backlight(bool turnOn) {
	Custom_GPIO_Set(LCD_Backlight_GPIO_Port, LCD_Backlight_Pin, turnOn);
}
