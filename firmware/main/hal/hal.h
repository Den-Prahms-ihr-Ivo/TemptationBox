#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h> 
#include <stddef.h>  

typedef uint8_t AudioClip; // TODO: placeholder until audio module is defined

typedef struct { 
  bool (*rfid_present)(void); 
  bool (*weight_present)(void); 
  bool (*ipad_slot_present)(void);
  void (*lock_release)(void); 
  void (*lock_arm)(void); 
  void (*led_set)(uint8_t idx, uint8_t r, uint8_t g, uint8_t b); 
  uint32_t (*time_now_unix)(void); 
  void (*play_audio)(AudioClip clip); 
  void (*ir_send_tv_off)(void); 
  void (*nvs_write) (const uint8_t *data, size_t len);
  bool (*nvs_read) (uint8_t *out, size_t len);
} HAL;

#endif // HAL_H