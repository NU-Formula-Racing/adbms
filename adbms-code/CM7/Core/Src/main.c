/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "adbms_driver.h"
#include "adbms_interface_2950.h"
#include "bms.h"
#include "usbd_cdc_if.h"

#if ISOSPI1_PACKBOARD_TEST_ENABLED
/* Keep the last complete report readable over SWD while the next one is built. */
static char isospi1_console_buffers[2][4096];
static char *volatile isospi1_console = isospi1_console_buffers[0];
static char *isospi1_console_pending = isospi1_console_buffers[1];
static size_t isospi1_console_length;
#endif

int _write(int file, char *ptr, int len)
{
  (void)file;
  if (len <= 0 || __get_IPSR() != 0U || !ENABLE_PRINTF_DEBUG_COMMS) return len;
#if ISOSPI1_PACKBOARD_TEST_ENABLED
  {
    const size_t remaining = sizeof(isospi1_console_buffers[0]) - isospi1_console_length - 1U;
    const size_t count = (size_t)len < remaining ? (size_t)len : remaining;
    memcpy(isospi1_console_pending + isospi1_console_length, ptr, count);
    isospi1_console_length += count;
    isospi1_console_pending[isospi1_console_length] = '\0';
  }
#endif
#if ENABLE_USB_COMMS
  CDC_Log_Write_FS((const uint8_t *)ptr, (uint32_t)len);
#endif
  (void)ptr;
  return len;
}
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* DUAL_CORE_BOOT_SYNC_SEQUENCE: Define for dual core boot synchronization    */
/*                             demonstration code based on hardware semaphore */
/* This define is present in both CM7/CM4 projects                            */
/* To comment when developping/debugging on a single core                     */
//#define DUAL_CORE_BOOT_SYNC_SEQUENCE

#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

FDCAN_HandleTypeDef hfdcan1;

SD_HandleTypeDef hsd1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */
#if ISOSPI1_PACKBOARD_TEST_ENABLED
/* Inspect these fields in the debugger during the single-packboard test. */
static volatile struct
{
  HAL_StatusTypeDef spi_status;
  uint32_t attempts;
  uint32_t good_reads;
  uint32_t pec_errors;
  uint32_t spi_errors;
  GPIO_PinState pd10_idle;
  GPIO_PinState pd10_after_wake;
  uint8_t response[DATA_LEN + PEC_LEN];
} isospi1_test;
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_SPI2_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ---- Reboot into the ROM USB DFU bootloader (flash over USB, no ST-LINK / BOOT0) ----
   Host opens the USB COM port at 1200 baud (or types "dfu") -> the app stores a magic
   value in .noinit RAM and resets -> the first thing main() does is see the magic and
   jump to the system bootloader, while the chip is still in its post-reset state.
   STM32H74x/75x system bootloader: 0x1FF09800 (AN2606; verified on this board). */
#define SYSTEM_BOOTLOADER_ADDR 0x1FF09800UL
#define BOOTLOADER_MAGIC       0xB007D00DUL
static volatile uint32_t bootloader_magic __attribute__((section(".noinit")));

static void Jump_To_Bootloader_If_Requested(void)
{
  if (bootloader_magic != BOOTLOADER_MAGIC) return;
  bootloader_magic = 0U; /* one shot: the next reset boots the app normally */

  __disable_irq();
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL = 0U;
  for (uint32_t i = 0U; i < 8U; i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFFU;
    NVIC->ICPR[i] = 0xFFFFFFFFU;
  }
  SCB->VTOR = SYSTEM_BOOTLOADER_ADDR;
  __DSB();
  __ISB();
  __enable_irq();

  const uint32_t boot_sp = *(volatile const uint32_t *)SYSTEM_BOOTLOADER_ADDR;
  const uint32_t boot_pc = *(volatile const uint32_t *)(SYSTEM_BOOTLOADER_ADDR + 4U);
  /* Load the bootloader's stack pointer and branch to its reset handler. */
  __asm volatile ("msr msp, %0\n\tbx %1" : : "r" (boot_sp), "r" (boot_pc) : "memory");
  while (1) { }
}

