#include "Soundboard.hpp"

bool kkt = false;

SoftwareTimer* Soundboard::pwmTimer = nullptr;
SoftwareTimer* Soundboard::toneTimer = nullptr;
bool Soundboard::isHigh = false;
bool Soundboard::isPlaying = false;

Sound4* Soundboard::currentSound = nullptr;
Melody* Soundboard::currentMelody = nullptr;
toneRecord* Soundboard::currentRecord = nullptr;

void Soundboard::initSoundboard(){
    SOUNDBOARD_OUTPUT_PIN_DDR |= (1 << SOUNDBOARD_OUTPUT_PIN_NUM);
    pwmTimer = SoftwareTimerPool::acquireTimer();
    toneTimer = SoftwareTimerPool::acquireTimer();
    initPlaylist();
}

/**
 * @brief Plays single tone (emulates pwm)
 */
void Soundboard::playTone(){
    if (pwmTimer->isDone() == true && currentRecord != nullptr){

        if (isHigh == false){
            isHigh = true;
            SOUNDBOARD_OUTPUT_PIN_PORT |= (1 << SOUNDBOARD_OUTPUT_PIN_NUM);
        }
        else {
            isHigh = false;
            SOUNDBOARD_OUTPUT_PIN_PORT &= ~(1 << SOUNDBOARD_OUTPUT_PIN_NUM);
        }

        pwmTimer->startTimerUs((1000000 / currentRecord->singleTone) / 2);
    }
}

void Soundboard::play(){
    //Stop playing
    if (isPlaying == false){
        currentRecord = nullptr;
    }

    //Playing sound
    else if (currentSound != nullptr){

        //No tone is playing - starting new one
        if (currentRecord == nullptr){
            kkt = !kkt;
            if (kkt == true) PORTD |= (1 << 6);
            else PORTD &= ~(1 << 6);

            pwmTimer->stop();
            toneTimer->stop();
            currentRecord = currentSound->getCurrentToneRecord();

            //Sound finished
            if (currentRecord == nullptr){
                currentSound = nullptr;
            }
            else {
                toneTimer->startTimerUs((uint32_t)currentRecord->toneDuration * 1000);
            }
        }
    }

    //Playing melody
    else if (currentMelody != nullptr){

        //No tone is playing - starting new one
        if (currentRecord == nullptr){
            pwmTimer->stop();
            toneTimer->stop();
            currentRecord = currentMelody->getCurrentToneRecord();

            //Melody finished
            if (currentRecord == nullptr){
                currentMelody = nullptr;
            }
            else {
                toneTimer->startTimerUs((uint32_t)currentRecord->toneDuration * 1000);
            }
        }
    }

    //Tone finished
    if (toneTimer->isDone() == true){
        currentRecord = nullptr;
        isHigh = false;
        SOUNDBOARD_OUTPUT_PIN_PORT &= ~(1 << SOUNDBOARD_OUTPUT_PIN_NUM);
    }
    else {
        playTone();
    }
}