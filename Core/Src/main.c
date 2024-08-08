/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Timer rational
 * Tim1 -> Master clock 				Provides ROG
 * Tim2 -> Blank time Tiggered by #1
 * Tim3 -> Gatemode   Triggered by #2  	Provides SensorClk
 * Tim8 -> 								Provides  trigger Ext trigger by 3 to ADC
 * ADC triggers DMA to Buffer D2
 * ADC Complete triggers DMA from D2 to SDram
 *
 *
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "bdma.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "mdma.h"
#include "memorymap.h"
#include "rtc.h"
#include "sdmmc.h"
#include "tim.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "memtest.h"
#include "ssd1306.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DMA_BUFFER   __attribute__((section(".sdram")))
#define DMAd2BUFFER   __attribute__((section(".dma_buffer")))

#define DMABUFFLEN (Npixels*2)

#define SLOTS ((30*1024*1024 /(Npixels*2)))
//#define TIMENumer 6
//#define TIMEDenom 8
#define ONEPWR
//#define  WRSLOTS 16

enum Seq{ SEQ_Ready=0,SEQ_Running,SEQ_ONgoing,SEQ_Complete};
enum ADC_state{ADC_idle,ADC_1half,ADC_2half };
enum SD_state{SDnotopen,SDopen};
enum SelectN_slc{SelectNNOTSelectNed,SelectNSelectNed};
enum ChoiceS{SelectN_RUN,SelectN_EXP,SelectN_LGTH};
enum XferReady {NotReady,Ready};
enum RUNSTATE {CONTINUOUS,SINGLE,HISTOGRAM,HALT};
//#define HALFCPLT_ADC_GPIO 	HAL_GPIO_TogglePin(ADChalfCplt_GPIO_Port, ADChalfCplt_Pin);

#define CPLT_TIM1pe_GPIO	HAL_GPIO_TogglePin(GPIOH, Tim1Cplt_Pin)
#define CPLT_TIM8pe_GPIO	HAL_GPIO_TogglePin(GPIOH, Tim8Cplt_Pin)
#define CPLT_ADC_GPIO		HAL_GPIO_TogglePin(GPIOH, ADCCplt_Pin)

#define TRIGGER_TIM1_GPIO	HAL_GPIO_TogglePin(GPIOH, Trigger_Pin)
#define DMA_XFR_CPLT_GPIO   HAL_GPIO_TogglePin(GPIOG, DMAxfr_Pin)
#define SD_WriteStartTime   HAL_GPIO_TogglePin(GPIOD, SDwrt_Pin)

#define SDRAMAREA 0xc0000000

#define DISK
#define  LCD_ShowString(a, b, c,d, e, f) {ssd1306_SetCursor(a, b); ssd1306_WriteString(f, Font_7x10);	   ssd1306_UpdateScreen();}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

volatile uint16_t  *DMA_2nBuffer;
FIL fp;
char rtext[_MAX_SS] = "1234567890 1234567890 ";;
//char whitespace [234];
char fileHeadertext[_MAX_SS];
uint8_t FontH;// = Font_7x10.FontHeight;
GPIO_PinState buttonState=0;
GPIO_PinState oldState=0;
uint8_t Sequence=SEQ_Ready;
uint8_t ADCstate=ADC_idle;
uint8_t SDstate=SDnotopen;
uint8_t RunState = HALT;
uint8_t xferState = NotReady;
int8_t MenuItem =0;
int8_t SelectNEState = 1;
uint16_t SelectNEStateValue[]={6000,7000,8000,9000,10000,12000};
int8_t SelectNLState = 2;
uint16_t SelectNLStateValue[]={4096,8192,16384,32768};

char RunType[]={'C','S','H'};
uint8_t SelectRT=2;
uint32_t DACval=0;

FRESULT f_err_code;
static FATFS FATFS_Obj;

int Nfilename=0;
FIL fp;
uint32_t BytesWritten=0;
int hlen;
volatile DMAd2BUFFER uint16_t dmaBuffer[DMABUFFLEN];

#ifdef Exposure
#undef Exposure
#undef NumLines
#endif
uint32_t Exposure ;
uint32_t NumLines ;
RTC_TimeTypeDef timeStart;
RTC_TimeTypeDef timeEnd;
RTC_TimeTypeDef Filetime;
uint32_t secStart=0;
uint16_t lastpixel=0;
volatile uint32_t IRQCNT_HAL_UART_TxCpltCallback=0;
volatile uint32_t IRQCNT_HAL_ADC_ConvCpltCallback=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback1=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback2=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback3=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback8=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedCallback1=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedCallback2=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedCallback3=0;
volatile uint32_t IRQCNT_HAL_TIM_PeriodElapsedCallback8=0;
volatile uint32_t IRQCNT_HAL_TIM_TriggerCallback1=0;
volatile uint32_t IRQCNT_HAL_TIM_TriggerCallback2=0;
volatile uint32_t IRQCNT_HAL_TIM_TriggerCallback3=0;
volatile uint32_t IRQCNT_HAL_TIM_TriggerCallback8=0;

