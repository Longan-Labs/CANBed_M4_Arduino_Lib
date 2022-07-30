/*
  CAN fd send example for CANbed M4
  
  begin(uint8_t idmodeset, uint32_t speedset, enum mcan_can_mode canmode)
  
  @idmodeset:
  
  #define MCP_STDEXT   0        // Standard and Extended    
  #define MCP_STD      1        // Standard IDs ONLY       
  #define MCP_EXT      2        // Extended IDs ONLY        
  #define MCP_ANY      3        // Disables Masks and Filters 

  @speedset:
  
  #define CAN_250K_500K
  #define CAN_250K_1M
  #define CAN_250K_1M5
  #define CAN_250K_2M
  #define CAN_500K_1M 
  #define CAN_500K_2M
  #define CAN_500K_4M
  #define CAN_1000K_4M 
  
  Note: If you need other baudrate, please contact info@longan-labs.cc
  
  @canmode:
  
  MCAN_MODE_CAN                             ISO 11898-1 CAN 2.0 mode 

  MCAN_MODE_EXT_LEN_CONST_RATE              Long CAN FD frame.
                                            * - TX and RX payloads up to 64 bytes.
                                            * - Slow transmitter: TX frames are sent at constant bit rate.
                                            * - Fast receiver: both constant-rate (slow) and dual-rate (fast)
                                            *   RX frames are supported and received. 

  MCAN_MODE_EXT_LEN_DUAL_RATE               Long and fast CAN FD frame.
                                            * - TX and RX payloads up to 64 bytes.
                                            * - Fast transmitter: control, data and CRC fields are transmitted at a
                                            *   higher bit rate.
                                            * - Fast receiver. 
*/

#include <same51_can.h>

SAME51_CAN can;


unsigned char dta[64];

void setup()
{
    Serial.begin(115200);
    while(!Serial);
    
    uint8_t ret;
    ret = can.begin_fd(MCP_ANY, CAN_500K_4M, MCAN_MODE_EXT_LEN_CONST_RATE);
    if (ret == CAN_OK) 
    {
        Serial.println("CAN Initialized Successfully!");
    } else 
    {
        Serial.println("Error Initializing CAN...");
        while(1);
    }
    
    for(int i=0; i<64; i++)
    {
        dta[i] = i;
    }
}


void loop()
{
    unsigned char len = 64;
    unsigned int id = 0x00;
    unsigned int ext = 0;       // 0 for standard frame, 1 for extend frame
    
    can.sendFdMsgBuf(id, ext, len, dta);
    // can.sendMsgBuf(id, ext, 8, dta);         // you can send 2.0 frame under fd mode.
    delay(100);
}

// END FILE
