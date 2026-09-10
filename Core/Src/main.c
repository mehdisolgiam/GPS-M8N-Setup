/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* USER CODE END Includes */


/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */


/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define GPS_BUFFER_SIZE 128

/* USER CODE END PD */


/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */


/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart1;


/* USER CODE BEGIN PV */

uint8_t msg = 0;

uint8_t gpsData[GPS_BUFFER_SIZE];
uint16_t index = 0;

bool receivingGPS = false;
bool gpsValid = false;
bool situationGNGGA = false;

char token[10][20];

float latitudeRaw = 0.0f;
float longitudeRaw = 0.0f;

float latitude = 0.0f;
float longitude = 0.0f;

int fixQuality = 0;
int satellites = 0;
float hdop = 0.0f;
float altitude = 0.0f;

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);


/* USER CODE BEGIN PFP */

void getAllChars(void);
void tokenizeGPSData(void);

float convertLatitude(float raw);
float convertLongitude(float raw);

void processGNGGA(void);

/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
 * ---------------------------------------------------------
 * GPS NMEA TOKENIZER
 * ---------------------------------------------------------
 *
 * GNGGA format:
 *
 * token[0] = $GNGGA
 * token[1] = UTC
 * token[2] = Latitude
 * token[3] = N/S
 * token[4] = Longitude
 * token[5] = E/W
 * token[6] = Fix Quality
 * token[7] = Satellites
 * token[8] = HDOP
 * token[9] = Altitude
 *
 */

void tokenizeGPSData(void)
{
    int tokenIndex = 0;
    int charIndex = 0;

    /* Clear tokens */

    for (int i = 0; i < 10; i++)
    {
        token[i][0] = '\0';
    }

    /* Split GPS data by comma */

    for (int i = 0;
         gpsData[i] != '\0' && tokenIndex < 10;
         i++)
    {
        if (gpsData[i] == ',')
        {
            token[tokenIndex][charIndex] = '\0';

            tokenIndex++;
            charIndex = 0;
        }
        else
        {
            if (charIndex < 19)
            {
                token[tokenIndex][charIndex] = gpsData[i];
                charIndex++;
            }
        }
    }

    /* End last token */

    if (tokenIndex < 10)
    {
        token[tokenIndex][charIndex] = '\0';
    }
}


/*
 * ---------------------------------------------------------
 * CONVERT NMEA LATITUDE
 * ---------------------------------------------------------
 *
 * Example:
 *
 * 3542.1234
 *
 * becomes:
 *
 * 35.702056
 *
 */

float convertLatitude(float raw)
{
    int degrees;
    float minutes;

    degrees = (int)(raw / 100.0f);

    minutes = raw - (degrees * 100.0f);

    return degrees + (minutes / 60.0f);
}


/*
 * ---------------------------------------------------------
 * CONVERT NMEA LONGITUDE
 * ---------------------------------------------------------
 *
 * Example:
 *
 * 05123.4567
 *
 * becomes:
 *
 * 51.390945
 *
 */

float convertLongitude(float raw)
{
    int degrees;
    float minutes;

    degrees = (int)(raw / 100.0f);

    minutes = raw - (degrees * 100.0f);

    return degrees + (minutes / 60.0f);
}


/*
 * ---------------------------------------------------------
 * PROCESS GNGGA
 * ---------------------------------------------------------
 */

