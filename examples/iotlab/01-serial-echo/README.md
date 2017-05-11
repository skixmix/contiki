IoT-LAB Serial Echo
===================

Prints "Hello World !" and echoes whatever arrives on the serial link.

This example shows how to use the serial link input API:
* line-level event: ``serial_line_event_message``

Notes:
* printf does not currently support the %l modifier (e.g. %lu)
* line buffered input is limited to 80 characters (Contiki's default)
