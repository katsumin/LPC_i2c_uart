/**********************************************************************
* $Id$		lpc8xx_i2c.c		2011-06-02
*//**
* @file		lpc8xx_i2c.c
* @brief	Contains all functions support for I2C firmware library
* 			on lpc8xx
* @version	1.0
* @date		02. June. 2011
* @author	NXP MCU SW Application Team
*
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors�
* relevant copyright in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
**********************************************************************/

/* Includes ------------------------------------------------------------------- */
#include "LPC8xx.h"
#include "lpc8xx_nmi.h"
#include "lpc8xx_i2c.h"

typedef struct {
	uint8_t txBuffer[I2C_BUFSIZE];
	uint8_t rxBuffer[I2C_BUFSIZE];
	uint32_t txReadPointer;
	uint32_t txWritePointer;
	uint32_t rxReadPointer;
	uint32_t rxWritePointer;
}I2C_CONTEXT;

volatile I2C_CONTEXT i2cContext[2]; // 0:コマンド、1:データ

volatile uint32_t ch = 0;
volatile uint32_t I2CStatus;

volatile uint32_t I2CInterruptCount = 0;
volatile uint32_t I2CMstRXCount = 0;
volatile uint32_t I2CMstTXCount = 0;
volatile uint32_t mstidle = 0;

/* Master related */
volatile uint32_t I2CMstIdleCount = 0;
volatile uint32_t I2CMstNACKAddrCount = 0;
volatile uint32_t I2CMstNACKTXCount = 0;
volatile uint32_t I2CARBLossCount = 0;
volatile uint32_t I2CMstSSErrCount = 0;

/* Slave related */
//volatile uint32_t I2CSlvSelectedCount = 0;
//volatile uint32_t I2CSlvDeselectedCount = 0;
//volatile uint32_t I2CSlvNotStrCount = 0;

/* Monitor related */
//volatile uint32_t I2CMonIdleCount = 0;
//volatile uint32_t I2CMonOverrunCount = 0;

#if TIMEOUT_ENABLED
/*****************************************************************************
** Function name:		I2C_I2CTimeoutStatus
**
** Descriptions:		I2C interrupt handler to handle timeout status
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_I2CTimeoutStatus( LPC_I2C_TypeDef *I2Cx, uint32_t active )
{
  if(active & STAT_SCLTIMEOUT) {
		I2CSCLTimeoutCount++;
		I2Cx->STAT = STAT_SCLTIMEOUT;
  }
  if(active & STAT_EVTIMEOUT) {
		I2CEventTimeoutCount++;
		I2Cx->STAT = STAT_EVTIMEOUT;
  }
}
#endif

/*****************************************************************************
** Function name:		I2C_MasterStatus
**
** Descriptions:		I2C interrupt handler to handle master status
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_MasterStatus( LPC_I2C_TypeDef *I2Cx, uint32_t active )
{
  if (active & STAT_MSTARBLOSS) {
		I2CARBLossCount++;
		I2CStatus |= STAT_MSTARBLOSS;
		I2Cx->STAT = STAT_MSTARBLOSS;
  }
  if (active & STAT_MSTSSERR) {
		I2CMstSSErrCount++;
		I2CStatus |= STAT_MSTSSERR;
		I2Cx->STAT = STAT_MSTSSERR;
  }
  return;
}

/*****************************************************************************
** Function name:		I2C_SlaveStatus
**
** Descriptions:		I2C interrupt handler to handle slave status
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_SlaveStatus( LPC_I2C_TypeDef *I2Cx, uint32_t active )
{
  if (active & STAT_SLVNOTSTR) {
		I2Cx->INTENCLR = STAT_SLVNOTSTR;
  }
  if (active & STAT_SLVDESEL) {
		I2Cx->STAT = STAT_SLVDESEL;
  }
  return;
}

#if I2C_MONITOR_MODE
/*****************************************************************************
** Function name:		I2C_MonitorStatus
**
** Descriptions:		I2C interrupt handler to handle monitor status
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_MonitorStatus( LPC_I2C_TypeDef *I2Cx, uint32_t active )
{
  if (active & STAT_MONIDLE) {
		I2CMonIdleCount++;
		I2Cx->STAT = STAT_MONIDLE;
  }
  if (active & STAT_MONOVERRUN) {
		I2CMonOverrunCount++;
		I2Cx->STAT = STAT_MONOVERRUN;
  }
  return;
}
#endif

/*****************************************************************************
** Function name:		I2C_IRQHandler
**
** Descriptions:		I2C interrupt handler
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_IRQHandler(void)
{
  uint32_t active = LPC_I2C->INTSTAT;
  uint32_t i2cmststate = LPC_I2C->STAT & MASTER_STATE_MASK;	/* Only check Master and Slave State */
  uint32_t i2cslvstate = LPC_I2C->STAT & SLAVE_STATE_MASK;

  if ( active ) {
		I2CInterruptCount++;
  }