volatile uint32_t TransferComplete7=0;
volatile uint32_t TransferCompleteelse=0;

volatile uint32_t HAL_GPIO_EXTI_CallbackK1=0;
volatile uint32_t HAL_GPIO_EXTI_CallbackSelectN=0;

volatile uint32_t HAL_GPIO_EXTI_CallbackNxt=0;

volatile uint32_t pixelPlaceOld, pixelPlace;
volatile uint32_t BuffDiff=0,BuffDiffMax=0;
volatile uint32_t CurrentSlot=0;
volatile uint32_t LastWrittenSlot=0;
volatile uint32_t NumberOfWrites=0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void TransferComplete(DMA_HandleTypeDef *hdma);
void TransferError(DMA_HandleTypeDef *hdma);
void TrialEnd(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int RTC_Set(
		uint8_t year, uint8_t month, uint8_t day,
		uint8_t hour, uint8_t min)
{
	HAL_StatusTypeDef res;
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	memset(&time, 0, sizeof(time));
	memset(&date, 0, sizeof(date));

	date.WeekDay = 0;
	date.Year = year;
	date.Month = month;
	date.Date = day;

	res = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
		Error_Handler();
	}

	time.Hours = hour;
	time.Minutes = min;
	time.Seconds = 0;

	res = HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
		Error_Handler();
	}

	return 0;
}
//=========================================================================================================================

//volatile uint8_t keyval=0;

uint8_t JOYstickPOS()
{
	uint8_t	keyval=0;
	keyval = HAL_GPIO_ReadPin (JOYstickPB_GPIO_Port,JOYstickPB_Pin)<<4 ;
	keyval +=		HAL_GPIO_ReadPin (GPIOB, JOYstick4_Pin)<<3 ;
	keyval +=		HAL_GPIO_ReadPin (GPIOB, JOYstick3_Pin)<<2 ;
	keyval +=		HAL_GPIO_ReadPin (GPIOH, JOYstick2_Pin)<<1 ;
	keyval +=		HAL_GPIO_ReadPin (GPIOH, JOYstick1_Pin) ;
	keyval = (~keyval) & 0x1f;

	return keyval;
}
void ledDisp(uint8_t Localval)
{
	if(0x01 & Localval)
		HAL_GPIO_WritePin(GPIOC, LED1_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, LED1_Pin, GPIO_PIN_RESET);
	if(0x02 & Localval)
		HAL_GPIO_WritePin(GPIOC, LED2_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, LED2_Pin, GPIO_PIN_RESET);
	if(0x04 & Localval)
		HAL_GPIO_WritePin(GPIOC, LED3_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, LED3_Pin, GPIO_PIN_RESET);
	if(0x08 & Localval)
		HAL_GPIO_WritePin(GPIOA, LED4_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOA, LED4_Pin, GPIO_PIN_RESET);
	if(0x10 & Localval)
		HAL_GPIO_WritePin(GPIOA, LED5_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOA, LED5_Pin, GPIO_PIN_RESET);
	if(0x20 & Localval)
		HAL_GPIO_WritePin(GPIOA, LED6_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOA, LED6_Pin, GPIO_PIN_RESET);
	if(0x40 & Localval)
		HAL_GPIO_WritePin(GPIOA, LED7_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOA, LED7_Pin, GPIO_PIN_RESET);
	if(0x80 & Localval)
		HAL_GPIO_WritePin(GPIOA, LED8_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOA, LED8_Pin, GPIO_PIN_RESET);

}
void setTime()
{
	int8_t Jsp=0,TimeItem=0;
	int8_t TimeArray[12];
	RTC_DateTypeDef FILEdate;
	RTC_TimeTypeDef FILEtime;
	HAL_StatusTypeDef res;

	ssd1306_Clear();

	res = HAL_RTC_GetTime(&hrtc, &FILEtime, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
		Error_Handler();
	}

	res = HAL_RTC_GetDate(&hrtc, &FILEdate, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
		Error_Handler();
	}
    TimeArray[0] =(FILEdate.Year  /10) %10;
    TimeArray[1] =(FILEdate.Year - TimeArray[0])%10;

    TimeArray[2] =(FILEdate.Month /10)%10;
    TimeArray[3] =(FILEdate.Month - TimeArray[2])%10;

    TimeArray[4] =(FILEdate.Date /10)%10;
    TimeArray[5] =(FILEdate.Date - TimeArray[4])%10;

    TimeArray[6] =(FILEtime.Hours /10)%10;
    TimeArray[7] =(FILEtime.Hours - TimeArray[6])%10;

    TimeArray[8] =(FILEtime.Minutes /10)%10;
    TimeArray[9] =(FILEtime.Minutes - TimeArray[8])%10;



	while (1)
	{
		sprintf(rtext, "%4d / %2d / %2d",
				2000+TimeArray[0]*10+TimeArray[1],
				TimeArray[2]*10+TimeArray[3],
				TimeArray[4]*10+TimeArray[5]);
		LCD_ShowString(2, 0, 6,8 , White, rtext);
		sprintf(rtext," %2d:%2d",
				TimeArray[6]*10+TimeArray[7],
				TimeArray[8]*10+TimeArray[9]);
		LCD_ShowString(2, FontH*2, 6,8 , White, rtext);
		Jsp = JOYstickPOS();
		HAL_Delay(25);
		switch (Jsp){
		case (1):{   //R
			if (TimeItem == 9) {TimeItem = 0;} else TimeItem++ ;
			break;
		}
		case (4):{  //L
			if (TimeItem>0) {TimeItem-- ;}
			else MenuItem =9;
			break;
		}
		case (16):{ //UP
			TimeArray[TimeItem] == 9 ? TimeArray[TimeItem]=0: TimeArray[TimeItem]++;
			break;
		}
		case (8):{ //DN
			TimeArray[TimeItem] == 0 ? TimeArray[TimeItem]=9 :TimeArray[TimeItem]--;
			break;
		}
		case (2):{
			RTC_Set(TimeArray[0]*10+TimeArray[1],
					TimeArray[2]*10+TimeArray[3],
					TimeArray[4]*10+TimeArray[5],
					TimeArray[6]*10+TimeArray[7],
					TimeArray[8]*10+TimeArray[9]);

			ssd1306_Clear();

			return;
			break;
		}

		}

	}

}
//=========================================================================================================================

