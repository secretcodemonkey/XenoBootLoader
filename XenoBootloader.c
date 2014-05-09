/****************************************************************************************************/
/*                                                                                                  */
/*  X    X                                                                                          */
/*   X  X                                                                                           */
/*    XX ENOMORPH                                                                                   */
/*    XX ENOMORPH                                                                                   */
/*   X  X                                                                                           */
/*  X    X                    VERSION: Bootloader - C8051F381                                       */
/*                                                                                                  */
/****************************************************************************************************/

#include "compiler_defs.h"
#include "C8051f380_defs.h"

#define UINT8	unsigned char
#define UINT16	unsigned int
#define UINT32	unsigned long
#define SINT8	char
#define SINT16	int
#define SINT32	long

#define DISABLE_WDT()      PCA0MD &= ~0x40;
#define FL_PAGE_SIZE       512
#define LOCK_PAGE          0xFA00
#define FLASH_SAFE_ADDR    0xFFFF
#define ROMPAGELEN         0x1000
#define ROMPAGE1START      0xC000
#define ROMPAGE2START      0xD000
#define ROMPAGE1COUNT 	   0xCDFE
#define ROMPAGE2COUNT 	   0xDDFE
#define ROMPAGE1VALID 	   0xCDFC
#define ROMPAGE2VALID 	   0xDDFC
#define ROMPAGESLEN        0x2000
#define ROMVALIDMARK       0xF9CE 
#define LASTBYTEOFAPP	   0xF9FF					
#define LASTPAGEOFAPP	   0xF800
#define HAL_MAXBATTERYFORBURN	50
#define HAL_MAXSENSORFORBURN	30
#define DOWNLOADCOMMSTIMEOUT	1000000


/*                                       0123456789012345678901234567890123456789*/
const char __at 0x0F90 XenoRevNum[20] = "Xeno Bootloader v2 ";
const char __at 0x0FB0 XenoSpecific[16] = "B064 24/03/2013";
const char __at 0x0FC0 XenoSign[32]   = "Xenomorph Bootloader Version 2 ";
#define ERASECOMPLETELEN		33
const char EraseComplete[ERASECOMPLETELEN] = "Erase Complete - Please Program";
#define PROGCOMPLETELEN			37
const char ProgComplete[PROGCOMPLETELEN] = {0x0a,0x0d,'P','r','o','g','r','a','m',' ','C','o','m','p','l','e','t','e',' ','-',' ','R',' ','t','o',' ','R','e','s','t','a','r','t',' ',' ',' '};
#define NEWLINECURSORLEN  3
const char NewlineCursor[NEWLINECURSORLEN] = {0x0a,0x0d, 0x3E};
#define WELCOMETEXTLEN    30
const char WelcomeText[WELCOMETEXTLEN] = {0x0a,0x0d,0x0a,0x0d,0x0a,0x0d, 'X','B','L',' ','(','c',')','2','0','1','2',' ','C','o','d','e','m','o','n','k','e','y',0x0a,0x0d};
#define COMMSLOSTLEN      31
const char CommsLost[COMMSLOSTLEN] = "Comms lost!! - Erase and retry";




/* Bootloader requirements *************************/
UINT32 __at 0xF9F0	BLSoftwareVersion;
UINT32 __at 0xF9F4	BLParameterVersion;
UINT16 __at 0xF9F8	BLChecksum;
UINT8 __at 0xC000 Rompages;
UINT8 __at 0xff	BLSemaphore;
#define BLSEMDOWNLOAD	0x5A
#define BLVALIDATOR		0xF9FA
#define BLVALID			0x13
/* Bootloader requirements *************************/



#define RSTSRC_VAL      0x02        // Set to enable desired reset sources

#define FUEL_MAP_PTS 			16
#define SPARK_MAP_PTS 			16
#define FUEL_RPM_PTS 			16
#define SPARK_RPM_PTS 			16
#define TURBO_DUTY_RPM_PTS		8
#define TURBO_DUTY_TPS_PTS		8
#define TURBO_TARG_RPM_PTS		8
#define TURBO_TARG_TPS_PTS		8

