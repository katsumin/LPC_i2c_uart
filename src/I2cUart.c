/*
===============================================================================
 Name        : I2cUart.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC8xx.h"
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here
#include "lpc8xx_i2c.h"
#include "lpc8xx_clkconfig.h"
#include "lpc8xx_uart.h"

// TODO: insert other definitions and declarations here
extern void SwitchMatrix_Init();
extern volatile uint32_t UARTRxCount;
extern volatile uint8_t UARTRxBuffer[BUFSIZE];

int main(void) {
    // TODO: insert code here
    SystemCoreClockUpdate();
	SwitchMatrix_Init();

	/* Enable I2C clock */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<5);
	/* Toggle peripheral reset control to I2C, a "1" bring it out of reset. */
	LPC_SYSCON->PRESETCTRL &= ~(0x1<<6);
	LPC_SYSCON->PRESETCTRL |= (0x1<<6);

	I2C_SlvInit(LPC_I2C, SLAVE_ADDR, CFG_SLVENA, I2C_FMODE_PRE_DIV);
	/* Ready to receive the slave address, interrupt(SLAVE ADDR STATE) if match occurs. */
    LPC_I2C->SLVCTL = CTL_SLVCONTINUE;
#if I2C_INTERRUPT
    LPC_I2C->INTENSET = STAT_SLVPEND;
#endif
    I2C_MstInit(LPC_I2C, I2C_FMODE_PRE_DIV, CFG_MSTENA, 0x00);

    /* Get device ID register */
    I2C_CheckIdle(LPC_I2C);

    // Enter an infinite loop, just incrementing a counter
    int baud[] = {300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    int settingLen = sizeof(baud) / sizeof(int);
    while ( 1 ) {
    	// I2Cコマンド受信バッファから取り出し、UART初期化
        int count = I2C_GetSlvRxCount(0);
        if ( count > 0 ) {
            int dat = I2C_GetSlvRxData(0);
            if ( dat < settingLen ) {
        		UARTInit(LPC_USART0, baud[dat]);
            }
        }
        // I2C受信バッファから取り出し、UART送信バッファに書き込む
        count = I2C_GetSlvRxCount(1);
        if ( count > 0 ) {
        	int data = I2C_GetSlvRxData(1);
        	UART_SetTxData(0, data);
        }
        // UART受信バッファから取り出し、I2C送信バッファに書き込む
        count = UART_GetRxCount(0);
        if ( count > 0 ) {
        	int data = UART_GetRxData(0);
        	I2C_SetSlvTxData( 1, data);
        }
    }
    return 0 ;
}

