#include "SoftwareTimer.hpp"
#include <util/delay.h>

SoftwareTimer SoftwareTimerPool::timerList[MAX_TIMER_NUM] = {};
bool SoftwareTimerPool::initialized = false;
uint32_t SoftwareTimerPool::lastTimestamp = 0;
uint8_t SoftwareTimerPool::timerNum = 0;

void SoftwareTimerPool::initTimerPool(uint8_t periodUs){
    if (initialized == false){
        hardware_timer_init(periodUs);
        initialized = true;
    }
}

SoftwareTimer* SoftwareTimerPool::acquireTimer(){
    if (timerNum < MAX_TIMER_NUM){
        return &(timerList[timerNum++]);
    }
    return nullptr;
}


void SoftwareTimerPool::tick(){
    uint32_t currentTimestamp = micros();
    uint32_t diff = currentTimestamp - lastTimestamp;
    
    for (uint8_t i = 0; i < timerNum; ++i){
        if (timerList[i].value <= diff){
            timerList[i].value = 0;
        }
        else {
            timerList[i].value -= diff;
        }
    }
    lastTimestamp = currentTimestamp;
}

void SoftwareTimer::startTimerUs(uint32_t value){
    this->value = value;
}

bool SoftwareTimer::isDone(){
    return value == 0;
}
void SoftwareTimer::stop(){
    value = 0;
}