#define FLASH_FOUR_PAGE_ADD			0xF800
#define FLASH_BLOCK_WRITE			0x01
#define FLASH_BLOCK_ERASE			0x03
void Port_Init(void);
void UART_Init(void);
UINT8 HexToBin(UINT8 * Data);
UINT8 BinToHex (UINT8 __code * TxPointer, UINT8 pos);
void EraseAppFlash(void);
void ProgramByte(UINT16 flashAddress, UINT8 Data);
void ProgramPage(UINT16 flashAddress, UINT8 * Data);


SBIT(Led1, SFR_P2, 2); 


#define FPCA0						500000L
#define FTIMER1						500000L
#define FTIMER3						2000000L
#define FSYS                        48000000L
#define FBAUD                       115200L
#define HAL_MINIMUMTIMERLATENCY		20
#define NULL						(UINT16 *)0


#define INJECTOR_OFF			P1 &= 0xF7

#define SPARK_OFF               P1 |= 0x10


UINT8 txmode;
UINT8 kill_ser;
UINT8 tble_idx;
UINT8 vfy_flg;
UINT8 vfy_fail;
UINT8 rxcnt;
UINT8 reinit_flag;
UINT8 can_id;
UINT16 txcnt;
UINT16 txgoal;
UINT16 kill_ser_t;
UINT16 rxoffset;
UINT16 rxnbytes;
UINT32 rcv_timeout;
UINT32 lmms;
UINT16 ROM1Num;
UINT16 ROM2Num;
UINT16 ROMActivePage;
UINT16 ROMRead;
UINT8 const * ROMBurnPointer;
UINT8 RXData[3];
UINT8 HexPtr[2];
#define MAINCOMMAND		0
#define IDCOMMAND		1
#define DOWNLOADCOMMAND 2
UINT8 CommandLevel;
UINT16 TxCounter; 
UINT8 __code * TxPtr;
UINT16 TxFormat;
UINT8 __code * TxFrPtr;
UINT8 TxType;
UINT32 ComTimeout;
UINT32 ComTimer;


