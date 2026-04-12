#include "Soundboard.hpp"

bool kkt = false;

SoftwareTimer* Soundboard::pwmTimer = nullptr;
SoftwareTimer* Soundboard::toneTimer = nullptr;
bool Soundboard::pwmIsHigh = false;
bool Soundboard::isPlaying = false;
bool Soundboard::useHardware = false;

Sound4* Soundboard::currentSound = nullptr;
Melody* Soundboard::currentMelody = nullptr;
toneRecord* Soundboard::currentRecord = nullptr;

void Soundboard::initSoundboard(bool hardwarePWM){
    if (hardwarePWM == true){
        useHardware = hardwarePWM;
        hardware_pwm_init();
    }
    else {
        SOUNDBOARD_OUTPUT_PIN_DDR |= (1 << SOUNDBOARD_OUTPUT_PIN_NUM);
        pwmTimer = SoftwareTimerPool::acquireTimer();
    }
    toneTimer = SoftwareTimerPool::acquireTimer();
    initPlaylist();
}

/**
 * @brief Plays single tone (emulates pwm)
 */
void Soundboard::playTone(){
    if (pwmTimer->isDone() == true && currentRecord != nullptr){

        if (pwmIsHigh == false){
            pwmIsHigh = true;
            SOUNDBOARD_OUTPUT_PIN_PORT |= (1 << SOUNDBOARD_OUTPUT_PIN_NUM);
        }
        else {
            pwmIsHigh = false;
            SOUNDBOARD_OUTPUT_PIN_PORT &= ~(1 << SOUNDBOARD_OUTPUT_PIN_NUM);
        }

        pwmTimer->startTimerUs((1000000 / currentRecord->singleTone) / 2);
    }
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
            kkt = !kkt;
            if (kkt == true) PORTD |= (1 << 6);
            else PORTD &= ~(1 << 6);
            currentRecord = currentSound->getCurrentToneRecord();

            //Sound finished
            if (currentRecord == nullptr){
                currentSound = nullptr;
                isPlaying = false;
            }

            //Start next tone
            else {
                toneTimer->startTimerUs((uint32_t)currentRecord->toneDuration * 1000);
                if (useHardware == true) hardware_pwm_set(currentRecord->singleTone);
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
                if (useHardware == true) hardware_pwm_set(currentRecord->singleTone);
            }
        }
    }

    //Tone finished
    if (toneTimer->isDone() == true){
        currentRecord = nullptr;
        if (useHardware == true){
            hardware_pwm_reset();
        }
        else {
            pwmTimer->stop();
            pwmIsHigh = false;
            SOUNDBOARD_OUTPUT_PIN_PORT &= ~(1 << SOUNDBOARD_OUTPUT_PIN_NUM);
        }
    }

    //Tone playing
    else if (useHardware == false) {
        playTone();
    }
}