/* Call from the main loop: if the host requested DFU, reset into the bootloader. */
static void Service_Bootloader_Request(void)
{
#if ENABLE_USB_COMMS
  if (!CDC_Bootloader_Requested_FS()) return;
  /* Let the USB control transfer that carried the request finish before dropping off the bus. */
  const uint32_t start = HAL_GetTick();
  while ((HAL_GetTick() - start) < 50U) { }
  bootloader_magic = BOOTLOADER_MAGIC;
  __DSB();
  NVIC_SystemReset();
#endif
}

static void Wait_TIM2(uint32_t milliseconds, bool service_usb)
{
  /* APB1 uses DIV1 and TIM2 has prescaler 0, so its counter runs at PCLK1. */
  const uint32_t ticks_per_ms = HAL_RCC_GetPCLK1Freq() / 1000U;
  const uint32_t ticks = milliseconds * ticks_per_ms;
  uint32_t next_usb_poll = 0U;
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  for (uint32_t elapsed = 0U; elapsed < ticks; elapsed = __HAL_TIM_GET_COUNTER(&htim2))
  {
    if (ENABLE_USB_COMMS && service_usb && elapsed >= next_usb_poll)
    {
      CDC_Log_Service_FS();
      next_usb_poll = elapsed + ticks_per_ms;
    }
  }
}

void Wait_TIM2_Milliseconds(uint32_t milliseconds)
{
  Wait_TIM2(milliseconds, false);
}

#if ISOSPI1_PACKBOARD_TEST_ENABLED
static void Publish_IsoSPI1_Console(void)
{
  fflush(stdout);
  if (isospi1_console_length == 0U) return;

  char *previous = isospi1_console;
  isospi1_console = isospi1_console_pending;
  isospi1_console_pending = previous;
  isospi1_console_pending[0] = '\0';
  isospi1_console_length = 0U;
}

