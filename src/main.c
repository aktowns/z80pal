/*
 * Dummy environment for z80, boots and feeds nops
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define UART_BAUD 9600
#define CLOCK_SPEED 1000
#define TICK_SPEED 63974

#define write_data(byte) (PORTL = byte)
#define read_address() ((uint16_t)(PINA | (PINC << 8)))

#define ADDR_A_0  PA0   // D22
#define ADDR_A_1  PA1   // D23
#define ADDR_A_2  PA2   // D24
#define ADDR_A_3  PA3   // D25
#define ADDR_A_4  PA4   // D26
#define ADDR_A_5  PA5   // D27
#define ADDR_A_6  PA6   // D28
#define ADDR_A_7  PA7   // D29
#define ADDR_C_8  PC7   // D30
#define ADDR_C_9  PC6   // D31
#define ADDR_C_10 PC5   // D32
#define ADDR_C_11 PC4   // D33
#define ADDR_C_12 PC3   // D34
#define ADDR_C_13 PC2   // D35
#define ADDR_C_14 PC1   // D36
#define ADDR_C_15 PC0   // D37

#define DATA_L_0  PL7   // D42
#define DATA_L_1  PL6   // D43
#define DATA_L_2  PL5   // D44
#define DATA_L_3  PL4   // D45
#define DATA_L_4  PL3   // D46
#define DATA_L_5  PL2   // D47
#define DATA_L_6  PL1   // D48
#define DATA_L_7  PL0   // D49

#define HALT_E    PE4   // D2
#define WAIT_E    PE5   // D3
#define INT_G     PG5   // D4
#define NMI_E     PE3   // D5
#define RESET_H   PH3   // D6

#define M1_H      PH4   // D7
#define MREQ_H    PH5   // D8
#define IORQ_H    PH6   // D9
#define RD_B      PB4   // D10
#define WR_B      PB5   // D11
#define RFSH_B    PB6   // D12

#define CLK_B     PB7   // D13

#define BUSRQ_B   PB1   // D52
#define BUSACK_B  PB0   // D53

#define Z80_NOP (0b00000000)

void setup_z80() {
  DDRB = 0;
  DDRE = 0;
  DDRG = 0;
  DDRH = 0;

  // Setup address bus
  DDRA = 0;
  DDRC = 0;

  // Setup data bus
  DDRL = UINT8_MAX;

  // Setup CPU Control
  DDRE |= (0<<HALT_E)|(1<<WAIT_E)|(1<<NMI_E);
  DDRG |= (1<<INT_G);
  DDRH |= (1<<RESET_H);

  // Setup CPU bus control
  DDRB |= (0<<BUSACK_B)|(1<<BUSRQ_B);

  // Setup Clock
  DDRB |= (1<<CLK_B);

  // Setup system control
  DDRH |= (0<<M1_H)|(0<<MREQ_H)|(0<<IORQ_H);
  DDRB |= (0<<RD_B)|(0<<WR_B)|(0<<RFSH_B);
}

static int tick_count = 0;

ISR (TIMER1_OVF_vect) {
  tick_count++;
  if (tick_count > 10) {
    PORTB ^= (1 << CLK_B);
    tick_count = 0;
  }
  TCNT1 = TICK_SPEED;
}

void boot() {
  PORTG |= (1<<INT_G);
  PORTE |= (1<<NMI_E)|(1<<WAIT_E);
  PORTB |= (1<<BUSRQ_B);

  _delay_ms(1000);

  PORTH |= (0<<RESET_H);
  _delay_ms(CLOCK_SPEED * 5);
  PORTH |= (1<<RESET_H);
}


int uart_putchar(char c, FILE *stream) {
  if (c == '\a') {
    fputs("*ring*\n", stderr);
    return 0;
  }

  if (c == '\n')
    uart_putchar('\r', stream);

  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;

  return 0;
}

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

int main() {
  // UART setup
  UCSR0A = _BV(U2X0);
  UBRR0L = (F_CPU / (8UL * UART_BAUD)) - 1;
  UCSR0B = _BV(TXEN0);

  stdout = stderr = &uart_str;

  _delay_ms(1000);

  // Clock
  cli();
  TCCR1A = 0x00;
  TCCR1B = 0x00;

  TCNT1 = TICK_SPEED;
  TCCR1B |= (1 << CS10) | (1 << CS12);
  TIMSK1 |= (1 << TOIE1);
  sei();

  setup_z80();
  boot();

  write_data(Z80_NOP);

  uint16_t addrq = 0;
  addrq = read_address();
  printf_P(PSTR("address=0x%02x\n"), addrq);

  int z = 0;
  while (1) {
    _delay_ms(1000);

    addrq = read_address();
    printf_P(PSTR("address=0x%02x\n"), addrq);

    if (z == 30) {
      boot();
      write_data(Z80_NOP);
      z = 0;
    }
    z++;
  }

  return 0;
}