void processGNGGA(void)
{
    /*
     * Make sure this is actually GNGGA
     */

    if (strncmp((char*)gpsData, "$GNGGA", 6) != 0)
    {
        return;
    }

    situationGNGGA = true;

    /*
     * Split NMEA sentence
     */

    tokenizeGPSData();


    /*
     * Check that latitude and longitude exist
     */

    if (strlen(token[2]) == 0 || strlen(token[4]) == 0)
    {
        gpsValid = false;
        return;
    }


    /*
     * Raw Latitude
     */

    latitudeRaw = atof(token[2]);


    /*
     * Raw Longitude
     */

    longitudeRaw = atof(token[4]);


    /*
     * N/S
     */

    char latitudeDirection = token[3][0];


    /*
     * E/W
     */

    char longitudeDirection = token[5][0];


    /*
     * GPS Fix Quality
     */

    fixQuality = atoi(token[6]);


    /*
     * Number of satellites
     */

    satellites = atoi(token[7]);


    /*
     * HDOP
     */

    hdop = atof(token[8]);


    /*
     * Altitude
     */

    altitude = atof(token[9]);


    /*
     * Check GPS Fix
     */

    if (fixQuality > 0)
    {
        gpsValid = true;


        /*
         * Convert NMEA coordinates
         * to decimal degrees
         */

        latitude = convertLatitude(latitudeRaw);

        longitude = convertLongitude(longitudeRaw);


        /*
         * South latitude
         */

        if (latitudeDirection == 'S')
        {
            latitude = -latitude;
        }


        /*
         * West longitude
         */

        if (longitudeDirection == 'W')
        {
            longitude = -longitude;
        }
    }
    else
    {
        gpsValid = false;
    }
}


/*
 * ---------------------------------------------------------
 * RECEIVE GPS CHARACTERS
 * ---------------------------------------------------------
 */

void getAllChars(void)
{
    /*
     * Start of NMEA sentence
     */

    if (msg == '$')
    {
        index = 0;

        receivingGPS = true;
    }


    /*
     * Store characters
     */

    if (receivingGPS)
    {
        if (index < GPS_BUFFER_SIZE - 1)
        {
            gpsData[index] = msg;
            index++;
        }
        else
        {
            /*
             * Buffer overflow
             */
					
            receivingGPS = false;
            index = 0;
        }
    }


    /*
     * End of NMEA sentence
     */

    if (msg == '\n' && receivingGPS)
    {
        /*
         * Null terminate string
         */

        gpsData[index] = '\0';


        /*
         * Process only GNGGA
         */
			
        processGNGGA();
			
        /*
         * Reset buffer
         */
			
        receivingGPS = false;
        index = 0;
    }
}

void gpsLogic(){
	/*
	 * Receive one byte from GPS
	 */

	HAL_StatusTypeDef status;

	status = HAL_UART_Receive(
							&huart1,
							&msg,
							1,
							HAL_MAX_DELAY
					);


	/*
	 * If receiving was successful
	 */

	if (status == HAL_OK)
	{
			getAllChars();
	}
}


/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */

int main(void)
	{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */


  /* MCU Configuration--------------------------------------------------------*/

  HAL_Init();


  /* USER CODE BEGIN Init */

  /* USER CODE END Init */


  /* Configure the system clock */

  SystemClock_Config();


  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */


  /* Initialize all configured peripherals */

  MX_GPIO_Init();

  MX_USART1_UART_Init();


  /* USER CODE BEGIN 2 */

  /*
   * Initialize variables
   */

  msg = 0;

  index = 0;

  receivingGPS = false;

  gpsValid = false;


  /* USER CODE END 2 */


  /* Infinite loop */

  while (1)
  {
		gpsLogic();
  }

}


/**
  * @brief System Clock Configuration
  * @retval None
  */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;

  RCC_OscInitStruct.HSEState = RCC_HSE_ON;

  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;

  RCC_OscInitStruct.HSIState = RCC_HSI_ON;

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;


  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }


  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;


  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;

  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;

  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;


  if (HAL_RCC_ClockConfig(
          &RCC_ClkInitStruct,
          FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */

static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;

  huart1.Init.BaudRate = 9600;

  huart1.Init.WordLength = UART_WORDLENGTH_8B;

  huart1.Init.StopBits = UART_STOPBITS_1;

  huart1.Init.Parity = UART_PARITY_NONE;

  huart1.Init.Mode = UART_MODE_TX_RX;

  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;

  huart1.Init.OverSampling = UART_OVERSAMPLING_16;


  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */

static void MX_GPIO_Init(void)
{

  __HAL_RCC_GPIOC_CLK_ENABLE();

  __HAL_RCC_GPIOD_CLK_ENABLE();

  __HAL_RCC_GPIOA_CLK_ENABLE();

}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */

void Error_Handler(void)
{
  __disable_irq();

  while (1)
  {
  }
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert line number
  * @retval None
  */

void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */