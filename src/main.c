#include "simba.h"

typedef struct {
  struct pin_driver_t address_bus[16];
  struct pin_driver_t data_bus[8];
  struct {
    struct pin_driver_t m1;
    struct pin_driver_t mreq;
    struct pin_driver_t iorq;
    struct pin_driver_t rd;
    struct pin_driver_t wr;
    struct pin_driver_t rfsh;
  } system_control;
  struct {
    struct pin_driver_t phalt;
    struct pin_driver_t pwait;
    struct pin_driver_t pint;
    struct pin_driver_t pnmi;
    struct pin_driver_t preset;
  } cpu_control;
  struct {
    struct pin_driver_t busrq;
    struct pin_driver_t busack;
  } cpu_bus_control;
  struct pin_driver_t clk;

  uint16_t state;
} z80_t;

#define pin_m1     (1 << 0)
#define pin_mreq   (1 << 1)
#define pin_iorq   (1 << 2)
#define pin_rd     (1 << 3)
#define pin_wr     (1 << 4)
#define pin_rfsh   (1 << 5)
#define pin_halt   (1 << 6)
#define pin_wait   (1 << 7)
#define pin_int    (1 << 7)
#define pin_nmi    (1 << 8)
#define pin_reset  (1 << 9)
#define pin_busrq  (1 << 10)
#define pin_busack (1 << 11)
#define pin_clk    (1 << 12)

#define set_high(z80, pin) (z80->state |= pin)
#define set_low(pinstate, pin) (z80->state &= ~pin)
#define pin_high(pinstate, pin) (pinstate & pin)
#define pin_low(pinstate, pin) (!(pinstate & pin))

void print_pin_state(uint16_t state) {
  std_printf(FSTR("-- pin status --\n"));
  std_printf(FSTR("m1: %i    mreq: %i    iorq: %i\n"), pin_high(state, pin_m1), pin_high(state, pin_mreq),
      pin_high(state, pin_iorq));
  std_printf(FSTR("rd: %i    wr: %i      rfsh: %i\n"), pin_high(state, pin_rd), pin_high(state, pin_wr),
      pin_high(state, pin_rfsh));
  std_printf(FSTR("halt: %i  wait: %i    int: %i\n"), pin_high(state, pin_halt), pin_high(state, pin_wait),
      pin_high(state, pin_int));
  std_printf(FSTR("nmi: %i   reset: %i   busrq: %i\n"), pin_high(state, pin_nmi), pin_high(state, pin_reset),
      pin_high(state, pin_busrq));
  std_printf(FSTR("busack: %i            clk: %i\n"), pin_high(state, pin_busack), pin_high(state, pin_clk));
}

void flush_pin_state(z80_t* z80) {
  pin_write(&z80->system_control.m1, pin_high(z80->state, pin_m1));
  pin_write(&z80->system_control.mreq, pin_high(z80->state, pin_mreq));
  pin_write(&z80->system_control.iorq, pin_high(z80->state, pin_iorq));
  pin_write(&z80->system_control.rd, pin_high(z80->state, pin_rd));
  pin_write(&z80->system_control.wr, pin_high(z80->state, pin_wr));
  pin_write(&z80->system_control.rfsh, pin_high(z80->state, pin_rfsh));
  pin_write(&z80->cpu_control.phalt, pin_high(z80->state, pin_halt));
  pin_write(&z80->cpu_control.pwait, pin_high(z80->state, pin_wait));
  pin_write(&z80->cpu_control.pint, pin_high(z80->state, pin_int));
  pin_write(&z80->cpu_control.pnmi, pin_high(z80->state, pin_nmi));
  pin_write(&z80->cpu_control.preset, pin_high(z80->state, pin_reset));
  pin_write(&z80->cpu_bus_control.busrq, pin_high(z80->state, pin_busrq));
  pin_write(&z80->cpu_bus_control.busack, pin_high(z80->state, pin_busack));
  pin_write(&z80->clk, pin_high(z80->state, pin_clk));
}

