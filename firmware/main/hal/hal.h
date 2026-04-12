#include <stdint.h>
#include <stdbool.h>   // for bool, if not already pulled in elsewhere

typedef uint8_t AudioClip;   // placeholder until audio module is defined

typedef struct { 
  bool (*rfid_present)(void); 
  bool (*weight_present)(void); 
  void (*lock_release)(void); 
  void (*lock_arm)(void); 
  void (*led_set)(uint8_t idx, uint8_t r, uint8_t g, uint8_t b); 
  uint32_t (*time_now_unix)(void); 
  void (*play_audio)(AudioClip clip); 
  void (*ir_send_tv_off)(void); 
} HAL;