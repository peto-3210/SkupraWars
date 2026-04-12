#include "Soundboard.hpp"

SoftwareTimer* Soundboard::toneTimer = nullptr;
bool Soundboard::isPlaying = false;

Sound4* Soundboard::currentSound = nullptr;
Melody* Soundboard::currentMelody = nullptr;
toneRecord* Soundboard::currentRecord = nullptr;

void Soundboard::initSoundboard(){
    hardware_pwm_init();
    toneTimer = SoftwareTimerPool::acquireTimer();
    initPlaylist();
}

void Soundboard::play(){
    //Stop playing
    if (isPlaying == false){
        toneTimer->stop();
        currentRecord = nullptr;
    }

    //Playing sound
    else if (currentSound != nullptr){

        //No tone is playing
        if (currentRecord == nullptr){
            currentRecord = currentSound->getCurrentToneRecord();

            //Sound finished
            if (currentRecord == nullptr){
                currentSound = nullptr;
                isPlaying = false;
            }

            //Start next tone
            else {
                toneTimer->startTimerUs((uint32_t)currentRecord->toneDuration * 1000);
                hardware_pwm_set(currentRecord->singleTone);
            }
        }
    }

    //Playing melody
    else if (currentMelody != nullptr){

        //No tone is playing
        if (currentRecord == nullptr){
            currentRecord = currentMelody->getCurrentToneRecord();

            //Melody finished
            if (currentRecord == nullptr){
                currentMelody = nullptr;
                isPlaying = false;
            }

            //Start next tone
            else {
                toneTimer->startTimerUs((uint32_t)currentRecord->toneDuration * 1000);
                hardware_pwm_set(currentRecord->singleTone);
            }
        }
    }

    //Tone finished
    if (toneTimer->isDone() == true){
        currentRecord = nullptr;
        hardware_pwm_reset();
    }
}