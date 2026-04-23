#include "Soundboard.hpp"

#define AoD_BPM    160
#define AoD_Q      (60000 / AoD_BPM)
#define AoD_E      (AoD_Q / 2)

Sound4 Soundboard::soundList[soundNum] = {};
Melody Soundboard::melodyList[melodyNum] = {};

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

const toneRecord halucinationToneRecord[] PROGMEM = {
    // --- Phrase 1 ---
    {cHS,  AoD_E},  {aS,   AoD_E},  {gS,   AoD_E},  {fHS,  AoD_E},
    {gS,   AoD_Q},  {none, AoD_E},
    {cHS,  AoD_E},  {aS,   AoD_E},  {gS,   AoD_E},  {fHS,  AoD_E},
    {gS,   AoD_Q}, {none, AoD_E},
    // --- Phrase 2 ---
    {cHS,  AoD_E},  {dHS,  AoD_E},  {cHS,  AoD_E},  {aS,   AoD_E},
    {gS,   AoD_Q},  {none, AoD_E},
    {fHS,  AoD_E},  {gS,   AoD_E},  {aS,   AoD_E},  {cHS,  AoD_E},
    {aS,   AoD_Q}, {none, AoD_E},
    // --- Phrase 3 ---
    {gS,   AoD_E},  {aS,   AoD_E},  {cHS,  AoD_E},  {dHS,  AoD_E},
    {eH,   AoD_Q},  {none, AoD_E},
    {dHS,  AoD_E},  {cHS,  AoD_E},  {aS,   AoD_E},  {gS,   AoD_E},
    {fHS,  AoD_Q}, {none, AoD_E},
    // --- Descending resolution ---
    {fHS,  AoD_E},  {eH,   AoD_E},  {dHS,  AoD_E},  {cHS,  AoD_E},
    {aS,   AoD_Q},  {none, AoD_E},
    {fHS,  AoD_E},  {e,    AoD_E},  {d,    AoD_E},  {cHS,  AoD_E},
    {b,    AoD_Q},  {none, AoD_E},
    // --- Final descent to low tones ---
    /*{fHS,  AoD_E},  {e,    AoD_E},  {d,    AoD_E},  {cHS,  AoD_E},
    {b,    AoD_Q},  {none, AoD_E},
    {aS,   AoD_E},  {gS,   AoD_E},  {fHS,  AoD_E},  {e,    AoD_E},
    {d,    AoD_DQ}, {none, AoD_S},
    // --- Resolve ---
    {d,    AoD_E},  {c,    AoD_E},  {d,    AoD_E},  {e,    AoD_E},
    {fHS,  AoD_Q},  {none, AoD_E},
    {e,    AoD_E},  {d,    AoD_E},  {c,    AoD_Q},
    {c,    AoD_DQ}, {none, AoD_S},*/
};

void Soundboard::initPlaylist(){
    soundList[sample].setToneBuffer(sampleToneRecord, sizeof(sampleToneRecord) / sizeof(toneRecord));
    melodyList[imperialMarch].setToneBuffer(iMarchToneRecord, sizeof(iMarchToneRecord) / sizeof(toneRecord));
    melodyList[halucination].setToneBuffer(halucinationToneRecord, sizeof(halucinationToneRecord) / sizeof(toneRecord));
}