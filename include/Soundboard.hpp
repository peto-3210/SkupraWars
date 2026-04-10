#include "SoftwareTimer.hpp"

#ifndef SOUNDOARD
#define SOUNDBOARD

#define SOUNDBOARD_OUTPUT_PIN_PORT PORTD
#define SOUNDBOARD_OUTPUT_PIN_DDR DDRD
#define SOUNDBOARD_OUTPUT_PIN_NUM 7

enum tone: uint16_t {
    none = 0,
    c = 261,
    d = 294,
    e = 329,
    f = 349,
    g = 391,
    gS = 415,
    a = 440,
    aS = 455,
    b = 466,
    cH = 523,
    cHS = 554,
    dH = 587,
    dHS = 622,
    eH = 659,
    fH = 698,
    fHS = 740,
    gH = 784,
    gHS = 830,
    aH = 880,
};

struct toneRecord{
    tone singleTone;
    uint16_t toneDuration;

    toneRecord() = default;

    toneRecord(tone t, uint16_t d){
        singleTone = t;
        toneDuration = d;
    }
};

template <uint8_t soundLength>
class Sound {
    friend class Soundboard;

    private:
    toneRecord toneList[soundLength];

    uint8_t toneNum = 0;
    bool finished = false;
    uint8_t iterator = 0;

    /**
     * @brief Retrieves currently played tone record,
     * then increments iterator.
     * 
     * @return Current Tone
     */
    toneRecord* getCurrentToneRecord(){
        if (iterator == toneNum){
            return nullptr;
        }
        return &(toneList[iterator++]);
    }

    public:
    Sound() = default;  

    /**
     * @brief Adds tone with duration to the sound.
     * 
     * @param Tone Type of tone
     * @param duration Duration of tone in milliseconds
     * 
     * @return False if sound is full, true otherwise.
     */
    bool addTone(tone singleTone, uint16_t toneDuration){
        if (toneNum < soundLength && finished == false){
            toneList[toneNum] = toneRecord(singleTone, toneDuration);
            toneNum++;
            return true;
        }
        return false;
    }; 
};

using Sound4 = Sound<4>;
using Melody = Sound<32>;

class Soundboard {

    private:
    // Private Constructor
    Soundboard() = default;

    // delete copy/move
    Soundboard(const Soundboard&) = delete;
    Soundboard& operator=(const Soundboard&) = delete;

    static SoftwareTimer* pwmTimer;
    static SoftwareTimer* toneTimer;
    static bool isHigh;
    static bool isPlaying;

    static Sound4* currentSound;
    static Melody* currentMelody;
    static toneRecord* currentRecord;

    static const uint8_t soundNum = 4;
    static Sound4 soundList[soundNum];
    static const uint8_t melodyNum = 4;
    static Melody melodyList[melodyNum];

    static void initPlaylist();
    static void playTone();

    public:

    enum sounds {
        sample,
    };

    enum melodies {
        imperialMarch,
    };

    /**
     * @brief Initializes soundboard.
     */
    static void initSoundboard();

    /**
     * @brief Plays a sound.
     * @param s Pointer to the sound to play.
     */
    static void playSound(sounds s){
        currentSound = &(soundList[s]);
        isPlaying = true;
    }

    /**
     * @brief Plays a melody.
     * @param m Pointer to the melody to play. If melody and sound are both set, 
     * melody will be interrupted and resumed after sound finishes.
     */
    static void playMelody(melodies m){
        currentMelody = &(melodyList[m]);
        isPlaying = true;
    }


    /**
     * @brief Pauses playback.
     */
    static void pause(){
        isPlaying = false;
    };

    /**
     * @brief Resumes playback.
     */
    static void resume(){
        isPlaying = true;
    };

    /**
     * @brief Plays the current sound or melody, should be called in a loop.
     */
    static void play();
};

#endif