#if I2C_MONITOR_MODE
	I2C_MonitorStatus(LPC_I2C, active);
#endif	

  /* The error interrupts should have higher priority too before data can be 
  processed. */  
#if TIMEOUT_ENABLED
  /* I2C timeout related interrupts. */
  I2C_I2CTimeoutStatus(LPC_I2C, active);
#endif
  
  /* master related interrupts. */
  I2C_MasterStatus(LPC_I2C, active);

  if ( active & STAT_MSTPEND )
  {
		LPC_I2C->INTENCLR = STAT_MSTPEND;
		/* Below five states are for Master mode: IDLE, TX, RX, NACK_ADDR, NAC_TX.
		IDLE is not under consideration for now. */
		switch ( i2cmststate )
		{
			case STAT_MSTIDLE:
			default:
				mstidle = 1;
			break;

			case STAT_MSTRX:
			break;

			case STAT_MSTTX:
			break;

			case STAT_MSTNACKADDR:
			break;

			case STAT_MSTNACKTX:
			break;
		}
  }
     
  /* slave related interrupts. */
  I2C_SlaveStatus(LPC_I2C, active);

  if ( active & STAT_SLVPEND )
  {
		LPC_I2C->INTENCLR = STAT_SLVPEND;
		/* Below three states are for Slave mode: Address Received, TX, and RX. */
		switch ( i2cslvstate )
		{
			case STAT_SLVRX: // ラズパイから、コマンド（ch=0）とUART送信データ（ch=1）を受信する
				I2C_SetSlvRxData(ch, LPC_I2C->SLVDAT);
				break;
			case STAT_SLVTX: // ラズパイに、受信データ数（ch=0）とUART受信データ（ch=1）を送信する
				if (ch == 0) {
					// ch=0の場合、I2C送信バッファサイズを、I2Cポートに渡す
					LPC_I2C->SLVDAT = I2C_GetSlvTxCount(1);
				} else {
					// ch=1の場合、I2C送信バッファから取り出して、I2Cポートに渡す
					LPC_I2C->SLVDAT = I2C_GetSlvTxData(ch);
				}
				break;
			case STAT_SLVADDR:
				/* slave address is received. */
				/* slaveaddrrcvd, slavetxrdy, and slaverxrdy are not used anymore
				as slave tx and rx are handled inside the ISR now. 
				I2C_SlaveSendData(), I2C_SlaveReceiveData(), I2C_SlaveReceiveSend()
				are for polling mode only. */
				ch = LPC_I2C->SLVDAT;
				ch = ch >> 1;
				ch &= 0x1;
				break;
			default:
				break;
		}
		LPC_I2C->SLVCTL = CTL_SLVCONTINUE;
		LPC_I2C->INTENSET = STAT_SLVPEND;
  }
  return;
}

/**
 * I2C送信バッファに、1データを書き込む。
 */
void I2C_SetSlvTxData(int ch, int data) {
	int count = I2C_GetSlvTxCount(ch);
	if ( count < I2C_BUFSIZE ) {
		i2cContext[ch].txBuffer[i2cContext[ch].txWritePointer++] = data;
		if ( i2cContext[ch].txWritePointer >= I2C_BUFSIZE ) {
			i2cContext[ch].txWritePointer = 0;
		}
	}
}

/**
 * I2C送信バッファから、1データを取り出す。
 */
int I2C_GetSlvTxData(int ch) {
	int data = -1;
	int count = I2C_GetSlvTxCount(ch);
	if ( count > 0 ){
		data = i2cContext[ch].txBuffer[i2cContext[ch].txReadPointer++];
		if ( i2cContext[ch].txReadPointer >= I2C_BUFSIZE ) {
			i2cContext[ch].txReadPointer = 0;
		}
	}
	return data;
}

/**
 * I2C送信バッファに書き込まれたサイズを返す。
 */
int I2C_GetSlvTxCount(int ch){
	int count = i2cContext[ch].txWritePointer - i2cContext[ch].txReadPointer;
	if ( count < 0 ) {
		count += I2C_BUFSIZE;
	}
	return count;
}

/**
 * I2C受信バッファに、1データを書き込む。
 */