//-----------------------------------------------------------------------------
// Main Routine
//-----------------------------------------------------------------------------
void main(void)
{

   UINT8 ReceiveLocal, rxcount;
	UINT8 ProgData;
	UINT16 ProgAddress;
	UINT16 FlashCounter;

/* Bootloader:- Check to see if code validator is correct, and that bootloader */
/* mode request variable is not set (by download request)                      */
								
	PCA0MD &= ~0x40;                       			/* Disable watchdog 	 */

	if ((*(UINT8 __code *)BLVALIDATOR == BLVALID) && (BLSemaphore != BLSEMDOWNLOAD))

		{
	__asm 
	
	ljmp 0x1000

	__endasm;
		}
	else
		{

		FLSCL     = 0x90;

		CLKSEL    = 0x03;

		OSCICN    = 0xC3;

		Port_Init();
		
		UART_Init();

		rxcount = 0;

		ProgAddress = 0x1000;

		TxType = 0;

		TxCounter = WELCOMETEXTLEN;

		TxPtr = WelcomeText;

		CommandLevel = MAINCOMMAND;

		ComTimeout = 0;

        FlashCounter = 0;

		SCON1 &= ~0x02;   
		
        SBUF1 = *TxPtr;

/* "Q" = Send over Embedded Code Revision Number                                   */
/* "R" = Reboot with no more prompts                                               */
/* "S" = Send program title.                                                       */
/* "U" = Upload parameter set segment                                              */
/* "V" = Validate code after download                                              */
/* "W" = Erase entire application code segment                                     */
/* "X" = Download application code                                                 */
/* "Y" = Download only parameter section                                           */
/* "Z" = Unit build details                                                        */

		while(1)
			{
				if (++FlashCounter >= 15000)
					{
						Led1 = !Led1;
						FlashCounter = 0;
					}
				
				/* Check for Rx Bytes */
				if (SCON1 & 0x01)
					{
							FlashCounter += 250;
		
							ReceiveLocal = SBUF1;				/* Take a local copy of the data */
			
							SCON1 &= ~0x01;						/* Clear receive interrupt flag  */

							ComTimer = 0;						/* Reset timeout */

								switch (CommandLevel)
									{
										case MAINCOMMAND:							/* Level 1 - waiting for command */
												switch (ReceiveLocal)
														{
															case 'R':                /* Reboot          */
															case 'r':
																BLSemaphore = 0x00;
																RSTSRC = 0x10;
																__asm 
																ljmp 0x0000
																__endasm;
															break;

															case 'Q':				/* Return BL ID     */
															case 'q':				/* Return BL ID     */
																TxCounter = 20; 
																TxPtr = XenoRevNum;
															break;

															case 'S':				/* Return BL ID     */
															case 's':				/* Return BL ID     */
																TxCounter = 32; 
																TxPtr = XenoSign;
															break;

															case 'U':				/* Upload paramter set */
															case 'u':				
																TxPtr = (UINT8 __code *)&Rompages;
																TxCounter = ROMPAGESLEN;
																TxType = 1;
															break;

															case 'W':				/* Upload paramter set */
															case 'w':				
																EraseAppFlash();
																TxCounter = ERASECOMPLETELEN; 
																TxPtr = EraseComplete;
															break;

															case 'X':				/* Program start    */
															case 'x':				/* Program start    */
																CommandLevel = DOWNLOADCOMMAND;
																ComTimeout = DOWNLOADCOMMSTIMEOUT;
																rxcount = 0;		
															break;

															case 'Z':
															case 'z':
																TxCounter = 16;		/* Unit data        */
																TxPtr = XenoSpecific;
															break;

															default:
																TxFormat = NEWLINECURSORLEN;
																TxFrPtr = NewlineCursor;
															break;
														}
										break;					
															

										case DOWNLOADCOMMAND:						/* Download command - waiting for data in slots of 2 */

												if (ReceiveLocal == 0x0d)
													{
													}
												else if (rxcount < 1)
													{
														RXData[rxcount] = ReceiveLocal;
														rxcount++;
													}
												else
													{
														RXData[rxcount] = ReceiveLocal;
														ProgData = HexToBin(RXData);
														ProgramByte(ProgAddress, ProgData);
														ProgAddress++;
						
														if ((ProgAddress & 0x01FF) == 0x0000)
															{
																while (!(SCON1 & 0x02));  SCON1 &= ~0x02;   SBUF1 = '-';
															}
																	
														if (ProgAddress >= LASTBYTEOFAPP)
															{
 																ComTimeout = 0;
																TxCounter = PROGCOMPLETELEN; 
																TxPtr = ProgComplete;
																CommandLevel = MAINCOMMAND;
															}

														rxcount = 0;
													}
										break;
									}

					}		
			else
					{
						if (ComTimeout > 0)
							{				/* If timeout has been set...*/
								if (++ComTimer > ComTimeout)
									{
										ComTimeout = 0;
										TxCounter = COMMSLOSTLEN; 
										TxPtr = CommsLost;
										CommandLevel = MAINCOMMAND;
									}
							}
					}
			if (TxCounter > 0)
				{
					if (TxType == 1) /* Decode to text? */
						{							
							while (!(SCON1 & 0x02));  
							SCON1 &= ~0x02;   
							SBUF1 = BinToHex ((__code) TxPtr, 0);
							TxType = 2;
						}
					else if (TxType == 2)
						{
							while (!(SCON1 & 0x02));  
							SCON1 &= ~0x02;   
							SBUF1 = BinToHex ((__code)TxPtr, 1);
							TxPtr++;
							TxType = 1;
							if (--TxCounter == 0)
								{
									TxType = 0;
								}
						}
					else
						{
							while (!(SCON1 & 0x02));  SCON1 &= ~0x02;   SBUF1 = *TxPtr;
							TxPtr++;
							if (--TxCounter == 0)
								{
									TxFormat = NEWLINECURSORLEN;
									TxFrPtr = NewlineCursor;
								}
						}
				}
			else if (TxFormat > 0)
				{
					TxType = 0;
					while (!(SCON1 & 0x02));  SCON1 &= ~0x02;   SBUF1 = *TxFrPtr;
					--TxFormat;
					TxFrPtr++;
				}
			};
		}
}
		


