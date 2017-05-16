/**
 ******************************************************************************
 *  FILE          : soundDriver.c
 *  PROJECT       : PROG 8125 - Assignment #4
 *  PROGRAMMER    : Artem Bordiuh
 *  FIRST VERSION : 2017-04-04
 *  DESCRIPTION   : Initialization of the sound driver which is basicly 
 *					TIM1 Channel 1 configuration as PWM output on PA7 and
 *					speaker connected to PA7		
 *	IMPROVMENTS   : Add notes list and function for playing them 
					
 ******************************************************************************/
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
#include "stm32_hal_legacy.h"

/* Timer 1 configuration structure*/
TIM_HandleTypeDef htim1;

/**
 * @brief Initialize the sound driver - timer 1 for pwm output 
 * on chanel 1.
 * Original author: Rudy Hofer
 * Editted by: Artem Bordiuh
 * @param TIM_HandleTypeDef: pointer to the TIM_HandleTypeDef type
 * @param int32_t: initial frequency of the timer
 * @retval None
 */
void initSoundDriver(TIM_HandleTypeDef* htim1, int32_t freq) {
	int32_t	pwmPeriod = 1000000000/(freq * 250);
	TIM_MasterConfigTypeDef sMasterConfig;
	TIM_OC_InitTypeDef sConfigOC;
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;
	htim1->Instance = TIM1;
	htim1->Init.Prescaler = 0;
	htim1->Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1->Init.Period = pwmPeriod;
	htim1->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1->Init.RepetitionCounter = 0;
	if (HAL_TIM_PWM_Init(&*htim1) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&*htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* adding this as ST Tech Support said PWM should be stopped before
	 * calling HAL_TIM_PWM_ConfigChannel and I've been getting flakey start-up
	 * i.e.: sometime PWM starts up, other times the line remains stuck high.
	 **************************************/
	HAL_TIM_PWM_Stop(&*htim1, TIM_CHANNEL_1);
	/*************************************/
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = pwmPeriod / 2;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&*htim1, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.BreakFilter = 0;
	sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
	sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
	sBreakDeadTimeConfig.Break2Filter = 0;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&*htim1, &sBreakDeadTimeConfig)
			!= HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief Will turn on PWM on chanel 1 of TIM1 with required frequency and
 * duration (PA7 stm32 pin)
 * @param uint32_t: Frequency of the PWM (sound signal)
 * @param uint32_t: Duration of the turned on PWM (sound signal)
 * @retval None
 */
void playNote( uint32_t freq, uint32_t duration){
	//Set up autoreload which is frequency
	__HAL_TIM_SetAutoreload(&htim1,freq);
	HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_1);
	//Delay which is duration
	HAL_Delay(duration);
	//Turn On PWM on channel 1 of TIM 1
	HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_1);
}


