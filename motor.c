
#include <stdbool.h>

#include "nrf.h"

#define PWM_FREQUENCY	20000
#define PWM_TOP     (16000000/PWM_FREQUENCY/2)        //PWM base clock is 16MHz

#define SEQ_LENGTH		200
#define SEQ_REFRESH		99

static uint16_t pwm_seq_acc[SEQ_LENGTH] = {0};
static uint16_t pwm_seq_deacc[SEQ_LENGTH] = {0};

static bool tail_is_out = false;
static bool head_is_out = false;
static bool mouth_is_open = false;

void motor_init(uint32_t tail_pin_nr, uint32_t head_pin_nr, uint32_t mouth_pin_nr)
{
	
	//fill up the acceleration sequence and the deacceleration sequence
	for(uint16_t i = 0; i < SEQ_LENGTH; i++)
	{
		pwm_seq_acc[i] = (i * PWM_TOP / (SEQ_LENGTH - 1)) | 0x8000;
		pwm_seq_deacc[i] = (PWM_TOP - i * PWM_TOP / (SEQ_LENGTH - 1)) | 0x8000;
	}
	
	//PWM0
	NRF_PWM0->PSEL.OUT[0] = (tail_pin_nr << PWM_PSEL_OUT_PIN_Pos) | 
                            (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
	
    NRF_PWM0->ENABLE      = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);
    NRF_PWM0->MODE        = (PWM_MODE_UPDOWN_UpAndDown << PWM_MODE_UPDOWN_Pos);
    NRF_PWM0->PRESCALER   = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);
    NRF_PWM0->COUNTERTOP  = (PWM_TOP << PWM_COUNTERTOP_COUNTERTOP_Pos);
    NRF_PWM0->LOOP        = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);
    NRF_PWM0->DECODER   = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | 
                          (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
    
    
    NRF_PWM0->SEQ[0].PTR  = ((uint32_t)(pwm_seq_acc) << PWM_SEQ_PTR_PTR_Pos);
    NRF_PWM0->SEQ[0].CNT  = ((sizeof(pwm_seq_acc) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);
    NRF_PWM0->SEQ[0].REFRESH  = SEQ_REFRESH;
    NRF_PWM0->SEQ[0].ENDDELAY = 0;
	
	//PWM1
	NRF_PWM1->PSEL.OUT[0] = (head_pin_nr << PWM_PSEL_OUT_PIN_Pos) | 
                            (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
	
    NRF_PWM1->ENABLE      = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);
    NRF_PWM1->MODE        = (PWM_MODE_UPDOWN_UpAndDown << PWM_MODE_UPDOWN_Pos);
    NRF_PWM1->PRESCALER   = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);
    NRF_PWM1->COUNTERTOP  = (PWM_TOP << PWM_COUNTERTOP_COUNTERTOP_Pos);
    NRF_PWM1->LOOP        = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);
    NRF_PWM1->DECODER   = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | 
                          (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
    
    
    NRF_PWM1->SEQ[0].PTR  = ((uint32_t)(pwm_seq_acc) << PWM_SEQ_PTR_PTR_Pos);
    NRF_PWM1->SEQ[0].CNT  = ((sizeof(pwm_seq_acc) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);
    NRF_PWM1->SEQ[0].REFRESH  = SEQ_REFRESH;
    NRF_PWM1->SEQ[0].ENDDELAY = 0;
	
	//PWM2
	NRF_PWM2->PSEL.OUT[0] = (mouth_pin_nr << PWM_PSEL_OUT_PIN_Pos) | 
                            (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
	
    NRF_PWM2->ENABLE      = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);
    NRF_PWM2->MODE        = (PWM_MODE_UPDOWN_UpAndDown << PWM_MODE_UPDOWN_Pos);
    NRF_PWM2->PRESCALER   = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);
    NRF_PWM2->COUNTERTOP  = (PWM_TOP << PWM_COUNTERTOP_COUNTERTOP_Pos);
    NRF_PWM2->LOOP        = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);
    NRF_PWM2->DECODER   = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | 
                          (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
    
    
    NRF_PWM2->SEQ[0].PTR  = ((uint32_t)(pwm_seq_acc) << PWM_SEQ_PTR_PTR_Pos);
    NRF_PWM2->SEQ[0].CNT  = ((sizeof(pwm_seq_acc) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);
    NRF_PWM2->SEQ[0].REFRESH  = SEQ_REFRESH;
    NRF_PWM2->SEQ[0].ENDDELAY = 0;
}

void billy_pwm_head_toggle(void)
{
	if(head_is_out)
	{
		NRF_PWM1->SEQ[0].PTR  = ((uint32_t)(pwm_seq_deacc) << PWM_SEQ_PTR_PTR_Pos);
		NRF_PWM1->TASKS_SEQSTART[0] = 1;
	}
	else
	{
		NRF_PWM1->SEQ[0].PTR  = ((uint32_t)(pwm_seq_acc) << PWM_SEQ_PTR_PTR_Pos);
		NRF_PWM1->TASKS_SEQSTART[0] = 1;
	}
}

void billy_pwm_tail_out(void)
{
	if(!tail_is_out)
	{
		NRF_PWM0->SEQ[0].PTR  = ((uint32_t)(pwm_seq_acc) << PWM_SEQ_PTR_PTR_Pos);
		NRF_PWM0->TASKS_SEQSTART[0] = 1;
		tail_is_out = true;
	}
}

void billy_pwm_tail_in(void)
{
	if(tail_is_out)
	{
		NRF_PWM0->SEQ[0].PTR  = ((uint32_t)(pwm_seq_deacc) << PWM_SEQ_PTR_PTR_Pos);
		NRF_PWM0->TASKS_SEQSTART[0] = 1;
		tail_is_out = false;
	}
}

void billy_pwm_mouth_open(void)
{
	if(!mouth_is_open)
	{
		NRF_PWM2->SEQ[0].PTR  = ((uint32_t)(pwm_seq_acc) << PWM_SEQ_PTR_PTR_Pos);
		NRF_PWM2->TASKS_SEQSTART[0] = 1;
		mouth_is_open = true;
	}
}

void billy_pwm_mouth_close(void)
{
	if(mouth_is_open)
	{
		NRF_PWM2->SEQ[0].PTR  = ((uint32_t)(pwm_seq_deacc) << PWM_SEQ_PTR_PTR_Pos);
		NRF_PWM2->TASKS_SEQSTART[0] = 1;
		mouth_is_open = false;
	}
}


