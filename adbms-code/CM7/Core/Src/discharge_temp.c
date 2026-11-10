#include "discharge_temp.h"

#define DT_ADC           ADC1
#define DT_CHANNEL       15U          /* PA3 = ADC12_INP15 */
#define DT_GPIO_PORT     GPIOA
#define DT_GPIO_PIN      GPIO_PIN_3
#define DT_TIMEOUT_MS    5U

static bool dt_ready = false;

static bool wait_bits(volatile uint32_t *reg, uint32_t mask, bool want_set, uint32_t timeout_ms)
{
	const uint32_t start = HAL_GetTick();
	while (((*reg & mask) != 0U) != want_set)
	{
		if (HAL_GetTick() - start > timeout_ms) return false;
	}
	return true;
}

void Discharge_Temp_Init(void)
{
	dt_ready = false;

	// PA3 as analog input
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef gpio = {0};
	gpio.Pin = DT_GPIO_PIN;
	gpio.Mode = GPIO_MODE_ANALOG;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DT_GPIO_PORT, &gpio);

	// ADC kernel clock = per_ck (HSI 64 MHz, set in PeriphCommonClock_Config), async /4 -> 16 MHz
	MODIFY_REG(RCC->D3CCIPR, RCC_D3CCIPR_ADCSEL, 2U << RCC_D3CCIPR_ADCSEL_Pos);
	RCC->AHB1ENR |= RCC_AHB1ENR_ADC12EN;
	(void)RCC->AHB1ENR;
	MODIFY_REG(ADC12_COMMON->CCR, ADC_CCR_CKMODE | ADC_CCR_PRESC, 2U << ADC_CCR_PRESC_Pos);

	// leave deep power down, start the ADC voltage regulator
	DT_ADC->CR &= ~ADC_CR_DEEPPWD;
	DT_ADC->CR |= ADC_CR_ADVREGEN;
	HAL_Delay(1);	// > tADCVREG_STUP (10 us)

	// boost for a 16 MHz ADC clock: rev V and later -> BOOST = 10 (12.5..25 MHz), rev Y -> single BOOST bit
	if ((DBGMCU->IDCODE >> DBGMCU_IDCODE_REV_ID_Pos) >= 0x2003U)
		MODIFY_REG(DT_ADC->CR, ADC_CR_BOOST, 2U << ADC_CR_BOOST_Pos);
	else
		DT_ADC->CR |= ADC_CR_BOOST_0;

	// single-ended offset + linearity calibration
	MODIFY_REG(DT_ADC->CR, ADC_CR_ADCALDIF, ADC_CR_ADCALLIN);
	DT_ADC->CR |= ADC_CR_ADCAL;
	if (!wait_bits(&DT_ADC->CR, ADC_CR_ADCAL, false, 100U)) return;

	// 16-bit, single conversion, software trigger, one regular conversion of channel 15
	DT_ADC->CFGR |= ADC_CFGR_OVRMOD;
	DT_ADC->DIFSEL &= ~(1U << DT_CHANNEL);
	DT_ADC->PCSEL |= (1U << DT_CHANNEL);
	DT_ADC->SQR1 = (DT_CHANNEL << ADC_SQR1_SQ1_Pos);	// L = 0 -> 1 conversion
	MODIFY_REG(DT_ADC->SMPR2, 7U << ADC_SMPR2_SMP15_Pos, 7U << ADC_SMPR2_SMP15_Pos);	// 810.5 cycles

	// enable
	DT_ADC->ISR = ADC_ISR_ADRDY;
	DT_ADC->CR |= ADC_CR_ADEN;
	if (!wait_bits(&DT_ADC->ISR, ADC_ISR_ADRDY, true, 10U)) return;

	dt_ready = true;
}

bool Discharge_Temp_Read(float *volts, float *deg_c)
{
	if (!dt_ready) return false;

	DT_ADC->ISR = ADC_ISR_EOC;
	DT_ADC->CR |= ADC_CR_ADSTART;
	if (!wait_bits(&DT_ADC->ISR, ADC_ISR_EOC, true, DT_TIMEOUT_MS)) return false;

	const uint32_t raw = DT_ADC->DR & 0xFFFFU;
	const float v = (float)raw * DISCHARGE_TEMP_VREF / 65535.0f;
	*volts = v;
	*deg_c = (v - DISCHARGE_TEMP_V_AT_0C) / DISCHARGE_TEMP_V_PER_C;
	return true;
}