void *memset16(void *m, uint16_t val, size_t count)
{
	uint16_t *buf = m;

	while(count--) *buf++ = val;
	return m;
}

//=========================================================================================================================
void ProcessBlock()
{
	uint32_t hist[128];
	uint32_t sharp[128];
	memset(hist,0,128*sizeof(uint32_t));
	memset(sharp,0,128*sizeof(uint32_t));
	uint32_t CS = CurrentSlot %SLOTS ? (CurrentSlot-2) %SLOTS : CurrentSlot %SLOTS ;
	uint16_t *linptr = (uint16_t *)(SDRAMAREA+((CS%SLOTS)*(Npixels*2)));
	uint32_t norm =0;//,zero=0;
	uint32_t pindex;
	uint32_t partialdiff;//,oldp;

	for (int i=0 ; i < Npixels ;i++){
		pindex = linptr[i];

		pindex = pindex>>9; //segments of 128(of 65536)
		if (pindex > 127) pindex = 127;
		hist[pindex]++;

		if (norm < hist[pindex])
			  norm = hist[pindex]; //find local max

		partialdiff = abs((int32_t)linptr[i] - (int32_t)linptr[i+1]);
		if (partialdiff > sharp[i/(Npixels>>7)] )
			sharp[i/(Npixels>>7)] = partialdiff;
	}
	ssd1306_Clear();

	for (int i=0 ;i<128;i++){
		if (hist[i] != 0)
		{
			uint32_t v= hist[i]*48/norm;
			ssd1306_DrawVerticalLine(i, (int16_t)48-v ,(int16_t) v);
		}
		uint32_t w = sharp[i]>>10;
		ssd1306_DrawVerticalLine(i, (int16_t)64-w ,(int16_t) w);

	}

	ssd1306_UpdateScreen();


	//	printf("norm %d , Zeros %d\n",norm,zero);


}
//=========================================================================================================================
void Histo()
{
	ssd1306_Clear();
	//HAL_GPIO_WritePin(REDled_GPIO_Port, REDled_Pin, GPIO_PIN_SET); //Clear Error light
	TransferComplete7=0;
	TransferCompleteelse=0;
			if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1))Error_Handler(); //reload Exposure& Length
			if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2))Error_Handler(); //reload Exposure& Length
			if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3))Error_Handler(); //reload Exposure& Length
//			HAL_TIM_Base_Stop(&htim1);
//			HAL_TIM_Base_Stop_IT(&htim1);

	htim1.Init.Period = (Exposure*TIMEDenom/TIMENumer)-1;
	htim1.Init.RepetitionCounter = NumLines-1;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK)Error_Handler();

	Sequence=SEQ_Running;
	CurrentSlot=0;
	LastWrittenSlot=0;
	HAL_TIM_Base_Start(&htim2);HAL_TIM_Base_Start(&htim3);HAL_TIM_Base_Start(&htim8);

	if (HAL_OK !=   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1))Error_Handler();
	if (HAL_OK !=   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2))Error_Handler();
	if (HAL_OK !=   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3))Error_Handler();
//	HAL_TIM_Base_Start(&htim1);
//	HAL_TIM_Base_Start_IT(&htim1);

	HAL_Delay(80);
	oldState = GPIO_PIN_SET;
	{
		while(RunState != HALT && (SEQ_Complete != Sequence)){
			if (CurrentSlot >2)ProcessBlock();
			if (0 != JOYstickPOS()){
				RunState = HALT;
				Sequence=SEQ_Complete;
			}
		}

	}
	TrialEnd();

}//complete

