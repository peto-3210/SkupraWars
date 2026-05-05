#include "Soundboard.hpp"

#define AoD_BPM    160
#define AoD_Q      (60000 / AoD_BPM)
#define AoD_E      (AoD_Q / 2)

// Definice pol� pro zvuky a melodie
Sound4 Soundboard::soundList[soundNum] = {};
Melody Soundboard::melodyList[melodyNum] = {};

// Railgun: Rychl� vysok� p�pnut� (default)
const toneRecord sfx_railgunToneRecord[] PROGMEM = {
	{aH, 30},
	{fH, 40},
	{c, 20}
};

// Burst: T�i kr�tk� p�pnut� (staccato)
const toneRecord sfx_burstToneRecord[] PROGMEM = {
	{cH, 20},
	{none, 20},
	{cH, 20}
};

// Raketomet: Hlubok�, del�� zvuk (nab�haj�c�)
const toneRecord sfx_rocketToneRecord[] PROGMEM = {
	{gH, 15}, // Z�blesk v�buchu
	{c, 80},
	{dS, 80},
	{f, 150}
};

// Laser: Pulsace
const toneRecord sfx_laserToneRecord[] PROGMEM = {
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

// Prázdný zásobník
const toneRecord sfx_noAmmoToneRecord[] PROGMEM = {
	{cH, 15},   // Velmi krátký vysoký "ťuk"
	{g, 15},    // Rychlý dozvuk mechaniky
	{none, 10}  // Ukončení
};

const toneRecord sfx_powerup_useToneRecord[] PROGMEM = {
	{c, 80},    // Start
	{e, 80},    // Střed
	{g, 80},    // Vyšší tón
	{cH, 150}   // Finální vysoké C (pocit "jsem silnější")
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

// Zakázané uvolnění 
const toneRecord zakazaneUvolneniToneRecord[] PROGMEM = {
	// První motiv
	{d, 200},   // Pam
	{none, 50}, // (krátká pauza pro údernost)
	{d, 100},   // ta
	{g, 150},   // da
	{none, 50},
	{g, 200},   // dam
	
	{none, 200}, // pauza mezi frázemi

	// Druhý motiv (posun na A)
	{d, 200},   // Pam
	{none, 50},
	{d, 100},   // ta
	{a, 150},   // da
	{none, 50},
	{a, 200},   // dam
	
	{none, 200},

	// Třetí motiv (variace)
	{d, 200},
	{none, 50},
	{d, 100},
	{g, 150},
	{fHS, 150}, // Sestup přes F#
	{e, 300}    // do ztracena
};

void Soundboard::initPlaylist(){
	soundList[sfx_railgun].setToneBuffer(sfx_railgunToneRecord, sizeof(sfx_railgunToneRecord) / sizeof(toneRecord));
	soundList[sfx_burst].setToneBuffer(sfx_burstToneRecord, sizeof(sfx_burstToneRecord) / sizeof(toneRecord));
	soundList[sfx_rocket].setToneBuffer(sfx_rocketToneRecord, sizeof(sfx_rocketToneRecord) / sizeof(toneRecord));
	soundList[sfx_laser].setToneBuffer(sfx_laserToneRecord, sizeof(sfx_laserToneRecord) / sizeof(toneRecord));
	soundList[sfx_noAmmo].setToneBuffer(sfx_noAmmoToneRecord, sizeof(sfx_noAmmoToneRecord) / sizeof(toneRecord));
	soundList[sfx_powerup_use].setToneBuffer(sfx_powerup_useToneRecord, sizeof(sfx_powerup_useToneRecord) / sizeof(toneRecord));
	melodyList[imperialMarch].setToneBuffer(iMarchToneRecord, sizeof(iMarchToneRecord) / sizeof(toneRecord));
	melodyList[zakazaneUvolneni].setToneBuffer(zakazaneUvolneniToneRecord, sizeof(zakazaneUvolneniToneRecord) / sizeof(toneRecord));
}