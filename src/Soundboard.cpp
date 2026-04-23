#include "Soundboard.hpp"

SoftwareTimer* Soundboard::toneTimer = nullptr;
bool Soundboard::isPlaying = false;

Sound4* Soundboard::currentSound = nullptr;
Melody* Soundboard::currentMelody = nullptr;
toneRecord Soundboard::currentRecord = {};

void Soundboard::initSoundboard(){
    hardware_pwm_init();
    toneTimer = SoftwareTimerPool::acquireTimer();
    initPlaylist();
}

void Soundboard::play(){
    //Stop playing
    if (isPlaying == false){
        toneTimer->stop();
        currentRecord = {};
    }

    //Playing sound
    else if (currentSound != nullptr){

        //No tone is playing
        if (currentRecord.isNull()){
            currentRecord = currentSound->getCurrentToneRecord();

            //Sound finished
            if (currentRecord.isNull()){
                currentSound = nullptr;
                isPlaying = false;
            }

            //Start next tone
            else {
                toneTimer->startTimerUs((uint32_t)currentRecord.toneDuration * 1000);
                hardware_pwm_set(currentRecord.singleTone);
            }
        }
    }

    //Playing melody
    else if (currentMelody != nullptr){

        //No tone is playing
        if (currentRecord.isNull()){
            currentRecord = currentMelody->getCurrentToneRecord();

            //Melody finished
            if (currentRecord.isNull()){
                currentMelody = nullptr;
                isPlaying = false;
            }

            //Start next tone
            else {
                toneTimer->startTimerUs((uint32_t)currentRecord.toneDuration * 1000);
                hardware_pwm_set(currentRecord.singleTone);
            }
        }
    }

    //Tone finished
    if (toneTimer->isDone() == true){
        currentRecord = {};
        hardware_pwm_reset();
    }
}