//----------------------------------------------------------------------------
void TrialStart()
{
	if( SDstate == SDnotopen)
	{
		ssd1306_Clear();
//		IRQCNT_HAL_UART_TxCpltCallback=0;
		IRQCNT_HAL_ADC_ConvCpltCallback=0;
//		IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback1=0;
//		IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback2=0;
//		IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback3=0;
//		IRQCNT_HAL_TIM_PeriodElapsedHalfCpltCallback8=0;
//		IRQCNT_HAL_TIM_PeriodElapsedCallback1=0;
//		IRQCNT_HAL_TIM_PeriodElapsedCallback2=0;
//		IRQCNT_HAL_TIM_PeriodElapsedCallback3=0;
//		IRQCNT_HAL_TIM_PeriodElapsedCallback8=0;
//		IRQCNT_HAL_TIM_TriggerCallback1=0;
//		IRQCNT_HAL_TIM_TriggerCallback2=0;
//		IRQCNT_HAL_TIM_TriggerCallback3=0;
//		IRQCNT_HAL_TIM_TriggerCallback8=0;
//		HAL_GPIO_WritePin(REDled_GPIO_Port, REDled_Pin, GPIO_PIN_SET); //Clear Error light
		TransferComplete7=0;
		TransferCompleteelse=0;
		DACval=0;BuffDiffMax=0;
#ifdef DISK
		f_err_code = f_mount( &FATFS_Obj, (TCHAR const*)SDPath,0);
		if (f_err_code != FR_OK )
		{
			printf("Diskerror%d /n",f_err_code);
			{
				Error_Handler();
			}
		}
#endif
		HAL_TIM_Base_Start(&htim2);HAL_TIM_Base_Start(&htim3);HAL_TIM_Base_Start(&htim8);

				if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1))Error_Handler(); //reload Exposure& Length
				if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2))Error_Handler(); //reload Exposure& Length
				if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3))Error_Handler(); //reload Exposure& Length
//				HAL_TIM_Base_Stop(&htim1);
//				HAL_TIM_Base_Stop_IT(&htim1);

		htim1.Init.Period = (Exposure*TIMEDenom/TIMENumer)-1;
		htim1.Init.RepetitionCounter = NumLines-1;
		if (HAL_TIM_Base_Init(&htim1) != HAL_OK)Error_Handler();


		RTC_DateTypeDef FILEdate;
		HAL_StatusTypeDef res;

		res = HAL_RTC_GetTime(&hrtc, &Filetime, RTC_FORMAT_BIN);
		if(res != HAL_OK) Error_Handler();


		res = HAL_RTC_GetDate(&hrtc, &FILEdate, RTC_FORMAT_BIN);
		if(res != HAL_OK) Error_Handler();


		HAL_Delay(800);

		sprintf((char *)rtext,"0:%02d%02d%02d%02d%02d%02d_%02d.pgm",
				FILEdate.Year,
				FILEdate.Month,
				FILEdate.Date,
				Filetime.Hours,
				Filetime.Minutes,
				Filetime.Seconds,
				SelectNEStateValue[SelectNEState]/1000);
#ifdef DISK
		f_err_code = f_open(&fp,( char *)rtext,FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
		HAL_Delay(800);
		if (f_err_code != 		FR_OK	){
			LCD_ShowString(2, 0, 6,8, White, "FC Error");
			f_close(&fp);
			Error_Handler();
		}
//
//		sprintf((char *)fileHeadertext,"P5 %5d %5d 65535 ", Npixels,SelectNLStateValue[SelectNLState] );
//		int hlen = strlen(fileHeadertext);
//		res = f_write ( &fp,
//				(const void*) fileHeadertext ,
//				hlen ,
//				(void *)&BytesWritten);
//		if (res  != 0 || (BytesWritten != (hlen))	){
//			LCD_ShowString(2, 0, 6,8,White, "FW Error");
//			f_close(&fp);
//			Error_Handler();
//		}

#endif
		secStart=((uint32_t)Filetime.Hours*3600)+((uint32_t)Filetime.Minutes*60)+(uint32_t)Filetime.Seconds;
		LCD_ShowString(2, FontH, 6, 8, White,rtext);

		SDstate=SDopen;
		sprintf((char *)rtext,"Run %d %d  ",SelectNEStateValue[SelectNEState],SelectNLStateValue[SelectNLState]);
		LCD_ShowString(2, 0, 6,8 , White, rtext);
		NumberOfWrites =0;
		Sequence=SEQ_Running;

		CurrentSlot=0;
		LastWrittenSlot=0;
		HAL_TIM_Base_Start(&htim2);HAL_TIM_Base_Start(&htim3);HAL_TIM_Base_Start(&htim8);
			if (HAL_OK !=   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1))Error_Handler();
			if (HAL_OK !=   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2))Error_Handler();
			if (HAL_OK !=   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3))Error_Handler();