static void Run_IsoSPI2_Packboard_Test(void)
{
  static adbms_raw_ packboard;
  static adbms_2950_ measurements;
  bool configured = false;
  bool usb_announced = false;
  uint32_t usb_test_counter = 0U;
  packboard.SPI_data.hspi = &hspi2;

  /* One device: command and command PEC, then six data and two PEC bytes. */
  uint8_t tx[CMD_LEN + PEC_LEN + DATA_LEN + PEC_LEN] = {
    (uint8_t)(RDCFGA >> 8), (uint8_t)RDCFGA
  };
  const uint16_t command_pec = Pec15_Calc(CMD_LEN, tx);
  tx[CMD_LEN] = (uint8_t)(command_pec >> 8);
  tx[CMD_LEN + 1U] = (uint8_t)command_pec;

  while (1)
  {
    uint8_t rx[sizeof(tx)] = {0};
    Publish_IsoSPI1_Console();
    Wait_TIM2(1000U, true);
    Service_Bootloader_Request();
    if (ENABLE_USB_COMMS && ENABLE_PRINTF_DEBUG_COMMS && CDC_Log_Ready_FS())
    {
      if (!usb_announced) printf("CM7 USB CDC initialized\n");
      usb_announced = true;
      printf("USB CDC test: %lu\n", (unsigned long)usb_test_counter++);
    }
    else
    {
      usb_announced = false;
    }
    isospi1_test.attempts++;
    if (ENABLE_PRINTF_DEBUG_COMMS)
      printf("CM7 bench | isoSPI 2 attempt %lu\n", (unsigned long)isospi1_test.attempts);
    isospi1_test.pd10_idle = HAL_GPIO_ReadPin(AD6822_State_GPIO_Port, AD6822_State_Pin);
    ADBMS_WakeUP_ICs();
    isospi1_test.pd10_after_wake = HAL_GPIO_ReadPin(AD6822_State_GPIO_Port, AD6822_State_Pin);

    HAL_GPIO_WritePin(SPI_CSB_GPIO_Port, SPI_CSB_Pin, GPIO_PIN_RESET);
    const HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi2, tx, rx, sizeof(tx), 10U);
    HAL_GPIO_WritePin(SPI_CSB_GPIO_Port, SPI_CSB_Pin, GPIO_PIN_SET);
    isospi1_test.spi_status = status;
    if (ENABLE_PRINTF_DEBUG_COMMS)
      printf("PD10 status: idle=%u after CS wake=%u\n",
             (unsigned)isospi1_test.pd10_idle, (unsigned)isospi1_test.pd10_after_wake);

    if (status != HAL_OK)
    {
      isospi1_test.spi_errors++;
      configured = false;
      if (ENABLE_PRINTF_DEBUG_COMMS)
        printf("isoSPI 2 / ADBMS2950 SPI error (HAL status %d)\n", (int)status);
      continue;
    }

    uint8_t *reply = rx + CMD_LEN + PEC_LEN;
    for (size_t i = 0; i < sizeof(isospi1_test.response); i++)
    {
      isospi1_test.response[i] = reply[i];
    }

    const uint16_t received_pec = ((uint16_t)(reply[DATA_LEN] & 0x03U) << 8)
                                  | reply[DATA_LEN + 1U];
    if (Pec10_Calc(true, DATA_LEN, reply) != received_pec)
    {
      isospi1_test.pec_errors++;
      configured = false;
      if (ENABLE_PRINTF_DEBUG_COMMS)
        printf("isoSPI 2 / ADBMS2950 CFGA PEC FAIL (RX=%03X)\n", (unsigned)received_pec);
      continue;
    }

    isospi1_test.good_reads++;

    printf("\nisoSPI 2 / ADBMS2950 CFGA PEC OK (valid replies %lu)\n",
           (unsigned long)isospi1_test.good_reads);
    if (configured)
    {
      /* Reconfigure if a packboard reset cleared OCEN, REFUP, or ACCI. */
      const uint8_t *expected = packboard.command_bit.cfg_a;
      configured = ((reply[0] ^ expected[0]) & 0x80U) == 0U
                   && ((reply[5] ^ expected[5]) & 0x17U) == 0U;
    }
    /* Console output may have lasted long enough for the link to go idle. */
    ADBMS_WakeUP_ICs();
    if (!configured)
    {
      /* Use the packboard's existing ADC and sense input configuration. */
      command_parameters_2950_ *params = &packboard.command_parameters.parameter_2950;
      config_command_bits_ *commands = &packboard.command_bit;
      ADBMS_2950_Config(params, commands);
      ADBMS_joint_Config(&packboard.command_parameters.parameter_joint, commands);
      ADBMS_Write_Data(&hspi2, WRCFGB, commands->cfg_b, packboard.SPI_data.spi_dataBuf);
      params->cfa2950.ocen = 0;
      ADBMS_Set_Config_A_2950(&params->cfa2950, commands->cfg_a, NUM_2950, POSITION_2950);
      ADBMS_Write_Data(&hspi2, WRCFGA, commands->cfg_a, packboard.SPI_data.spi_dataBuf);
      Wait_TIM2_Milliseconds(1U);
      params->cfa2950.ocen = 1;
      ADBMS_Set_Config_A_2950(&params->cfa2950, commands->cfg_a, NUM_2950, POSITION_2950);
      ADBMS_Write_Data(&hspi2, WRCFGA, commands->cfg_a, packboard.SPI_data.spi_dataBuf);
      ADBMS_Write_CMD(&hspi2, commands->adcv);
      ADBMS_Write_CMD(&hspi2, commands->adsv);
      ADBMS_Write_CMD(&hspi2, commands->adv);
      configured = true;
      /* Read on the next pass, after the ADCs have settled. */
      continue;
    }

    adbms_read_raw_ *acc = &packboard.read_raw_2950_filtered;
    ADBMS_Read_Voltages(acc, Accumulate_Read_2950, &hspi2, packboard.SPI_data.spi_dataBuf);
    const bool accumulator_pec_failed = ADBMS_2950_Pec_Failure(acc, &measurements);
    Update_2950_InternalFault(&measurements);
    if (accumulator_pec_failed)
    {
      printf("ADBMS2950 accumulated current/voltage PEC FAIL\n");
    }
    else
    {
      const uint8_t acci = packboard.command_parameters.parameter_2950.cfa2950.acci;
      ADBMS_2950_Calculate_Accumulator_Vbat(acc, &measurements, acci);
      ADBMS_2950_Calculate_Accumulator_Current(acc, &measurements, acci);
      ADBMS_2950_Print_Vals(&measurements);
    }
    ADBMS_2950_Print_Raw_ADC(&packboard);
  }
}
#endif

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* Must be first: enter the USB DFU bootloader if the app asked for it before resetting. */
  Jump_To_Bootloader_If_Requested();

  /* SysTick (HAL_GetTick/HAL_Delay), USB and FDCAN IRQs must use the CM7 application
     vector table. Without this, HAL_Delay hangs forever. Must run in every build mode. */
  extern const uint32_t g_pfnVectors[];
  SCB->VTOR = (uint32_t)g_pfnVectors;
  __DSB();
  __ISB();

  /* USER CODE END 1 */
