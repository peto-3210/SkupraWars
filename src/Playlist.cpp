#include "Soundboard.hpp"
#include <avr/pgmspace.h>

#define AoD_BPM    160
#define AoD_Q      (60000 / AoD_BPM)
#define AoD_E      (AoD_Q / 2)

// Definice polí pro zvuky a melodie
Sound4 Soundboard::soundList[soundNum] = {};
Melody Soundboard::melodyList[melodyNum] = {};

// Railgun: Rychlé vysoké pípnutí (default)
const toneRecord sfx_railgun[] PROGMEM = {
	{aH, 30},
	{fH, 40},
	{c, 20}
};

// Burst: Tøi krátká pípnutí (staccato)
const toneRecord sfx_burst[] PROGMEM = {
	{cH, 20},
	{none, 20},
	{cH, 20}
};

// Raketomet: Hluboký, delší zvuk (nabíhající)
const toneRecord sfx_rocket[] PROGMEM = {
	{gH, 15}, // Záblesk výbuchu
	{c, 80},
	{dS, 80},
	{f, 150}
};

// Laser: Pulsace
const toneRecord sfx_laser[] PROGMEM = {
	{e, 60},
	{f, 60},
	{g, 60},
	{f, 60},
	{e, 60},
	{e, 60},
	{f, 60},
	{g, 60},
	{f, 60},
	{e, 60}
};

const toneRecord sampleToneRecord[] PROGMEM = {
	{a, 1000},
	{gH, 1000},
	{aH, 1000}
};

const toneRecord iMarchToneRecord[] PROGMEM = {
	{a, 500},
	{a, 500},
	{a, 500},
	{f, 350},
	{cH, 150},
	
	{a, 500},
	{f, 350},
	{cH, 150},
	{a, 1000},
	{eH, 500},
	
	{eH, 500},
	{eH, 500},
	{fH, 350},
	{cH, 150},
	{gS, 500},
	
	{f, 350},
	{cH, 150},
	{a, 1000}
};

void Soundboard::initPlaylist(){
	soundList[sfx_railgun].setToneBuffer(sfx_railgun, sizeof(sfx_railgun) / sizeof(toneRecord));
	soundList[sfx_burst].setToneBuffer(sfx_burst, sizeof(sfx_burst) / sizeof(toneRecord));
	soundList[sfx_rocket].setToneBuffer(sfx_rocket, sizeof(sfx_rocket) / sizeof(toneRecord));
	soundList[sfx_laser].setToneBuffer(sfx_laser, sizeof(sfx_laser) / sizeof(toneRecord));
	soundList[sample].setToneBuffer(sampleToneRecord, sizeof(sampleToneRecord) / sizeof(toneRecord));
	melodyList[imperialMarch].setToneBuffer(iMarchToneRecord, sizeof(iMarchToneRecord) / sizeof(toneRecord));
}