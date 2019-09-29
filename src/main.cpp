#include <stdio.h> // NOLINT(modernize-deprecated-headers,hicpp-deprecated-headers)
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "digital.h"

#define UART_BAUD 9600
#define CLOCK_SPEED 1000
#define TICK_SPEED 63974

#define write_data(byte) (PORTL = byte)
#define read_address() (((uint16_t)PINA << 8u) | PINC)

static IOPort PortA(&PORTA, &PINA, &DDRA);
static IOPort PortB(&PORTB, &PINB, &DDRB);
static IOPort PortC(&PORTC, &PINC, &DDRC);
static IOPort PortD(&PORTD, &PIND, &DDRD);
static IOPort PortE(&PORTE, &PINE, &DDRE);
static IOPort PortF(&PORTF, &PINF, &DDRF);
static IOPort PortG(&PORTG, &PING, &DDRG);
static IOPort PortH(&PORTH, &PINH, &DDRH);
static IOPort PortJ(&PORTJ, &PINJ, &DDRJ);
static IOPort PortK(&PORTK, &PINK, &DDRK);
static IOPort PortL(&PORTL, &PINL, &DDRL);

#define ADDR_H PortA
#define ADDR_L PortC

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

#define DATA PortL
#define DATA_L_0  PL7   // D42
#define DATA_L_1  PL6   // D43
#define DATA_L_2  PL5   // D44
#define DATA_L_3  PL4   // D45
#define DATA_L_4  PL3   // D46
#define DATA_L_5  PL2   // D47
#define DATA_L_6  PL1   // D48
#define DATA_L_7  PL0   // D49

#define HALT   (PortE[PE4])
#define WAIT   (PortE[PE5])
#define INT    (PortG[PG5])
#define NMI    (PortE[PE3])
#define RESET  (PortH[PH3])

#define M1     (PortH[PH4])
#define MREQ   (PortH[PH5])
#define IORQ   (PortH[PH6])
#define RD     (PortB[PB4])
#define WR     (PortB[PB5])
#define RFSH   (PortB[PB6])

#define CLK    (PortB[PB7])

#define BUSRQ  (PortB[PB1])
#define BUSACK (PortB[PB0])

#define Z80_NOP (0b00000000)

void setup_z80() {
  // Setup address bus
  PortA.setPortDirection(PinDirection::Input);
  PortC.setPortDirection(PinDirection::Input);

  // Setup data bus
  PortL.setPortDirection(PinDirection::Output);

  // Setup CPU Control
  HALT.setInput();
  WAIT.setOutput();
  NMI.setOutput();
  INT.setOutput();
  RESET.setOutput();

  // Setup CPU bus control
  BUSACK.setInput();
  BUSRQ.setOutput();

  // Setup Clock
  CLK.setOutput();

  // Setup system control
  M1.setInput();
  MREQ.setInput();
  IORQ.setInput();
  RD.setInput();
  WR.setInput();
  RFSH.setInput();
}

static int tick_count = 0;

ISR (TIMER1_OVF_vect) {
  tick_count++;
  if (tick_count > 5) {
    CLK.toggle();
    tick_count = 0;
  }
  TCNT1 = TICK_SPEED;
}

void boot() {
  INT.setHigh();
  NMI.setHigh();
  WAIT.setHigh();
  BUSRQ.setHigh();

  // cout << P("Rebooting..") << endl;

  RESET.setHigh();
  _delay_ms(1000);
  printf_P(PSTR("Rebooting.."));
  RESET.setLow();
  _delay_ms(CLOCK_SPEED * 15);
  RESET.setHigh();
  printf_P(PSTR("Rebooted..\n"));
}

int uart_putchar(char c, FILE *stream) {
  if (c == '\a') {
    fputs("*ring*\n", stderr);
    return 0;
  }

  if (c == '\n') { uart_putchar('\r', stream); }

  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;

  return 0;
}

FILE uart_str = { // NOLINT(cert-fio38-c,misc-non-copyable-objects)
    .flags = _FDEV_SETUP_WRITE,
    .put = uart_putchar,
    .get = nullptr,
    .udata = 0
};

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
  TCCR1B |= (1u << (unsigned)CS10)
         |  (1u << (unsigned)CS12);
  TIMSK1 |= (1u << (unsigned)TOIE1);
  sei();

  setup_z80();
  boot();

  write_data(Z80_NOP);

  uint16_t addrq = 0;
  addrq = read_address();
  printf_P(PSTR("address=0x%04x\n"), addrq);

  int z = 0;


  while (1) {
    _delay_ms(1000);

    printf_P(PSTR("address: "));
    for (int i = 0; i < 8; i++) {
      if (bit_is_set(PINA, i)) {
        printf_P(PSTR("1"));
      } else {
        printf_P(PSTR("0"));
      }
    }
    printf_P(PSTR("_"));
    for (int i = 0; i < 8; i++) {
      if (bit_is_set(PINC, i)) {
        printf_P(PSTR("1"));
      } else {
        printf_P(PSTR("0"));
      }
    }
    printf_P(PSTR("\n"));


    //addrq = read_address();
    //printf_P(PSTR("address=0x%04x\n"), addrq);

    if (z == 30) {
      boot();
      write_data(Z80_NOP);
      z = 0;
    }
    z++;
  }

  return 0;
}