/* USER CODE BEGIN Boot_Mode_Sequence_0 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  int32_t timeout;
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_0 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
  Error_Handler();
  }
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();
/* USER CODE BEGIN Boot_Mode_Sequence_2 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
/* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
HSEM notification */
/*HW semaphore Clock enable*/
__HAL_RCC_HSEM_CLK_ENABLE();
/*Take HSEM */
HAL_HSEM_FastTake(HSEM_ID_0);
/*Release HSEM in order to notify the CPU2(CM4)*/
HAL_HSEM_Release(HSEM_ID_0,0);
/* wait until CPU2 wakes up from stop mode */
timeout = 0xFFFF;
while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
if ( timeout < 0 )
{
Error_Handler();
}
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_2 */

  /* USER CODE BEGIN SysInit */
  /* Run pin tests before USB and other peripheral initialization. */
  MX_GPIO_Init();
  MX_TIM2_Init();

  if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
#if ISOSPI1_PACKBOARD_TEST_ENABLED
#if ENABLE_USB_COMMS
  /* The USB HAL uses SysTick for mode switching and timeout checks. */
  const uint32_t usb_tick_start = HAL_GetTick();
  Wait_TIM2_Milliseconds(3U);
  if (HAL_GetTick() != usb_tick_start) MX_USB_DEVICE_Init();
#endif
  MX_SPI2_Init();
  Run_IsoSPI2_Packboard_Test();
#endif
#if CONTACTOR_PULSE_TEST_ENABLED
  /* Start PB7/PB8 (5 s) and PB9 (2 s) HIGH together, once per reset. */
  HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin|CONTACTOR_P_CTRL_Pin
                          |CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_SET);
#endif
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  Wait_TIM2_Milliseconds(2000U);

  GPIO_InitTypeDef pb6_input = {0};
  pb6_input.Pin = GPIO_PIN_6;
  pb6_input.Mode = GPIO_MODE_INPUT;
  pb6_input.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &pb6_input);

