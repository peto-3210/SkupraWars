#include "hal/hardware_timer.h"

/*
This library serves as a timer interface. It uses single hardware timer with 
specific period. All timers used in current project are software-based.

Library can hold up to 16 sofware timers, each determined by its 
reference number. Timers have initial value which will decrease by
every tick of hardware clock.

Library requires MCU running at 16MHz.
*/

class SoftwareTimer{

    friend class SoftwareTimerPool;

    private:
    // delete copy/move
    SoftwareTimer(const SoftwareTimer&) = delete;
    SoftwareTimer& operator=(const SoftwareTimer&) = delete;
    SoftwareTimer() = default;

    uint32_t value = 0;

    public:
    /**
     * @brief Sets initial value of specified timer (in microseconds).
     * The timer will start counting down immediately after
     * the function finishes.
     * 
     * @param value Initial value of countdown.
     */
    void startTimerUs(uint32_t value);


    /**
     * @brief Retrieves current state of timer.
     * 
     * @return True if timer finished, false otherwise.
     */
    bool isDone();
};

class SoftwareTimerPool{

    public:
    static const uint8_t MAX_TIMER_NUM = 16;

    private:
    static SoftwareTimer timerList[MAX_TIMER_NUM];
    uint32_t lastTimestamp = 0;
    uint8_t timerNum = 0;
    static bool initialized;

    // Private Constructor
    SoftwareTimerPool() = default;

    // delete copy/move
    SoftwareTimerPool(const SoftwareTimer&) = delete;
    SoftwareTimerPool& operator=(const SoftwareTimer&) = delete;

    public:
    /**
     * @brief Initializes software timer
     * 
     * @param periodUs Determines period of timer. A tick of hardware clock lasts 2 microseconds.
     * 
     * @return SoftwareTimer reference
     */
    static SoftwareTimerPool& initTimerPool(uint8_t periodUs);

    /**
     * @brief Obtains timer. Timer is automatically added to timer pool,
     * with value 0 (as if timer is finished).
     * 
     * @return Reference to timer object, or the last available timer 
     * if user exceed max amount of timers.
     */
    SoftwareTimer& acquireTimer();


    /**
     * @brief Main timing function, must be called periodically in a loop.
     */
    void tick();
    
};