//			HAL_TIM_Base_Start(&htim1);
//			HAL_TIM_Base_Start_IT(&htim1);

		oldState = GPIO_PIN_SET;
	}
}
//=========================================================================================================================
void TrialEnd()
{
	if (SDstate == SDopen)
	{
#ifdef DISK
		f_close(&fp);
#endif
	}

			if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1))Error_Handler(); //reload Exposure& Length
			if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2))Error_Handler(); //reload Exposure& Length
			if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3))Error_Handler(); //reload Exposure& Length
			HAL_TIM_Base_Stop(&htim2);HAL_TIM_Base_Stop(&htim3);HAL_TIM_Base_Stop(&htim8);
//			HAL_TIM_Base_Stop(&htim1);
//			HAL_TIM_Base_Stop_IT(&htim1);



}
//=========================================================================================================================
void TrialComplete()
{
	if (SDstate == SDopen)
	{
		{

			FRESULT res;
			//			SD_WriteStartTime;
			BuffDiff=(CurrentSlot)-(LastWrittenSlot);

			while ((CurrentSlot)> (LastWrittenSlot) && (RunState != HALT))
			{
#ifdef DISK

				res = f_write ( &fp,														/* Pointer to the file object */
						(const void*) (SDRAMAREA+((LastWrittenSlot%SLOTS)*(Npixels*2))),	/* Pointer to the data to be written */
						(Npixels*2),														/* Number of bytes to write */
						(void *)&BytesWritten										/* Pointer to number of bytes written */
				);
				if (res  != FR_OK || (BytesWritten !=(Npixels*2))	){
					LCD_ShowString(2, 0, 6,8,White, "FW Error");
					f_close(&fp);
					Error_Handler();
				}
#endif
				LastWrittenSlot++;
				//				SD_WriteStartTime;
				pixelPlace=((LastWrittenSlot <<7) / NumLines);
				if (pixelPlaceOld != pixelPlace){
					ssd1306_DrawPixel(pixelPlace, 62);
					HAL_GPIO_TogglePin(BLUEled_GPIO_Port, BLUEled_Pin);
					ssd1306_UpdateScreen();
					pixelPlaceOld = pixelPlace;
				}
				if (0 != JOYstickPOS()){
					RunState = HALT;
				}

			}
		}
		TrialEnd();
		HAL_StatusTypeDef res = HAL_RTC_GetTime(&hrtc, &timeEnd, RTC_FORMAT_BIN);
		if(res != HAL_OK) {
			Error_Handler();
		}

		uint32_t sec = ((uint32_t)timeEnd.Hours*3600)+((uint32_t)timeEnd.Minutes*60)+((uint32_t)timeEnd.Seconds) -(secStart);
		sprintf(( char *)rtext,"Time %ld sec          ",sec);
		LCD_ShowString(2, 2*FontH,6,8,White,rtext);
		SDstate= SDnotopen;
		RTC_DateTypeDef FILEdate;
		res = HAL_RTC_GetDate(&hrtc, &FILEdate, RTC_FORMAT_BIN);

	}//complete
	Sequence=SEQ_Ready;
	HAL_Delay(100);
	ledDisp(0);
}

//=========================================================================================================================
void TrialOngoing (){

	if (SDstate == SDopen){
		FRESULT res;
		BuffDiff=(CurrentSlot)-(LastWrittenSlot);
		//		SD_WriteStartTime;



		while ((CurrentSlot)> (LastWrittenSlot) ){

#ifdef DISK
			if(LastWrittenSlot == 0 ){
				sprintf((char *)SDRAMAREA,"P5 %5d %5d 65535 ", Npixels,SelectNLStateValue[SelectNLState] );
			}
			res = f_write ( &fp,														/* Pointer to the file object */
					(const void*) (SDRAMAREA+((LastWrittenSlot%SLOTS)*(Npixels*2))),	/* Pointer to the data to be written */
					(Npixels*2),														/* Number of bytes to write */
					(void *)&BytesWritten										/* Pointer to number of bytes written */
			);
			BuffDiff=(CurrentSlot)-(LastWrittenSlot);
			if(BuffDiff > BuffDiffMax)(BuffDiffMax=BuffDiff);
			LastWrittenSlot++;
			NumberOfWrites++;


			if (res  != FR_OK	 || (BytesWritten !=(Npixels*2))	){
				LCD_ShowString(2, 0,6,8,White, "FW Error");
				f_close(&fp);
				Error_Handler();

			}
#endif
			//			SD_WriteStartTime;
			pixelPlace=((LastWrittenSlot <<7) / NumLines);
			if (pixelPlaceOld != pixelPlace){
//				ssd1306_DrawPixel(pixelPlace, 63);
//				ssd1306_DrawPixel(((CurrentSlot <<7) / NumLines), 61);
				HAL_GPIO_TogglePin(BLUEled_GPIO_Port, BLUEled_Pin);
				if (BuffDiff>SLOTS) {
//					HAL_GPIO_WritePin(REDled_GPIO_Port, REDled_Pin, GPIO_PIN_RESET); //error
					RunState = HALT;
					Sequence=SEQ_Complete;

				}
				//if (pixelPlace % 8 == 0)ssd1306_UpdateScreen();
//				ledDisp(1<<((LastWrittenSlot/(NumLines>>3))));
				pixelPlaceOld = pixelPlace;
			}
			ledDisp(((LastWrittenSlot/(NumLines>>8))));

		}
	}
	ADCstate = ADC_idle;
	if (0 != JOYstickPOS()){
		RunState = HALT;
		Sequence=SEQ_Complete;
	}
}
//__-------___________________________________________________________________________________________