UINT8 HexToBin(UINT8 * Data)
{

UINT8 retval;
	/* Convert first byte */

		retval = 0;

        if ((Data[0] >= '0') && (Data[0] <= '9'))
			{
	            retval = (Data[0] - '0');
			}
        else if ((Data[0] >= 'a') && (Data[0] <= 'f'))
			{
	            retval = (Data[0] - 'a' + 10);
			}
		else
			{
			}
		retval = retval << 4;
        if ((Data[1] >= '0') && (Data[1] <= '9'))
			{
	            retval += (Data[1] - '0');
			}
        else if ((Data[1] >= 'a') && (Data[1] <= 'f'))
			{
	            retval += (Data[1] - 'a' + 10);
			}
		else
			{
			}

		return (retval);
	
}

UINT8 BinToHex (UINT8 __code * TxPointer, UINT8 pos)
{
	UINT8 localdata, retval;
	
	localdata = *TxPointer;	

	if (pos == 1)
		{
			localdata = localdata & 0x0F;
		}
	else
		{
			localdata = localdata >> 4;
		}
		
		
	if (localdata > 9)
		{
			retval = 'a' + (localdata - 10);
		}
	else
		{
			retval = '0' + localdata;
		}
	return (retval);

}

void EraseAppFlash(void)
{

	UINT16 flashAddress;
    SEGMENT_VARIABLE_SEGMENT_POINTER(pwrite, UINT8, SEG_XDATA, SEG_DATA);

   VDM0CN = 0x80;

   RSTSRC = (RSTSRC_VAL | 0x02);

   flashAddress = LASTPAGEOFAPP;

   pwrite = (UINT8 SEG_XDATA *) flashAddress;
   FLKEY  = 0xA5; 
   FLKEY  = 0xF1;
   PSCTL |= 0x03;              // set up PSEE, PSWE
   *pwrite = 0x55;             // write the byte
   PSCTL &= ~0x03;             // clear PSEE and PSWE			       

   for (flashAddress = 0x1000; flashAddress < LASTPAGEOFAPP; flashAddress += 0x200)
   		{
	       pwrite = (UINT8 SEG_XDATA *) flashAddress;
		   FLKEY  = 0xA5; 
		   FLKEY  = 0xF1;
	       PSCTL |= 0x03;              // set up PSEE, PSWE
	       *pwrite = 0x55;             // write the byte
	       PSCTL &= ~0x03;             // clear PSEE and PSWE			       
		}

}

void ProgramByte(UINT16 flashAddress, UINT8 Data)
{

    SEGMENT_VARIABLE_SEGMENT_POINTER(pwrite, UINT8, SEG_XDATA, SEG_DATA);

	   VDM0CN = 0x80;

	   RSTSRC = (RSTSRC_VAL | 0x02);


       	pwrite = (UINT8 SEG_XDATA *) flashAddress;

   		FLKEY  = 0xA5; 
   		FLKEY  = 0xF1;
   		PSCTL |= 0x01;		       // set up PSEE, PSWE
		*pwrite = Data;     		// write the byte
		PSCTL &= ~0x03;             // clear PSEE and PSWE			       
}


void ProgramPage(UINT16 flashAddress, UINT8 * Data)
{

    SEGMENT_VARIABLE_SEGMENT_POINTER(pwrite, UINT8, SEG_XDATA, SEG_DATA);
	UINT16 Count;

	VDM0CN = 0x80;

	RSTSRC = (RSTSRC_VAL | 0x02);

    pwrite = (UINT8 SEG_XDATA *) flashAddress;

	for (Count = 0; Count < 0x400; Count++)
		{
	   		FLKEY  = 0xA5; 
	   		FLKEY  = 0xF1;
	   		PSCTL |= 0x01;		       // set up PSEE, PSWE
			*pwrite++ = *Data++;     		// write the byte
			PSCTL &= ~0x03;             // clear PSEE and PSWE			       
		}
}


INTERRUPT(Timer0_ISR, INTERRUPT_TIMER0)
{
	__asm
	ljmp 0x100B
	__endasm;
}

INTERRUPT(Timer1_ISR, INTERRUPT_TIMER1)
{
	__asm
	ljmp 0x101B
	__endasm;
}

