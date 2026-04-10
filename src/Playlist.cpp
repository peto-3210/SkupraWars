#include "Soundboard.hpp"

Sound4 Soundboard::soundList[soundNum] = {};
Melody Soundboard::melodyList[melodyNum] = {};

void Soundboard::initPlaylist(){

    soundList[sample].addTone(a, 1000);
    soundList[sample].addTone(gH, 1000);
    soundList[sample].addTone(aH, 1000);

    //Imperial march
    melodyList[imperialMarch].addTone(a, 500); 
    melodyList[imperialMarch].addTone(a, 500);
    melodyList[imperialMarch].addTone(a, 500);
    melodyList[imperialMarch].addTone(f, 350);
    melodyList[imperialMarch].addTone(cH, 150);
    
    melodyList[imperialMarch].addTone(a, 500);
    melodyList[imperialMarch].addTone(f, 350);
    melodyList[imperialMarch].addTone(cH, 150);
    melodyList[imperialMarch].addTone(a, 1000);
    melodyList[imperialMarch].addTone(eH, 500);
    
    melodyList[imperialMarch].addTone(eH, 500);
    melodyList[imperialMarch].addTone(eH, 500);
    melodyList[imperialMarch].addTone(fH, 350);
    melodyList[imperialMarch].addTone(cH, 150);
    melodyList[imperialMarch].addTone(gS, 500);
    
    melodyList[imperialMarch].addTone(f, 350);
    melodyList[imperialMarch].addTone(cH, 150);
    melodyList[imperialMarch].addTone(a, 1000);
    
}