z80_t* new_z80() {
  z80_t* z80 = malloc(sizeof(z80_t));

  uint8_t i = 2;

  // cpu control
  std_printf(FSTR("halt: D%i\n"), i);
  pin_init(&z80->cpu_control.phalt, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("wait: D%i\n"), i);
  pin_init(&z80->cpu_control.pwait, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("int: D%i\n"), i);
  pin_init(&z80->cpu_control.pint, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("nmi: D%i\n"), i);
  pin_init(&z80->cpu_control.pnmi, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("reset: D%i\n"), i);
  pin_init(&z80->cpu_control.preset, &pin_device[i++], PIN_OUTPUT);

  // cpu bus control
  std_printf(FSTR("busack: D%i\n"), i);
  pin_init(&z80->cpu_bus_control.busack, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("busrq: D%i\n"), i);
  pin_init(&z80->cpu_bus_control.busrq, &pin_device[i++], PIN_OUTPUT);

  i = 22;

  // addressbus
  std_printf(FSTR("address bus: D%i"), i);
  for (uint8_t z = 0; z < 16; z++, i++)
    pin_init(&z80->address_bus[z], &pin_device[i], PIN_INPUT);
  std_printf(FSTR("-D%i\n"), i-1);

  // databus
  std_printf(FSTR("data bus: D%i"), i);
  for (uint8_t z = 0; z < 8; z++, i++)
    pin_init(&z80->data_bus[z], &pin_device[i], PIN_OUTPUT);
  std_printf(FSTR("-D%i\n"), i-1);

  // clock
  std_printf(FSTR("clk: D%i\n"), i);
  pin_init(&z80->clk, &pin_device[i++], PIN_OUTPUT);

  // system control
  std_printf(FSTR("m1: D%i\n"), i);
  pin_init(&z80->system_control.m1, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("mreq: D%i\n"), i);
  pin_init(&z80->system_control.mreq, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("iorq: D%i\n"), i);
  pin_init(&z80->system_control.iorq, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("rd: D%i\n"), i);
  pin_init(&z80->system_control.rd, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("wr: D%i\n"), i);
  pin_init(&z80->system_control.wr, &pin_device[i++], PIN_OUTPUT);
  std_printf(FSTR("rfsh: D%i\n"), i);
  pin_init(&z80->system_control.rfsh, &pin_device[i++], PIN_OUTPUT);


  return z80;
}

#define CLOCK_SPEED 1000

//static struct event_t clock_event;
static struct timer_t clock_timer;
static struct pin_driver_t led;

static void clock_cb(void *arg_p) {
 // std_printf(FSTR("tick\n"));

  pin_toggle(&((z80_t*)arg_p)->clk);
  pin_toggle(&led);

  //uint32_t mask = 0x01;
  //event_write_isr(&clock_event, &mask, sizeof(mask));
}

void write_data(z80_t* z80, uint8_t byte) {
  std_printf(FSTR("writing: "));
  for (uint8_t i = 0; i < 8; i++) {
    int status = (byte >> 1) & 1;
    std_printf(FSTR("%i to pin %i "), status, i);
    pin_write(&z80->data_bus[i], status);
  }
  std_printf(FSTR("\n"));
}

uint16_t read_address(z80_t* z80) {
  uint16_t out = 0;
  for (uint8_t i = 0; i < 16; i++) {
    int r = pin_read(&z80->address_bus[i]);
    if (r)
      out = out | (1 << i);
  }

}

void clock_start(z80_t* z80) {
  //uint32_t mask;
  struct time_t timeout;

  //event_init(&clock_event);

  timeout.seconds = 1;
  timeout.nanoseconds = 0;

  timer_init(&clock_timer, &timeout, clock_cb, z80, TIMER_PERIODIC);
  timer_start(&clock_timer);
}

void boot(z80_t* z80) {
  set_high(z80, pin_int);
  set_high(z80, pin_nmi);
  pin_write(&z80->cpu_control.pint, 1);
  pin_write(&z80->cpu_control.pnmi, 1);
  pin_write(&z80->cpu_control.pwait, 1);
  pin_write(&z80->cpu_bus_control.busrq, 1);

  thrd_sleep_ms(1000);
  std_printf(FSTR("rebooting z80\n"));
  pin_write(&z80->cpu_control.preset, 0);
  thrd_sleep_ms(CLOCK_SPEED * 5);
  pin_write(&z80->cpu_control.preset, 1);
}

#define Z80_NOP (0b00000000)

int main() {
  struct uart_driver_t uart;

  sys_start();

  uart_module_init();
  uart_init(&uart, &uart_device[0], 38400, NULL, 0);
  uart_start(&uart);

  sys_set_stdout(&uart.chout);

  pin_init(&led, &pin_led_dev, PIN_OUTPUT);

  thrd_sleep_ms(1000);
  std_printf(FSTR("booting z80 cpu\n"));

  z80_t* z80 = new_z80();

  clock_start(z80);

  boot(z80);
  write_data(z80, Z80_NOP);

  //uint16_t addrq = 0;
  //addrq = read_address(z80);

  //std_printf(FSTR("address=0x%08x\n"), addrq);

  int z = 0;
  while (1) {
    thrd_sleep_ms(1000);

    if (z == 30) {
      boot(z80);
      write_data(z80, Z80_NOP);
      z = 0;
    }
    z++;
  }

  return 0;
}
