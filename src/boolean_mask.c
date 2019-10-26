#if INTERFACE

#include <stdlib.h>
#include <stdbool.h>

typedef struct BooleanMask {
unsigned char *chars;
unsigned int entry_count;
unsigned int char_count;
} BooleanMask;

#endif

#include "boolean_mask.h"

BooleanMask *boolean_mask_create () {
  BooleanMask *result = malloc(sizeof(BooleanMask));
  result->chars = NULL;
  result->entry_count;
  return result;
}

void boolean_mask_reallocate(BooleanMask *mask, unsigned int char_index) {
  if (char_index + 1 > mask->char_count) {
    unsigned char *new_chars = calloc(char_index + 1, sizeof(char));
    for (unsigned int i = 0; i < mask->char_count; i++) {
      new_chars[i] = mask->chars[i];
    }
    mask->char_count = char_index + 1;
    free(mask->chars);
    mask->chars = new_chars;
  }
}

void boolean_mask_set(BooleanMask *mask, unsigned int index, bool value) {
  unsigned int char_index = index / 8;
  boolean_mask_reallocate(mask, char_index);
  unsigned int inner_index = index - (char_index * 8);
  unsigned char desired = mask->chars[char_index];
  desired ^= (-value ^ desired) & (1UL << inner_index);
  mask->chars[char_index] = desired;
}

bool boolean_mask_get(BooleanMask *mask, unsigned int index, bool value) {
  unsigned int char_index = index / 8;
  boolean_mask_reallocate(mask, char_index);
  unsigned int inner_index = index - (char_index * 8);
  unsigned char desired = mask->chars[char_index];
  unsigned char result = (desired >> inner_index) & 1U;
  return result;
}
