
#include <stdint.h>

void motor_init(uint32_t tail_pin_nr, uint32_t head_pin_nr, uint32_t mouth_pin_nr);

void billy_pwm_head_toggle(void);
void billy_pwm_tail_out(void);
void billy_pwm_tail_in(void);
void billy_pwm_mouth_open(void);
void billy_pwm_mouth_close(void);
