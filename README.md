Main Page
================

Introduction
----------------

This library is for connecting a SAME51 to a CAN bus using Arduino. It is made to be compatible with MCP_CAN_lib 
(https://github.com/coryjfowler/MCP_CAN_lib) so that I can create a single library
that uses either depending on what processor is being used on the project.

Requirements
----------------

SAME51 support is still not in the mainline samd (Arduino or Adafruit). SAME51 support
is available for Arduino here:

https://github.com/deezums/ArduinoCore-same

CMSIS-Atmel support is also needed for SAME51, available here:

https://github.com/deezums/ArduinoModule-CMSIS-Atmel

Bossa does not support SAME51, variant available here

https://github.com/deezums/BOSSA



License
-----------------
LGPL V3

Acknowledgements
-----------------
The API was copied from https://github.com/coryjfowler/MCP_CAN_lib, as well as the
constant files mcp_can_dfs.h.  Other things might be from there, also.

This fork was largely copied from https://github.com/hugllc/samc21_can. Some unnecessary bits remain, might clean up later...
