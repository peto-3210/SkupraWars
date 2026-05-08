#include "Utilities/Soundboard.hpp"

#define AoD_BPM    160
#define AoD_Q      (60000 / AoD_BPM)
#define AoD_E      (AoD_Q / 2)

// Define arrays for sounds and melodies
Sound4 Soundboard::soundList[soundNum] = {};
Melody Soundboard::melodyList[melodyNum] = {};

// Railgun
const toneRecord sfx_railgunToneRecord[] PROGMEM = {
	{aH, 30},
	{fH, 40},
	{c, 20}
};

// Burst
const toneRecord sfx_burstToneRecord[] PROGMEM = {
	{cH, 20},
	{none, 20},
	{cH, 20}
};

// Rocket
const toneRecord sfx_rocketToneRecord[] PROGMEM = {
	{gH, 15}, 
	{c, 80},
	{dS, 80},
	{f, 150}
};

// Laser
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

// No ammo
const toneRecord sfx_noAmmoToneRecord[] PROGMEM = {
	{cH, 15},   
	{g, 15},    
	{none, 10}  
};

// Power-up use
const toneRecord sfx_powerup_useToneRecord[] PROGMEM = {
	{c, 80},    
	{e, 80},    
	{g, 80},    
	{cH, 150}   
};

// Hit enemy
const toneRecord sfx_hit_enemyToneRecord[] PROGMEM = {
    {gH, 30}, 
    {none, 10}  
};

// Fatal error
const toneRecord sfx_fatal_errorToneRecord[] PROGMEM = {
    {fHS, 500}, 
    {c, 500},   
    {fHS, 500}, 
    {c, 500}    
};

// System crash
const toneRecord sfx_system_crashToneRecord[] PROGMEM = {
    {fHS, 100}, 
    {cS, 100}, 
    {fHS, 100}, 
    {c, 600}   
};

// Game over
const toneRecord gameOverToneRecord[] PROGMEM = {
    {c, 200},
    {none, 50},
    {c, 200},
    {f, 200},
    {e, 200},
    {dS, 400}, 
    {none, 50},
    {d, 600}   
};

// Grand Victory
const toneRecord grandVictoryToneRecord[] PROGMEM = {
    {c, 150}, 
    {g, 150}, 
    {c, 150}, 
    {e, 150},
    
    {g, 100}, 
    {none, 20},
    {g, 100},
    {a, 150},
    {b, 150},

    {cH, 600}, 
    {none, 50},
    {g, 150},  
    {cH, 800}   
};

// Imperial March
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
    {aH, 20}, {none, 50}, {aH, 20}, {none, 50}, 
    {aH, 20}, {aH, 20}, {none, 100},           

    {aS, 120},  // Da
    {cH, 120},  // da
    {f, 120},   // da
    {cH, 120},  // da
    
    {dH, 250},  // Daaaa
    {fH, 120},  // da
    {dH, 120},  // da
    {cH, 350},  // Daaaa
    
    {none, 50},
    
    {aS, 120},  // Da
    {cH, 120},  // da
    {f, 120},   // da
    {cH, 120},  // da
    
    {cH, 250},  // DAAAA
    {aS, 150},  // da
    {a, 150},   // da
    {g, 350}    // Duuuu
};

// Zakazane uvolneni
const toneRecord zakazaneUvolneniToneRecord[] PROGMEM = {
    {d, 150}, 
    {none, 30},
    {d, 80},               
    {g, 150},              
    {none, 20},
    {g, 150},            
    {none, 100},           

    {d, 150}, 
    {none, 30}, 
    {d, 80},             
    {a, 150},              
    {none, 20},
    {a, 150},           
    {none, 100},

    {d, 150}, 
    {none, 30},
    {d, 80}, 
    {g, 120}, 
    {fHS, 120},         
    {e, 120},
    {d, 250},             

    {none, 50},
    {a, 100}, 
    {g, 100}, 
    {fHS, 100}, 
    {e, 200}             
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
    melodyList[gameOver].setToneBuffer(gameOverToneRecord, sizeof(gameOverToneRecord) / sizeof(toneRecord));
}