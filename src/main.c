#include "main.h"

#define SIZE 32                  /* Transfer Buffer Size */
#define FIN_FL "data.txt"        /* Name of file on RAM */
#define FIN_FL_N 9               /* Size of file name fin */
#define FOUT_FL "data.txt"       /* Name of file on USB */
#define FOUT_FL_N 9              /* Size of file name fout */

#define DELAY 1000               /* Delay between two read of ACCELLERO data */

FATFS RAMFatFs;                  /* File system object for RAM disk logical drive */
FATFS USBFatFs;                  /* File system object for RAM disk logical drive */
FIL fin;                         /* File object in RAM*/
FIL fout;                        /* File object in USB*/
char RAMPath[4];                 /* RAM disk logical drive path */
char USBPath[4];                 /* USB disk logical drive path */
static uint8_t buffer[_MAX_SS];  /* a work buffer for the f_mkfs() */
static uint8_t buffer_tra[SIZE]; /* transfer buffer for copy data from ram*/
static uint16_t count_data = 0;  /* count of data in ram */
static uint8_t dataworked = 0;   /* number of byte read/write */
static uint8_t bufsize;          /* Temp var*/
int16_t datasensor[3];           /* Data of sensor */
char* fin_name;                  /* Path of file fin */
char* fout_name;                 /* Path of file fout */

uint16_t counter = 0;            /* Counter for Delay */

enum status STATE = IDLE;        /* USB Device Status */
USBH_HandleTypeDef  hUSB_Host;

/* Private function prototypes -----------------------------------------------*/
static void make_path(char disk[4], char* filename, int n, char** out);
static void USBH_UserProcess(USBH_HandleTypeDef* phost, uint8_t id);
static void SystemClock_Config(void);
static void Error_Handler(void);

int main(void)
{
	FRESULT res;

	HAL_Init();
	SystemClock_Config();
	
	BSP_LED_Init(LED4);
	BSP_LED_Init(LED5);

	//Init ACCELLERO sensor
	if (BSP_ACCELERO_Init() == ACCELERO_OK)
	{
		//Linking Driver for USB e RAM device
		if (FATFS_LinkDriver(&SRAMDISK_Driver, RAMPath) == 0 && FATFS_LinkDriver(&USBH_Driver, USBPath) == 0)
		{
			//Mount FATFs on RAM
			if (f_mount(&RAMFatFs, (TCHAR const*)RAMPath, 0) != FR_OK)
			{
				Error_Handler();
			}
			else
			{
				//MAKE FAT on RAM
				if (f_mkfs((TCHAR const*)RAMPath, FM_ANY, 0, buffer, sizeof(buffer)) != FR_OK)
				{
					Error_Handler();
				}
				else
				{
					//INIT USB to enable USB Device
					USBH_Init(&hUSB_Host, USBH_UserProcess, 0);
					USBH_RegisterClass(&hUSB_Host, USBH_MSC_CLASS);
					USBH_Start(&hUSB_Host);

					//Concat Path of RAM device with fin file name
					//fin_name = strcat(RAMPath, FIN_FL);
					make_path(RAMPath, FIN_FL, FIN_FL_N, &fin_name);
					
					//Open file fin, to write data of ACCELLERO
					if (f_open(&fin, fin_name, FA_CREATE_ALWAYS | FA_WRITE | FA_READ) != FR_OK)
					{
						Error_Handler();
					}
					else
					{
						while (1)
						{
							//Controll USB Device
							USBH_Process(&hUSB_Host);

							switch (STATE)
							{
								case IDLE:
								break;
								//IF USB is MEM
							case MEM:
								BSP_LED_Off(LED4);
								
								//Read data from ACCELLERO
								BSP_ACCELERO_GetXYZ(datasensor);
								//Write data on fout file
								res = f_write(&fin, datasensor, 3, (void*)&dataworked);

								if ((dataworked == 0) || (res != FR_OK))
									Error_Handler();
								//Increment count of data written
								count_data = count_data + 3;
								STATE = IDLE;
								break;
								//IF USB is TRANSFER
							case TRANSFER:
								if (count_data > 0)
								{
									if (f_mount(&USBFatFs, (TCHAR const*)USBPath, 0) != FR_OK)
									{
										Error_Handler();
									}
									else
									{
										//Concat Path of USB device with fout file name
										make_path(USBPath, FOUT_FL, FOUT_FL_N, &fout_name);
										//Open file fout, to read data from RAM
										if (f_open(&fout, fout_name, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
											Error_Handler();
										else
										{
											//Set index of file fin to beginning of file
											f_rewind(&fin);

											while (count_data > 0)
											{
												bufsize = count_data > SIZE ? SIZE : count_data;
												//Read data from fin
												res = f_read(&fin, buffer_tra, bufsize, (void*)&dataworked);

												if (res != FR_OK || dataworked == 0)
													Error_Handler();
												else
												{
													bufsize = count_data > SIZE ? SIZE : count_data;
													count_data = count_data - bufsize;
													//Write data to fout
													res = f_write(&fout, buffer_tra, bufsize, (void*)&dataworked);

													if (res != FR_OK || dataworked == 0)
														Error_Handler();
												}
											}

											//Close file fout
											if (f_close(&fout) != FR_OK)
												Error_Handler();

											//Set index of file fin to beginning of file
											f_rewind(&fin);
											//Set size of file fin to 0
											res = f_truncate(&fin);
											
											BSP_LED_On(LED4);
										}
									}

									break;
								}
							}
						}
					}
				}
			}
		}

		f_close(&fin);
		FATFS_UnLinkDriver(RAMPath);
		FATFS_UnLinkDriver(USBPath);
		while (1);
	}
	else
		Error_Handler();

}


void counter_it(void)
{
	if( ++counter == DELAY )
	{
		STATE = MEM;
		counter = 0;
	}
}

static void make_path(char disk[4], char* filename, int n, char** o)
{
	uint8_t i = 0;
	*o = (char*)malloc(sizeof(char) * (n + 3));
	for (i = 0; i < n + 3; i++)
		(*o)[i] = i < 3 ? disk[i] : filename[i - 3];
}

static void USBH_UserProcess(USBH_HandleTypeDef* phost, uint8_t id)
{
	switch (id)
	{
	case HOST_USER_SELECT_CONFIGURATION:
		break;

	case HOST_USER_DISCONNECTION:
		STATE = MEM;
		//Unmount USB Device
		f_mount(NULL, (const TCHAR*)USBPath, 0);
		break;

	case HOST_USER_CLASS_ACTIVE:
		STATE = TRANSFER;
		break;

	default:
		break;
	}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* The voltage scaling allows optimizing the power consumption when the device is
	   clocked below the maximum system frequency, to update the voltage scaling value
	   regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	   clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

	/* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
	if (HAL_GetREVID() == 0x1001)
	{
		/* Enable the Flash prefetch */
		__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
	}
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
	/* Turn LED3 on */
	BSP_LED_On(LED5);
	while (1);
}