INTERRUPT(Timer2_ISR, INTERRUPT_TIMER2)
{
	__asm
	ljmp 0x102B
	__endasm;
}

INTERRUPT(Timer3_ISR, INTERRUPT_TIMER3)
{
	__asm
	ljmp 0x1073
	__endasm;
}

INTERRUPT(Timer4_ISR, INTERRUPT_TIMER4)
{
	__asm
	ljmp 0x109B
	__endasm;
}

INTERRUPT(Timer5_ISR, INTERRUPT_TIMER5)
{
	__asm
	ljmp 0x10A3
	__endasm;
}

INTERRUPT(PCA0_ISR, INTERRUPT_PCA0)
{
	__asm
	ljmp 0x105B
	__endasm;
}

INTERRUPT(Adc_ConvComplete_ISR, INTERRUPT_ADC0_EOC)
{
	__asm
	ljmp 0x1053
	__endasm;
}

/***********************************************************************************/
/*                                                                                 */
/* UART_Init() - Initialisation routines for UART1                                 */
/*                                                                                 */
/***********************************************************************************/


void UART_Init()
{

	SMOD1     = 0x0C;
	SCON1     = 0x10;
    SBRLL1    = 0x30;
    SBRLH1    = 0xFF;
    SBCON1   |= 0x03;                  // set prescaler to 1
    SCON1    |= 0x02;
    SBCON1   |= 0x40;
}




/***********************************************************************************/
/*                                                                                 */
/* Port_Init() - Initialisation routines for the processor port pins               */
/*                                                                                 */
/***********************************************************************************/

void Port_Init(void)
{
    // P0.0  -  SCK  (SPI0), Open-Drain, Digital
    // P0.1  -  MISO (SPI0), Open-Drain, Digital
    // P0.2  -  MOSI (SPI0), Open-Drain, Digital
    // P0.3  -  NSS, (SPI0), Open-Drain, Digital
    // P0.4  -  CEX0  (PCA), Open-Drain, Digital
    // P0.5  -  CEX1  (PCA), Open-Drain, Digital
    // P0.6  -  CEX2  (PCA), Push-Pull,  Digital
    // P0.7  -  CEX3  (PCA), Push-Pull,  Digital

    // P1.0  -  CEX4  (PCA), Open-Drain, Digital
    // P1.1  -  TX1 (UART1), Push-Pull,  Digital
    // P1.2  -  RX1 (UART1), Open-Drain, Digital
    // P1.3  -  Unassigned,  Push-Pull,  Digital
    // P1.4  -  Unassigned,  Push-Pull,  Digital
    // P1.5  -  Unassigned,  Open-Drain, Digital
    // P1.6  -  Unassigned,  Open-Drain, Digital
    // P1.7  -  Unassigned,  Open-Drain, Digital

    // P2.0  -  Unassigned,  Open-Drain, Digital
    // P2.1  -  Unassigned,  Open-Drain, Digital
    // P2.2  -  Unassigned,  Push-Pull,  Digital
    // P2.3  -  Unassigned,  Open-Drain, Digital
    // P2.4  -  Unassigned,  Open-Drain, Digital
    // P2.5  -  Unassigned,  Open-Drain, Digital
    // P2.6  -  Unassigned,  Open-Drain, Digital
    // P2.7  -  Unassigned,  Open-Drain, Digital

    // P3.0  -  Unassigned,  Open-Drain, Digital

    P1MDIN    = 0x3F;
    P2MDIN    = 0xC4;
    P0MDOUT   = 0xCD;
    P1MDOUT   = 0x1A;
    P2MDOUT   = 0x04;

//    P0SKIP    = 0x08;
    XBR0      = 0x02;
    XBR1      = 0x45;
    XBR2      = 0x01;

}




/***********************************************************************************/
/*                                                                                 */
/* _sdcc_external_startup() - Correction for SDCC problem                          */
/*                                                                                 */
/***********************************************************************************/

#if defined SDCC
void _sdcc_external_startup (void)
{
   PCA0MD &= ~0x40;                       /* Set watchdog off */
}
#endif
