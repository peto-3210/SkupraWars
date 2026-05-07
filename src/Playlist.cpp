#include "Utilities/Soundboard.hpp"

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

// Pouziti power-upu
const toneRecord sfx_powerup_useToneRecord[] PROGMEM = {
	{c, 80},    // Start
	{e, 80},    // Střed
	{g, 80},    // Vyšší tón
	{cH, 150}   // Finální vysoké C (pocit "jsem silnější")
};

// Hit enemy
const toneRecord sfx_hit_enemyToneRecord[] PROGMEM = {
    {gH, 30},   // Krátké vysoké pípnutí
    {none, 10}  // Okamžité utnutí
};

// Fatal error
const toneRecord sfx_fatal_errorToneRecord[] PROGMEM = {
    {fHS, 500}, // První tón (vysoký a ostrý)
    {c, 500},   // Druhý tón (hluboký, tvoří tritón k fHS)
    {fHS, 500}, // Opakování pro naléhavost
    {c, 500}    // Finální dlouhý hluboký tón
};

// System crash
const toneRecord sfx_system_crashToneRecord[] PROGMEM = {
    {fHS, 100}, // Ostrý, nepříjemný start
    {cS, 100},  // Okamžitý propad do disonance
    {fHS, 100}, // Rychlé "cuknutí" zpět
    {c, 600}    // Finální, hluboký, "mrtvý" tón, který zní jako bzučení chyby
};

// Grand Victory
const toneRecord grandVictoryToneRecord[] PROGMEM = {
    // První část: Rozjezd
    {c, 150}, 
    {g, 150}, 
    {c, 150}, 
    {e, 150},
    
    // Druhá část: Gradace
    {g, 100}, 
    {none, 20},
    {g, 100},
    {a, 150},
    {b, 150},

    // Finále: Triumf
    {cH, 600},  // Dlouhé vysoké C
    {none, 50},
    {g, 150},   // Krátký dovětek
    {cH, 800}   // Poslední vítězný úder
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

// NEVER GONNA GIVE YOU UP, NEVERE GONNA LET YOU DOWN
const toneRecord rickRollRiffToneRecord[] PROGMEM = {
    // --- Bicí úvod (ponechán pro kontext) ---
    {aH, 20}, {none, 50}, {aH, 20}, {none, 50}, 
    {aH, 20}, {aH, 20}, {none, 100},           

    // --- Ten hlavní riff ---
    {aS, 120},  // Da
    {cH, 120},  // da
    {f, 120},   // da
    {cH, 120},  // da
    
    {dH, 250},  // DÁÁ (vyšší tón)
    {fH, 120},  // da
    {dH, 120},  // da
    {cH, 350},  // DÁÁÁ...
    
    {none, 50}, // mikro pauza pro oddělení
    
    {aS, 120},  // Da
    {cH, 120},  // da
    {f, 120},   // da
    {cH, 120},  // da
    
    {cH, 250},  // DÁ (střední)
    {aS, 150},  // da
    {a, 150},   // da
    {g, 350}    // DŮŮŮ...
};

// Zakázané uvolnění (Turbo Arcade Edition)
const toneRecord zakazaneUvolneniToneRecord[] PROGMEM = {
    // --- 1. MOTIV (D -> G) ---
    {d, 150}, {none, 30},  // PAM (důrazný úder)
    {d, 80},               // ta
    {g, 150},              // DA
    {none, 20},
    {g, 150},              // DAM
    {none, 100},           // krátký oddech

    // --- 2. MOTIV (D -> A) ---
    {d, 150}, {none, 30},  // PAM
    {d, 80},               // ta
    {a, 150},              // DA
    {none, 20},
    {a, 150},              // DAM
    {none, 100},

    // --- 3. MOTIV (Gradace) ---
    {d, 150}, {none, 30},
    {d, 80}, 
    {g, 120}, 
    {fHS, 120},            // Rychlejší sestup
    {e, 120},
    {d, 250},              // Návrat do základní tóniny (D)

    // --- 4. ZÁVĚR (Odpich) ---
    {none, 50},
    {a, 100}, 
    {g, 100}, 
    {fHS, 100}, 
    {e, 200}               // Zakončení fráze
};

void Soundboard::initPlaylist(){
	soundList[sfx_railgun].setToneBuffer(sfx_railgunToneRecord, sizeof(sfx_railgunToneRecord) / sizeof(toneRecord));
	soundList[sfx_burst].setToneBuffer(sfx_burstToneRecord, sizeof(sfx_burstToneRecord) / sizeof(toneRecord));
	soundList[sfx_rocket].setToneBuffer(sfx_rocketToneRecord, sizeof(sfx_rocketToneRecord) / sizeof(toneRecord));
	soundList[sfx_laser].setToneBuffer(sfx_laserToneRecord, sizeof(sfx_laserToneRecord) / sizeof(toneRecord));
	soundList[sfx_noAmmo].setToneBuffer(sfx_noAmmoToneRecord, sizeof(sfx_noAmmoToneRecord) / sizeof(toneRecord));
	soundList[sfx_hit_enemy].setToneBuffer(sfx_hit_enemyToneRecord, sizeof(sfx_hit_enemyToneRecord) / sizeof(toneRecord));
	soundList[sfx_powerup_use].setToneBuffer(sfx_powerup_useToneRecord, sizeof(sfx_powerup_useToneRecord) / sizeof(toneRecord));
	soundList[sfx_fatal_error].setToneBuffer(sfx_fatal_errorToneRecord, sizeof(sfx_fatal_errorToneRecord) / sizeof(toneRecord));
	soundList[sfx_system_crash].setToneBuffer(sfx_system_crashToneRecord, sizeof(sfx_system_crashToneRecord) / sizeof(toneRecord));
	melodyList[grandVictory].setToneBuffer(grandVictoryToneRecord, sizeof(grandVictoryToneRecord) / sizeof(toneRecord));
	melodyList[imperialMarch].setToneBuffer(iMarchToneRecord, sizeof(iMarchToneRecord) / sizeof(toneRecord));
	melodyList[rickRollRiff].setToneBuffer(rickRollRiffToneRecord, sizeof(rickRollRiffToneRecord) / sizeof(toneRecord));
	melodyList[zakazaneUvolneni].setToneBuffer(zakazaneUvolneniToneRecord, sizeof(zakazaneUvolneniToneRecord) / sizeof(toneRecord));
}