void I2C_SetSlvRxData(int ch, int data) {
	int count = I2C_GetSlvRxCount(ch);
	if ( count < I2C_BUFSIZE ) {
		i2cContext[ch].rxBuffer[i2cContext[ch].rxWritePointer++] = data;
		if ( i2cContext[ch].rxWritePointer >= I2C_BUFSIZE ) {
			i2cContext[ch].rxWritePointer = 0;
		}
	}
}

/**
 * I2C受信バッファから、1データを取り出す。
 */
int I2C_GetSlvRxData(int ch){
	int data = -1;
	int count = I2C_GetSlvRxCount(ch);
	if ( count > 0 ) {
		data = i2cContext[ch].rxBuffer[i2cContext[ch].rxReadPointer++];
		if ( i2cContext[ch].rxReadPointer >= I2C_BUFSIZE ) {
			i2cContext[ch].rxReadPointer = 0;
		}
	}
	return data;
}

/**
 * I2C受信バッファに書き込まれたサイズを返す。
 */
int I2C_GetSlvRxCount(int ch){
	int count = i2cContext[ch].rxWritePointer - i2cContext[ch].rxReadPointer;
	if ( count < 0 ) {
		count += I2C_BUFSIZE;
	}
	return count;
}

/*****************************************************************************
** Function name:		I2C_MstInit
**
** Descriptions:		I2C port initialization routine
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_MstInit( LPC_I2C_TypeDef *I2Cx, uint32_t div, uint32_t cfg, uint32_t dutycycle )
{
	/* For master mode plus, if desired I2C clock is 1MHz (SCL high time + SCL low time). 
	If CCLK is 36MHz, MasterSclLow and MasterSclHigh are 0s, 
	SCL high time = (ClkDiv+1) * (MstSclHigh + 2 )
	SCL low time = (ClkDiv+1) * (MstSclLow + 2 )
	Pre-divider should be 8. 
	If fast mode, e.g. communicating with a temp sensor, Max I2C clock is set to 400KHz.
	Pre-divider should be 11. */
  I2Cx->DIV = div;
  I2Cx->CFG &= ~(CFG_MSTENA);

  I2Cx->MSTTIME = TIM_MSTSCLLOW(dutycycle) | TIM_MSTSCLHIGH(dutycycle);
#if I2C_INTERRUPT
  I2Cx->INTENSET |= (STAT_MSTARBLOSS | STAT_MSTSSERR);
  NVIC_DisableIRQ(I2C_IRQn);
  NVIC_ClearPendingIRQ(I2C_IRQn);
#if NMI_ENABLED
	NMI_Init( I2C_IRQn );
#else
  NVIC_EnableIRQ(I2C_IRQn);
#endif
#endif
  I2Cx->CFG |= cfg;
  return;
}

/*****************************************************************************
** Function name:		I2C_SlvInit
**
** Descriptions:		I2C Slave mode initialization routine
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_SlvInit( LPC_I2C_TypeDef *I2Cx, uint32_t addr, uint32_t cfg, uint32_t clkdiv )
{
  I2Cx->CFG &= ~(CFG_SLVENA);

  /* For master mode plus, if desired I2C clock is 1MHz (SCL high time + SCL low time). 
	If CCLK is 36MHz, MasterSclLow and MasterSclHigh are 0s, 
	SCL high time = (ClkDiv+1) * (MstSclHigh + 2 )
	SCL low time = (ClkDiv+1) * (MstSclLow + 2 )
	Pre-divider should be 8. 
	If fast mode, e.g. communicating with a temp sensor, Max I2C clock is set to 400KHz.
	Pre-divider should be 11. */
  I2Cx->DIV = clkdiv;

  /* Enable all addresses */
  I2Cx->SLVADR0 = addr;
  I2Cx->SLVADR1 = addr + 0x02;
//  I2Cx->SLVADR2 = addr + 0x04;
//  I2Cx->SLVADR3 = addr + 0x06;

#if ADDR_QUAL_ENABLE
  /* RANGE (bit0 = 1) or MASK(bit0 = 0) mode */
#if 1
  /* RANGE (bit0 = 1) mode, (SLVADR0 <= addr <= SLVQUAL0) */
  I2Cx->SLVQUAL0 = ((LPC_I2C->SLVADR0 + 0x60) | 0x01);
#else
  /* MASK (bit0 = 0) mode, (addr & ~SLVQUAL0) = SLVADR0 */
  I2Cx->SLVQUAL0 = 0x06;
//  I2Cx->SLVQUAL0 = 0xfe;
#endif
#endif
  
