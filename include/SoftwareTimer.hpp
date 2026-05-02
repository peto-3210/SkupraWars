#include "hal/hardware_timer.h"

/*
This library serves as a timer interface. It uses single hardware timer with 
specific period. All timers used in current project are software-based.

Library can hold up to 16 sofware timers, each determined by its 
reference number. Timers have initial value which will decrease by
every tick of hardware clock.

Library requires MCU running at 16MHz.
*/

#ifndef SOFTWARE_TIMER
#define SOFTWARE_TIMER

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
    inline void startTimerUs(uint32_t value){
        this->value = value;
    }

    /**
     * @brief Retrieves current state of timer.
     * 
     * @return True if timer finished, false otherwise.
     */
    inline bool isDone(){
        return value == 0;
    }

    /**
     * @brief Immediately stops timer.
     */
    inline void stop(){
        value = 0;
    }

    /**
     * @brief Retrieves current timestamp in microseconds.
     * 
     * @return Current timestamp.
     */
    inline static uint32_t getTimestampUs(){
        return micros();
    }
};

class SoftwareTimerPool{

    public:
    static const uint8_t MAX_TIMER_NUM = 16;

    private:
    static SoftwareTimer timerList[MAX_TIMER_NUM];
    static uint32_t lastTimestamp;
    static uint8_t timerNum;
    static bool initialized;

    // No Constructor
    SoftwareTimerPool() = delete;

    // delete copy/move
    SoftwareTimerPool(const SoftwareTimer&) = delete;
    SoftwareTimerPool& operator=(const SoftwareTimer&) = delete;

    public:
    /**
     * @brief Initializes software timer
     * 
     * @param periodUS Period of hardware timer in microseconds. This value determines the resolution of software timers.
     */
    static void initTimerPool(uint8_t periodUS);

    /**
     * @brief Obtains timer. Timer is automatically added to timer pool,
     * with value 0 (as if timer is finished).
     * 
     * @return Pointer to timer object, or nullptr
     * if no timer is available.
     */
    static SoftwareTimer* acquireTimer();

    /**
     * @brief Main timing function, must be called periodically in a loop.
     */
    static void tick();

    /**
     * @brief Waits for specified time period inside a loop, without exiting the blocking function.
     * The timers are still ticking even during the busy wait.
     */
    static void busyWaitUs(uint32_t periodUs);
};

#endif

