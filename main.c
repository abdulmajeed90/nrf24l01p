#include <avr/io.h>
#include "nrf24l01.h"
#define LED_PIN (1<<0)
void toggle_led()
{
    PORTD |= LED_PIN;
	delay_ms(500);
	PORTD &= ~(LED_PIN);
    delay_ms(500);
}

int main (void)
{
    char buf[32];   
    memset(buf, 'h', 32);

    //set LED_PIN to output
    DDRD |= LED_PIN; 

    //init nordic chip
    nrf_init();
    nrf_enable_pipe(NRF_PIPE_0, true);  
    nrf_set_pipe_width(NRF_PIPE_0, 32);
    
    while (1) {
        nrf_send(buf, 32);
        toggle_led();
    }
    return 0;
}