#if CONTACTOR_PULSE_TEST_ENABLED
  /* Two seconds elapsed: PB9 goes LOW; PB7/PB8 stay HIGH for 3 s more. */
  HAL_GPIO_WritePin(CONTACTOR_PRE_CTRL_GPIO_Port, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
  Wait_TIM2_Milliseconds(3000U);
  HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin|CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
  Wait_TIM2_Milliseconds(2000U);
#endif

  HAL_TIM_Base_Stop(&htim2);

#if CONTACTOR_PULSE_TEST_ENABLED
  /* Test complete: leave PB7/PB8/PB9 LOW until reset, without BMS writes. */
  while (1)
  {
    __NOP();
  }
#endif

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  //i am commenting this out so it doesn't throw me an error
  //MX_SDMMC1_SD_Init();


  MX_SPI2_Init();
  /* FDCAN before USB so a USB init problem can never block CAN. */
  MX_FDCAN1_Init();
#if ENABLE_USB_COMMS
  {
    /* USB init spins on HAL_Delay; only start it if the HAL tick is actually running. */
    const uint32_t usb_tick_start = HAL_GetTick();
    HAL_TIM_Base_Start(&htim2);
    Wait_TIM2_Milliseconds(3U);
    HAL_TIM_Base_Stop(&htim2);
    if (HAL_GetTick() != usb_tick_start) MX_USB_DEVICE_Init();
  }
#endif
  MX_FATFS_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  Bms_Mainboard_Setup(&hspi2, &hfdcan1);
  HAL_TIM_Base_Start_IT(&htim4);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  Tick_Mainboard_Timers();
    if (ENABLE_USB_COMMS) CDC_Log_Service_FS();
    Service_Bootloader_Request();
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 9;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
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
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CKPER;
  PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 10;
  hfdcan1.Init.NominalSyncJumpWidth = 8;
  hfdcan1.Init.NominalTimeSeg1 = 8;
  hfdcan1.Init.NominalTimeSeg2 = 1;
  hfdcan1.Init.DataPrescaler = 10;
  hfdcan1.Init.DataSyncJumpWidth = 8;
  hfdcan1.Init.DataTimeSeg1 = 8;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.MessageRAMOffset = 0;
  hfdcan1.Init.StdFiltersNbr = 20;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.RxFifo0ElmtsNbr = 32;
  hfdcan1.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.RxFifo1ElmtsNbr = 0;
  hfdcan1.Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.RxBuffersNbr = 0;
  hfdcan1.Init.RxBufferSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.TxEventsNbr = 0;
  hfdcan1.Init.TxBuffersNbr = 0;
  hfdcan1.Init.TxFifoQueueElmtsNbr = 32;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  hfdcan1.Init.TxElmtSize = FDCAN_DATA_BYTES_8;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 10;
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */
#if ISOSPI1_PACKBOARD_TEST_ENABLED
  /* HAL_SPI_Init leaves SPI disabled. Use 1 MHz SCK for the bench test. */
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  MODIFY_REG(hspi2.Instance->CFG1, SPI_CFG1_MBR, hspi2.Init.BaudRatePrescaler);
#endif

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 200;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
#if ISOSPI1_PACKBOARD_TEST_ENABLED
  HAL_GPIO_WritePin(SPI_CSB_GPIO_Port, SPI_CSB_Pin, GPIO_PIN_SET);
#else
  HAL_GPIO_WritePin(SPI_CSB_GPIO_Port, SPI_CSB_Pin, GPIO_PIN_RESET);
#endif

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_LED_3_Pin|GPIO_LED_2_Pin|GPIO_LED_1_Pin, GPIO_PIN_RESET);

  /* Hold PD7 (BMS_STATUS_OUT) low from GPIO initialization onward. */
  HAL_GPIO_WritePin(BMS_STATUS_OUT_GPIO_Port, BMS_STATUS_OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin|CONTACTOR_P_CTRL_Pin
                          |CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);

  /* STM light test: PB4 (TSSI_G) and PB5 (TSSI_R) high. */
  HAL_GPIO_WritePin(GPIOB, TSSI_G_Pin|TSSI_R_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SD_CARD_DETECT_Pin */
  GPIO_InitStruct.Pin = SD_CARD_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(SD_CARD_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI_CSB_Pin */
  GPIO_InitStruct.Pin = SPI_CSB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI_CSB_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GPIO_LED_3_Pin GPIO_LED_2_Pin GPIO_LED_1_Pin */
  GPIO_InitStruct.Pin = GPIO_LED_3_Pin|GPIO_LED_2_Pin|GPIO_LED_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : Shutdown_Contactors_Pin AD6822_State_Pin IMD_STATUS_IN_Pin */
  GPIO_InitStruct.Pin = Shutdown_Contactors_Pin|AD6822_State_Pin|IMD_STATUS_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : BMS_STATUS_OUT_Pin */
  GPIO_InitStruct.Pin = BMS_STATUS_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BMS_STATUS_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TSSI_G_Pin TSSI_R_Pin CONTACTOR_N_CTRL_Pin CONTACTOR_P_CTRL_Pin
                           CONTACTOR_PRE_CTRL_Pin */
  GPIO_InitStruct.Pin = TSSI_G_Pin|TSSI_R_Pin|CONTACTOR_N_CTRL_Pin|CONTACTOR_P_CTRL_Pin
                          |CONTACTOR_PRE_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* Configure PB6 low; main() runs the timed test. */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
