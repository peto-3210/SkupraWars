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
	
	
	//snakejazz
	melodyList[snakeJazz].addTone((tone)800, 80);
	melodyList[snakeJazz].addTone((tone)1200, 60);
	melodyList[snakeJazz].addTone((tone)600, 70);
	melodyList[snakeJazz].addTone((tone)1400, 50);

	melodyList[snakeJazz].addTone((tone)1000, 80);
	melodyList[snakeJazz].addTone((tone)0, 30);

	melodyList[snakeJazz].addTone((tone)1500, 40);
	melodyList[snakeJazz].addTone((tone)900, 60);
	melodyList[snakeJazz].addTone((tone)1300, 50);
	melodyList[snakeJazz].addTone((tone)700, 80);

	melodyList[snakeJazz].addTone((tone)1600, 40);
	melodyList[snakeJazz].addTone((tone)500, 90);

	// repeat-ish vibe
	melodyList[snakeJazz].addTone((tone)800, 60);
	melodyList[snakeJazz].addTone((tone)1100, 60);
	melodyList[snakeJazz].addTone((tone)900, 60);
	melodyList[snakeJazz].addTone((tone)1400, 60);
	
	
	// Alors on danse (with groove + pauses)
	// Phrase 1
	melodyList[alorsOnDanse].addTone(gS, 180);
	melodyList[alorsOnDanse].addTone((tone)0, 40);

	melodyList[alorsOnDanse].addTone(gS, 180);
	melodyList[alorsOnDanse].addTone((tone)0, 40);

	melodyList[alorsOnDanse].addTone(e, 220);
	melodyList[alorsOnDanse].addTone((tone)0, 30);

	melodyList[alorsOnDanse].addTone(dS, 200);
	melodyList[alorsOnDanse].addTone((tone)0, 60);

	// Phrase 2 (repeat with slight variation feel)
	melodyList[alorsOnDanse].addTone(gS, 180);
	melodyList[alorsOnDanse].addTone((tone)0, 40);

	melodyList[alorsOnDanse].addTone(gS, 180);
	melodyList[alorsOnDanse].addTone((tone)0, 40);

	melodyList[alorsOnDanse].addTone(e, 220);
	melodyList[alorsOnDanse].addTone((tone)0, 30);

	melodyList[alorsOnDanse].addTone(dS, 200);
	melodyList[alorsOnDanse].addTone((tone)0, 80);

	// Phrase 3 (lower part)
	melodyList[alorsOnDanse].addTone(cS, 260);
	melodyList[alorsOnDanse].addTone((tone)0, 40);

	melodyList[alorsOnDanse].addTone(cS, 260);
	melodyList[alorsOnDanse].addTone((tone)0, 40);

	melodyList[alorsOnDanse].addTone(a, 260);
	melodyList[alorsOnDanse].addTone((tone)0, 40);

	melodyList[alorsOnDanse].addTone(e, 300);
	melodyList[alorsOnDanse].addTone((tone)0, 120);
    
}