void RunMenu()
{
	uint8_t Jsp=0,i=0;
	ssd1306_Clear();
	sprintf(rtext,"RunType:  %c",RunType[SelectRT]);
	LCD_ShowString(2, 0, 6,8 , White, rtext);
	sprintf(rtext,"Exposure: %d",(SelectNEStateValue[SelectNEState]));
	LCD_ShowString(2, FontH, 6,8 , White, rtext);
	sprintf(rtext,"Length:   %d",SelectNLStateValue[SelectNLState]);
	LCD_ShowString(2, 2*FontH, 6,8 , White, rtext);
	ssd1306_DrawVerticalLine(127, MenuItem*FontH,FontH);
	while (RunState == HALT)
	{
		if (i ==0) i=1;
	//ledDisp(i);15
		i<<=1;
		Jsp = JOYstickPOS();

		{
		HAL_Delay(25);
		switch (Jsp){
		case (1):{   //R
			ssd1306_SetColor(Black);
			ssd1306_DrawVerticalLine(127,0,4*FontH);     //Cursur blank
			ssd1306_DrawVerticalLine(126,0,4*FontH);     //Cursur blank
			ssd1306_SetColor(White);
			if (MenuItem == 2) {MenuItem = 0;} else MenuItem++ ;
			ssd1306_DrawVerticalLine(127, MenuItem*FontH,FontH);
			ssd1306_DrawVerticalLine(126, MenuItem*FontH,FontH);
			break;
		}
		case (4):{  //L
			ssd1306_SetColor(Black);
			ssd1306_DrawVerticalLine(127, 0,4*FontH);
			ssd1306_DrawVerticalLine(126, 0,4*FontH);
			ssd1306_SetColor(White);
			if (MenuItem) {MenuItem-- ;}
			else MenuItem =2;
			ssd1306_DrawVerticalLine(127, MenuItem*FontH,FontH);
			ssd1306_DrawVerticalLine(126, MenuItem*FontH,FontH);
			break;
		}
		case (16):{ //UP
			if (MenuItem == 0)
			{
				if (SelectRT == 2) {SelectRT = 0;} else SelectRT++ ;
			}
			else if (MenuItem == 1)
			{
				if (SelectNEState == 5) {SelectNEState = 0;} else SelectNEState++ ;
			}
			else
			{
				if (SelectNLState == 3) {SelectNLState = 0;} else SelectNLState++ ;
			}
			break;
		}
		case (8):{ //DN
			if (MenuItem == 0)
			{
				if(SelectRT) {SelectRT--;} else SelectRT =2;
			}
			else if (MenuItem == 1)
			{
				if (SelectNEState) { SelectNEState--;} else  SelectNEState =5;
			}
			else
			{
				if(SelectNLState) 	{SelectNLState--;}else SelectNLState=3;
			}
			break;
		}
		case (2):{
			if (SelectRT == 0 )
				RunState = CONTINUOUS;
			else if (SelectRT == 1 )
				RunState = SINGLE;
			else
				RunState = HISTOGRAM;
			break;
		}

		}
		sprintf(rtext,"RunType:  %c",RunType[SelectRT]);
		LCD_ShowString(2, 0, 6,8 , White, rtext);
		sprintf(rtext,"Exposure: %d    ",(SelectNEStateValue[SelectNEState]));
		LCD_ShowString(2, FontH, 6,8 , White, rtext);
		sprintf(rtext,"Length:   %d    ",SelectNLStateValue[SelectNLState]);
		LCD_ShowString(2, 2*FontH, 6,8 , White, rtext);
	}
	}

}
//__-------___________________________________________________________________________________________

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	 FontH = Font_7x10.FontHeight;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

	//	uint8_t cState=2;

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	HAL_I2C_RegisterCallback(&hi2c4,HAL_I2C_MEM_TX_COMPLETE_CB_ID,HAL_I2C_MemTxCpltCallback);

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_MDMA_Init();
  MX_BDMA_Init();
  MX_TIM3_Init();
  MX_TIM8_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_FATFS_Init();
  MX_RTC_Init();
  MX_SDMMC1_SD_Init();
  MX_ADC3_Init();
  MX_FMC_Init();
  MX_I2C4_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
   MX_RTC_InitAlt();

  //	HAL_GPIO_WritePin(SensorPWRenable_GPIO_Port, SensorPWRenable_Pin, GPIO_PIN_RESET);
	MX_GPIO2Init();  //pullups for keypad

	for(int i=1; i<128; i++)
	{
		int ret = HAL_I2C_IsDeviceReady(&hi2c4, (uint16_t)(i<<1), 3, 5);
		if (ret != HAL_OK) /* No ACK Received At That Address */
		{
			// HAL_UART_Transmit(&huart2, Space, sizeof(Space), 10000);
		}
		else if(ret == HAL_OK)
		{
			sprintf(rtext, "0x%X", i);
			//    HAL_UART_Transmit(&huart2, Buffer, sizeof(Buffer), 10000);
		}
	}
	HAL_I2C_RegisterCallback(&hi2c4,HAL_I2C_MEM_TX_COMPLETE_CB_ID,HAL_I2C_MemTxCpltCallback);

	ssd1306_Init();
