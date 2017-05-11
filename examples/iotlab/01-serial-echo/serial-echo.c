#include "contiki.h"
#include "dev/serial-line.h"
#include <stdio.h>

/*
 * Prints "Hello World !", and echoes whatever arrives on the serial link
 */

PROCESS(serial_echo, "Serial Echo");
AUTOSTART_PROCESSES(&serial_echo);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_echo, ev, data)
{
  PROCESS_BEGIN();

  printf("Hello World !\n");
  while(1) {
    printf("> ");
    PROCESS_YIELD();
    if (ev == serial_line_event_message) {
      printf("Echo cmd: '%s'\n", (char*)data);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
