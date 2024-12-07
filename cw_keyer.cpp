#include "SDT.h"
#include "cw_keyer.h"
// Putting static variables and #defines here instead of in a .h file to avoid polluting the global namespace.
/******************************************************
Iambic CW keyer.  Adapted from https://github.com/ve3wmb/SimpleCWKeyer/blob/main/SimpleCwKeyer.ino
2024-12-05 JLK
*******************************************************/
enum KSTYPE
  {
  IDLE,
  CHK_DIT,
  CHK_DAH,
  KEYED_PREP,
  KEYED,
  INTER_ELEMENT, 
  PRE_IDLE
  };

KSTYPE keyer_state = PRE_IDLE;

enum KEYER_CMD_STATE_TYPE
  {
  CMD_IDLE,
  CMD_ENTER,
  CMD_TUNE,
  CMD_SPEED_WAIT_D1,
  CMD_SPEED_WAIT_D2,
  };

KEYER_CMD_STATE_TYPE keyer_command_state = CMD_IDLE;

// Data type for storing Keyer settings in EEPROM
struct KeyerConfigType {
  uint32_t ms_per_dit;         // Speed
  uint8_t dit_paddle_pin;      // Dit paddle pin number
  uint8_t dah_paddle_pin;      // Dah Paddle pin number
  uint8_t iambic_keying_mode;  // 0 - Iambic-A, 1 Iambic-B
  uint8_t sidetone_is_muted;   //
  uint32_t num_writes;         // Number of writes to EEPROM, incremented on each 'W' command
  uint32_t data_checksum;      // Checksum containing a 32-bit sum of all of the other fields
};


uint8_t current_morse_character;
uint8_t keyer_control;
extern unsigned long transmitDitLength;
//uint32_t dit_time = 100;            //milliseconds - just to try
#define DIT_L 0x01     // Dit latch (Binary 0000 0001)
#define DAH_L 0x02     // Dah latch (Binary 0000 0010)
#define DIT_PROC 0x04  // Dit is being processed (Binary 0000 0100)
#define IAMBIC_B 0x10  // 0x00 for Iambic A, 0x10 for Iambic B (Binary 0001 0000)
#define IAMBIC_A 0

void process_keyer_command( char current_morse_character, int ditTime )
  {
  
  }

void update_paddle_latch()
  {
  if( digitalRead( paddleDit ) == LOW ) keyer_control |= DIT_L;
  if( digitalRead( paddleDah ) == LOW ) keyer_control |= DAH_L; 
  }

void cw_keyer()
  {
  static uint32_t keyer_timer;
  switch( keyer_state )
    {
    case IDLE:
    // Wait for direct or latched paddle press
      {
      update_paddle_latch();
      current_morse_character = B11111110;  // Sarting point for accumlating a character input via paddles
      keyer_state = CHK_DIT;
      }
    break;

    case CHK_DIT:
    // See if the dit paddle was pressed
    if( keyer_control & DIT_L )
      {
      keyer_control |= DIT_PROC;
      keyer_timer = transmitDitLength;
      current_morse_character = ( current_morse_character << 1);  // Shift a DIT (0) into the bit #0 position.
      keyer_state = KEYED_PREP;
      }
    else
      {
      keyer_state = CHK_DAH;
      }
    break;

    case CHK_DAH:
    // See if dah paddle was pressed
    if( keyer_control & DAH_L )
      {
      keyer_timer = transmitDitLength * 3;
      current_morse_character = (( current_morse_character << 1 ) | 1 );  // Shift left one position and make bit #0 a DAH (1)
      keyer_state = KEYED_PREP;
      }
    else
      {
      keyer_timer = millis() + ( transmitDitLength * 2 );  // Character space, is transmitDitLength x 2 because already have a trailing intercharacter space
      keyer_state = PRE_IDLE;
      }
    break;

    case KEYED_PREP:
    // Assert key down, start timing, state shared for dit or dah
    start_sending_cw();

    keyer_timer += millis();                  // set ktimer to interval end time
    keyer_control &= ~(DIT_L + DAH_L);  // clear both paddle latch bits
    keyer_state = KEYED;                // next state
    break;

    case KEYED:
    // Wait for timer to expire
    if( millis() > keyer_timer )
      {  // are we at end of key down ?
      stop_sending_cw();
      keyer_timer = millis() + transmitDitLength;  // inter-element time
      keyer_state = INTER_ELEMENT;   // next state
      }
    else if( keyer_control & IAMBIC_B)
      {
      update_paddle_latch();  // early paddle latch check in Iambic B mode
      }
    break;

    case INTER_ELEMENT:
    // Insert time between dits/dahs
    update_paddle_latch();                       // latch paddle state
    if( millis() > keyer_timer )
      {                    // are we at end of inter-space ?
      if( keyer_control & DIT_PROC )
        {          // was it a dit or dah ?
        keyer_control &= ~(DIT_L + DIT_PROC);  // clear two bits
        keyer_state = CHK_DAH;                 // dit done, check for dah
        }
      else
        {
        keyer_control &= ~( DAH_L );           // clear dah latch
        keyer_timer = millis() + ( transmitDitLength * 2 );  // Character space, is g_ditTime x 2 because already have a trailing intercharacter space
        keyer_state = PRE_IDLE;              // go idle
        }
      }
    break;

    case PRE_IDLE:  // Wait for an intercharacter space
    // Check for direct or latched paddle press
    if(( digitalRead( paddleDit ) == LOW) || ( digitalRead( paddleDah ) == LOW ) || ( keyer_control & 0x03 ))
      {
      update_paddle_latch();
      keyer_state = CHK_DIT;
      }
    else
      {                    // Check for intercharacter space
      if( millis() > keyer_timer )
        {  // We have a character
        // Serial.println(g_current_morse_character);
        keyer_state = IDLE;  // go idle
        if( keyer_command_state != CMD_IDLE )
          {
          process_keyer_command( current_morse_character, transmitDitLength );
          }
        }
      }
    break;
    }
  }