//	HAL_GPIO_WritePin(REDled_GPIO_Port, REDled_Pin, GPIO_PIN_SET);
	ledDisp(255);
	sprintf(rtext," Hellow World");
	LCD_ShowString(2, 0, 6,8, White, rtext);
	LCD_ShowString(2, 2*FontH, 6,8, White, rtext);
	LCD_ShowString(2, 4*FontH, 6,8, White, rtext);
	LCD_ShowString(2, 6*FontH, 6,8, White, rtext);
	ledDisp(0);

	memTest();



#ifdef DISK
	if(BSP_SD_Init()!=0)
	{
		HAL_Delay(5);
	}
	if(disk_initialize(0) != RES_OK)
	{
		HAL_Delay(5);
	}
#endif
	if(HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)	Error_Handler();

	HAL_ADC_RegisterCallback(&hadc3, HAL_ADC_CONVERSION_COMPLETE_CB_ID,HAL_ADC_ConvCpltCallback );
	HAL_ADC_RegisterCallback(&hadc3, HAL_ADC_CONVERSION_HALF_CB_ID,HAL_ADC_ConvHalfCpltCallback );

	HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_stream7, HAL_DMA_XFER_CPLT_CB_ID, TransferComplete);
	HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_stream7, HAL_DMA_XFER_ERROR_CB_ID, TransferError);


	HAL_ADC_Start_DMA(&hadc3,(uint32_t *)dmaBuffer,DMABUFFLEN);

	if (HAL_OK != HAL_TIM_Base_Start_IT(&htim1)) Error_Handler();


	if (HAL_OK !=  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2))Error_Handler();
	if (HAL_OK !=  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1))		Error_Handler();
	if (HAL_OK !=  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1))		Error_Handler();
	if (HAL_OK !=  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2))		Error_Handler();
	if (HAL_OK !=  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1))		Error_Handler();

	//RTC_Set(23, 5, 5, 11,28, 0,5) ;  //CLOCK INITILAIZE

	if (HAL_GPIO_ReadPin (GPIOH, TimeSet_Pin)==0) setTime();
#ifdef ONEPWR
	HAL_GPIO_WritePin(Enable9_GPIO_Port, Enable9_Pin, GPIO_PIN_SET);
	HAL_Delay(500);
	HAL_GPIO_WritePin(SensorPWRenable_GPIO_Port, SensorPWRenable_Pin, GPIO_PIN_RESET); //5v enable signal* 125 boards
	HAL_Delay(500);
#endif
	HAL_Delay(500);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		RunMenu();
//		RunState = HISTOGRAM;//}{ menu test debug

		Exposure = (SelectNEStateValue[SelectNEState]);
		NumLines = SelectNLStateValue[SelectNLState];
		//
		if (RunState == HISTOGRAM)
		{
#ifndef ONEPWR
			HAL_GPIO_WritePin(Enable9_GPIO_Port, Enable9_Pin, GPIO_PIN_SET);
			HAL_Delay(500);
			HAL_GPIO_WritePin(SensorPWRenable_GPIO_Port, SensorPWRenable_Pin, GPIO_PIN_RESET); //5v enable signal
			HAL_Delay(500);
#endif
			Histo();
#ifndef ONEPWR
			HAL_GPIO_WritePin(SensorPWRenable_GPIO_Port, SensorPWRenable_Pin, GPIO_PIN_SET); //5v disable signal
    		HAL_Delay(250);
			HAL_GPIO_WritePin(Enable9_GPIO_Port, Enable9_Pin, GPIO_PIN_RESET);
		//	HAL_Delay(1000);
#endif
		}
		else{
			//***********************************************************************************************
#ifndef ONEPWR
			HAL_GPIO_WritePin(Enable9_GPIO_Port, Enable9_Pin, GPIO_PIN_SET);
			HAL_Delay(500);
			HAL_GPIO_WritePin(SensorPWRenable_GPIO_Port, SensorPWRenable_Pin, GPIO_PIN_RESET); //5v enable signal
			HAL_Delay(500);
#endif
			do{
				TrialStart();
				while (1){

					if (SEQ_Complete == Sequence){  //================================================================
						TrialComplete();
						sprintf(rtext," Complete          ");
						LCD_ShowString(2, 0, 6,8, White, rtext);
						HAL_Delay(1000);

						break;
					}
					if (ADCstate == ADC_2half && Sequence==SEQ_ONgoing){//============================================
						TrialOngoing();
					}
					if (xferState == Ready){
						xferState = NotReady;
					}

				}//******************************************************************************************************
			}while (RunState == CONTINUOUS);
#ifndef ONEPWR
			HAL_GPIO_WritePin(SensorPWRenable_GPIO_Port, SensorPWRenable_Pin, GPIO_PIN_SET); //5v enable signal
			HAL_Delay(250);
			HAL_GPIO_WritePin(Enable9_GPIO_Port, Enable9_Pin, GPIO_PIN_RESET);
			HAL_Delay(100);
#endif
		}
		RunState = HALT;

		//-----------------------------------------------------------------------------------------------------------------

	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 20;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV4;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