#if I2C_INTERRUPT
  I2Cx->INTENSET |= (STAT_SLVDESEL | STAT_SLVNOTSTR);
  NVIC_DisableIRQ(I2C_IRQn);
  NVIC_ClearPendingIRQ(I2C_IRQn);
  NVIC_EnableIRQ(I2C_IRQn);
#endif
  I2Cx->CFG |= cfg;
  return;
}

#if I2C_MONITOR_MODE
/*****************************************************************************
** Function name:		I2C_MonInit
**
** Descriptions:		I2C Monitor mode initialization routine
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_MonInit( LPC_I2C_TypeDef *I2Cx, uint32_t cfg )
{
  I2Cx->CFG &= ~CFG_MONENA;
#if I2C_INTERRUPT
  I2Cx->INTENSET = STAT_MONRDY | STAT_MONOVERRUN | STAT_MONIDLE;
  NVIC_DisableIRQ(I2C_IRQn);
  NVIC_ClearPendingIRQ(I2C_IRQn);
  NVIC_EnableIRQ(I2C_IRQn);
#endif
  I2Cx->CFG |= cfg;
}
#endif

#if TIMEOUT_ENABLED
/*****************************************************************************
** Function name:		I2C_TimeoutInit
**
** Descriptions:		I2C Timeout initialization routine
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C_TimeoutInit( LPC_I2C_TypeDef *I2Cx, uint32_t timeout_value )
{
  uint32_t to_value;
  
  I2Cx->CFG &= ~CFG_TIMEOUTENA;
  to_value = I2Cx->TIMEOUT & 0x000F;
  to_value |= (timeout_value << 4);
  I2Cx->TIMEOUT = to_value;  
#if I2C_INTERRUPT
  I2Cx->INTENSET |= (STAT_EVTIMEOUT|STAT_SCLTIMEOUT); 
  
  NVIC_DisableIRQ(I2C_IRQn);
  NVIC_ClearPendingIRQ(I2C_IRQn);
  NVIC_EnableIRQ(I2C_IRQn);
#endif
  I2Cx->CFG |= CFG_TIMEOUTENA;
}
#endif

/*****************************************************************************
** Function name:		I2C_CheckIdle
**
** Descriptions:		Check idle, if the master is NOT idle, it shouldn't 
**									start the transfer.		
** parameters:			None
** Returned value:		None, if NOT IDLE, spin here forever.
** 
*****************************************************************************/
void I2C_CheckIdle( LPC_I2C_TypeDef *I2Cx )
{
#if I2C_INTERRUPT
  I2Cx->INTENSET = STAT_MSTPEND;
  while(!mstidle);
  mstidle = 0;
#else
  /* Once the master is enabled, pending bit should be set, as the state should be in IDLE mode. */
  /* Pending, but not idle, spin here forever. */
  while ( (I2Cx->STAT & (STAT_MSTPEND|MASTER_STATE_MASK)) != (STAT_MSTPEND|STAT_MSTIDLE) );
#endif
  return;
}

#if 0
/*****************************************************************************
** Function name:		I2C_SlaveSend
**
** Descriptions:		Send a block of data to the I2C port, the 
**									first parameter is the buffer pointer, the 2nd 
**									parameter is the block length.
**
** parameters:			buffer pointer, and the block length
** Returned value:	None
** 
*****************************************************************************/
void I2C_SlaveSendData( LPC_I2C_TypeDef *I2Cx, uint8_t *tx, uint32_t Length )
{
  uint32_t i;

  I2Cx->SLVCTL = CTL_SLVCONTINUE;
#if I2C_INTERRUPT
  I2Cx->INTENSET = STAT_SLVPEND;
#endif

  for ( i = 0; i < Length; i++ ) 
  {
#if I2C_INTERRUPT
		while(!slvtxrdy);
		slvtxrdy = 0;
		I2Cx->SLVDAT = *tx++;
		I2Cx->SLVCTL = CTL_SLVCONTINUE;
		I2Cx->INTENSET = STAT_SLVPEND; 
#else
		/* Move only if TX is ready */
		while ( (I2Cx->STAT & (STAT_SLVPEND|SLAVE_STATE_MASK)) != (STAT_SLVPEND|STAT_SLVTX) );
		I2Cx->SLVDAT = *tx++;
		I2Cx->SLVCTL = CTL_SLVCONTINUE;
#endif
  }
  return; 
}
#endif

