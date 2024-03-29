#ifndef CuteBuzzerSounds_h
#define CuteBuzzerSounds_h
#ifdef __AVR__
#include <avr/power.h>
#endif
#include "Sounds.h"

// Removed invalid code

class CuteBuzzerSoundsClass {
  public:
  void init(int buzzerPin);
  void initBuzzer(int buzzerPin);

  // Sounds
  void _tone(float noteFrequency, long noteDuration, int silentDuration);
  void bendTones(float initFrequency, float finalFrequency, float prop, long noteDuration, int silentDuration);
  void play(int soundName);

  private:
  int buzzerPin;
};

extern CuteBuzzerSoundsClass cute;

#endif