void HAL_ADC_ConvCpltCallback (ADC_HandleTypeDef *hadc)
//HAL_ADC_ConvCpltCallback
{
	if (Sequence!=SEQ_Ready){
		//		CPLT_ADC_GPIO;
		xferState = Ready;
		Sequence = SEQ_ONgoing;
		//dmaBuffer = ;
		//		dmaBuffer[0] =(uint32_t ) IRQCNT_HAL_ADC_ConvCpltCallback;
		//		dmaBuffer[1] = (uint32_t )CurrentSlot;
		//		dmaBuffer[2] = (uint32_t )CurrentSlot%SLOTS;
		//		dmaBuffer[3] = (uint32_t )0x0;
		////		dmaBuffer[4] = (uint32_t )0xffffffff;

		HAL_DMA_Start_IT(&hdma_memtomem_dma1_stream7,(uint32_t )dmaBuffer, (uint32_t )(SDRAMAREA+((CurrentSlot%SLOTS)*(Npixels*2))), 2*Npixels*2);
		CurrentSlot+=2;

		IRQCNT_HAL_ADC_ConvCpltCallback++;
		//		CPLT_ADC_GPIO;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	// Check which version of the timer triggered this callback and toggle LED
	if (htim == &htim1)
	{
		//		CPLT_TIM1pe_GPIO;
		if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1))		Error_Handler();
		if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2))		Error_Handler();
		if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3))		Error_Handler();
//		if (HAL_OK !=  HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1))	Error_Handler();
		if (Sequence!=SEQ_Ready)
			Sequence=SEQ_Complete;
		IRQCNT_HAL_TIM_PeriodElapsedCallback1++;
		//		CPLT_TIM1pe_GPIO;
	}
}

void HAL_TIM_TriggerCallback(TIM_HandleTypeDef *htim)
{
	// Check which version of the timer triggered this callback and toggle LED
	if (htim == &htim1)
	{
		//		TRIGGER_TIM1_GPIO;
		IRQCNT_HAL_TIM_TriggerCallback1++;
		//		TRIGGER_TIM1_GPIO;
	}

}
void TransferComplete(DMA_HandleTypeDef *hdma)
{
	if (hdma == &hdma_memtomem_dma1_stream7){
		//CurrentSlot++;
		if (SEQ_ONgoing == Sequence)
		{
			//			DMA_XFR_CPLT_GPIO;
			ADCstate=ADC_2half;

			//			DMA_XFR_CPLT_GPIO;
		}
		TransferComplete7++;
	}else{
		TransferCompleteelse++;
	}
}
void TransferError(DMA_HandleTypeDef *hdma)
{
	ledDisp(0xE0);
	HAL_Delay(1500);
	if (SDstate == SDopen)
	{
#ifdef DISK
		f_close(&fp);
#endif
	}

	Error_Handler();
}
void Diskerror()
{

	ledDisp(0xD0);
	HAL_Delay(1500);
	if (SDstate == SDopen)
	{
#ifdef DISK
		f_close(&fp);
#endif
	}

	Error_Handler();
}


//
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//	if(GPIO_Pin == nextN_Pin) // INT Source is pin B0
	//	{
	//		NextNFlag = NxtSelectNed; //
	//		HAL_GPIO_EXTI_CallbackNxt++;
	//	}
	//	if(GPIO_Pin == K1_Pin) // INT Source is pin D4
	//	{
	//		K1Flag = 1;
	//		HAL_GPIO_EXTI_CallbackK1++;
	//		f_close(&fp);
	//		Error_Handler();
	//
	//	}
	//	if(GPIO_Pin == SelectN_Pin) // INT Source is pin B1
	//	{
	//		SelectNFlag = SelectNSelectNed;
	//		HAL_GPIO_EXTI_CallbackSelectN++;
	//
	//	}


}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
//	HAL_GPIO_WritePin(REDled_GPIO_Port, REDled_Pin, GPIO_PIN_RESET);
	__disable_irq();
	while (1)
	{
		ledDisp(0xf0);
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
