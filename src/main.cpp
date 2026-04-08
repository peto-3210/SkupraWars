

#include <avr/io.h>
#include <util/delay.h>
#include "Datalink.hpp"
#include "SoftwareTimer.hpp"
//#include "avr8-stub.h"
//#include "app_api.h" // only needed with flash breakpoints

int main(void)
{
    sei();
    DDRB = 0b00111111;
    DDRD = 0b01000000;
    PORTB = 0b00000000;
    uart_begin(8);

    //_delay_ms(500);

    SoftwareTimerPool& pool1 = SoftwareTimerPool::initTimerPool(10);
    //_delay_us(20);
    SoftwareTimer& timer1 = pool1.acquireTimer();
    

  //_delay_ms(2000);
  //debug_init();
  //breakpoint();

  /*while (1) {
    PORTB = 0b00100000;
    _delay_ms(500);
    PORTB = 0b00000000;
    UDR0 = 1;
    _delay_ms(500);
  }*/

  bool toggle = false;
  bool toggle1 = false;
  timer1.startTimerUs(500000);
  uint8_t buffer[1] = {0};


  while(1){
    //_delay_us(10000);
    pool1.tick();
    
    if (timer1.isDone() == true){
      toggle = !toggle;
      timer1.startTimerUs(500000);
    }
    if ((uart_recv(buffer, 1)) == 1 && buffer[0] == 0xcc){
      toggle1 = !toggle1;
      buffer[0] = 0;
      uint16_t a = 0xaabb;
      uart_send((uint8_t*)&a, 2);
    }
    PORTB = (((int)toggle) << 5) | (((int)toggle1) << 4);

    _delay_ms(1);
  }

  return 0;
}

/*
int main(void) {
    uart_init();

    while (1) {
        uint8_t byte = uart_receive();  // unsigned
        //uart_send(byte);                // echo
        uart_send(1);
    }
}*/