#if 0
/*****************************************************************************
** Function name:		I2C_SlaveReceive
** Descriptions:		the module will receive a block of data from 
**						the I2C, the 2nd parameter is the block 
**						length.
** parameters:			buffer pointer, and block length
** Returned value:		None
** 
*****************************************************************************/
void I2C_SlaveReceiveData( LPC_I2C_TypeDef *I2Cx, uint8_t *rx, uint32_t Length )
{
  uint32_t i;

  I2Cx->SLVCTL = CTL_SLVCONTINUE;
#if I2C_INTERRUPT
  I2Cx->INTENSET = STAT_SLVPEND;
#endif

  for ( i = 0; i < Length; i++ )
  {
#if I2C_INTERRUPT
		while(!slvrxrdy);
		slvrxrdy = 0;
		*rx++ = I2Cx->SLVDAT;
		if ( i != Length-1 ) {
			I2Cx->SLVCTL = CTL_SLVCONTINUE;
		}
		else {
			// I2Cx->SLVCTL = CTL_SLVNACK | CTL_SLVCONTINUE;
			I2Cx->SLVCTL = CTL_SLVCONTINUE;
		}
		I2Cx->INTENSET = STAT_SLVPEND; 
#else	
		while ( (I2Cx->STAT & (STAT_SLVPEND|SLAVE_STATE_MASK)) != (STAT_SLVPEND|STAT_SLVRX) );
		*rx++ = I2Cx->SLVDAT;
		if ( i != Length-1 ) {
			I2Cx->SLVCTL = CTL_SLVCONTINUE;
		}
		else {
			// I2Cx->SLVCTL = CTL_SLVNACK | CTL_SLVCONTINUE; 
			I2Cx->SLVCTL = CTL_SLVCONTINUE; 
		}
#endif
  }
  return; 
}
#endif

#if 0
/*****************************************************************************
** Function name:		I2C_SlaveReceiveSend
** Descriptions:		Based on bit 0 of slave address, this module will
**						send or receive block of data to/from the master.
**						The length of TX and RX are the same. Two buffers,
**						one for TX and one for RX, are used.
** parameters:			tx buffer ptr, rx buffer ptr, and block length
** Returned value:		None
** 
*****************************************************************************/
void I2C_SlaveReceiveSend( LPC_I2C_TypeDef *I2Cx, uint8_t *tx, uint8_t *rx, uint32_t Length )
{
  uint32_t addr7bit;
  
    LPC_I2C->SLVCTL = CTL_SLVCONTINUE;
#if I2C_INTERRUPT
  I2Cx->INTENSET = STAT_SLVPEND;
  while(!slvaddrrcvd);
  slvaddrrcvd = 0;
#else
  while ( (I2Cx->STAT & (STAT_SLVPEND|SLAVE_STATE_MASK)) != (STAT_SLVPEND|STAT_SLVADDR) );
#endif
  SlaveAddr = I2Cx->SLVDAT;
  addr7bit = SlaveAddr & 0xFE;
#if ADDR_QUAL_ENABLE
  if ( I2Cx->SLVQUAL0 & 0xFF )
  {
		/* Either MASK or RANGE */
		if ( I2Cx->SLVQUAL0 & 0x01 ) {
			/* RANGE mode (SLVQUAL0 >= addr >= SLVADR0) */
			if ( (addr7bit < I2Cx->SLVADR0) || (addr7bit > (I2Cx->SLVQUAL0 & 0xFE)) ) {
				/* For debugging. That shouldn't happen. */
				while ( 1 );
			}
		}		
		else {
#if 0
			/* MASK mode */
			if ( (addr7bit & (I2Cx->SLVQUAL0&0xFE)) != I2Cx->SLVADR0 ) {
				/* For debugging. That shouldn't happen. */
				while ( 1 );
			}
#endif
		}		
  }
#else
  if ( ( addr7bit != I2Cx->SLVADR0 ) && ( addr7bit != I2Cx->SLVADR1 )
		&& ( addr7bit != I2Cx->SLVADR2 ) && ( addr7bit != I2Cx->SLVADR3 ) ) {
		/* For debugging. It should never happen, SLDADDR bit changes but addr doesn't match. */
		while ( 1 );
  }
#endif
   
  if ( (SlaveAddr & RD_BIT) == 0x00 ) {
		/* slave reads from master. */
		I2C_SlaveReceiveData( I2Cx, rx, Length ); 
  }
  else {
		/* slave write to master. */
		I2C_SlaveSendData( I2Cx, tx, Length ); 
  }
  return; 
}
#endif
//#endif /* _I2C */


/* --------------------------------- End Of File ------------------------------ */
