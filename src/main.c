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
} z80_t;

z80_t* new_z80() {
  z80_t* z80 = malloc(sizeof(z80_t));

  uint8_t i = 0;
  // addressbus
  for (uint8_t z = 0; z < 16; z++, i++)
    pin_init(&z80->address_bus[z], &pin_device[i], PIN_INPUT);

  // databus
  for (uint8_t z = 0; z < 8; z++, i++)
    pin_init(&z80->data_bus[z], &pin_device[i], PIN_OUTPUT);

  // clock
  pin_init(&z80->clk, &pin_device[i++], PIN_OUTPUT);

  // system control
  pin_init(&z80->system_control.m1, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->system_control.mreq, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->system_control.iorq, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->system_control.rd, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->system_control.wr, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->system_control.rfsh, &pin_device[i++], PIN_OUTPUT);

  // cpu control
  pin_init(&z80->cpu_control.phalt, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->cpu_control.pwait, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->cpu_control.pint, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->cpu_control.pnmi, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->cpu_control.preset, &pin_device[i++], PIN_OUTPUT);

  // cpu bus control
  pin_init(&z80->cpu_bus_control.busack, &pin_device[i++], PIN_OUTPUT);
  pin_init(&z80->cpu_bus_control.busrq, &pin_device[i++], PIN_OUTPUT);

  return z80;
}

//static struct event_t clock_event;
static struct timer_t clock_timer;

static void clock_cb(void *arg_p) {
  pin_toggle(&((z80_t*)arg_p)->clk);

  //uint32_t mask = 0x01;
  //event_write_isr(&clock_event, &mask, sizeof(mask));
}

void write_data(z80_t* z80, uint8_t byte) {
  for (uint8_t i = 0; i < 8; i++)
    pin_write(&z80->data_bus[i], (byte >> i) & 1);
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
  pin_write(&z80->cpu_control.pint, 1);
  pin_write(&z80->cpu_control.pnmi, 1);
  pin_write(&z80->cpu_control.pwait, 1);
  pin_write(&z80->cpu_bus_control.busrq, 1);

  pin_write(&z80->cpu_control.preset, 0);
  thrd_sleep_ms(1000);
  pin_write(&z80->cpu_control.preset, 1);

  clock_start(z80);
}

#define Z80_NOP = (0b00000000)

int main() {
  sys_start();
  z80_t* z80 = new_z80();

  boot(z80);

  uint16_t addrq = 0;
  write_data(z80, Z80_NOP)
  addrq = read_address(z80);

  return 0;
}
