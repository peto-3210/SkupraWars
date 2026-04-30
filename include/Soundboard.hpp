#include "SoftwareTimer.hpp"
#include <avr/pgmspace.h>

#ifndef SOUNDOARD
#define SOUNDBOARD

#define SOUNDBOARD_OUTPUT_PIN_PORT PORTB
#define SOUNDBOARD_OUTPUT_PIN_DDR DDRB
#define SOUNDBOARD_OUTPUT_PIN_NUM 1

enum tone: uint16_t {
    none = 0,
	dS = 250,
    c = 261,
    d = 294,
	cS = 300,
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

/*In C/C++, aggregate initialization fills members in order, 
stopping when the initializer list runs out. The rest are zero-initialized. 
For a union, only the first member is considered for initialization.*/

struct toneRecord{
    union {
        struct {
            tone singleTone;
            uint16_t toneDuration;
        };
        uint32_t rawData;
    };

    toneRecord() = default;

    bool isNull(){
        return singleTone == none && toneDuration == 0;
    }
};

template <uint8_t soundLength>
class Sound {
    friend class Soundboard;

    private:
    const toneRecord* toneList;

    uint8_t toneNum = 0;
    uint8_t iterator = 0;

    /**
     * @brief Retrieves currently played tone record,
     * then increments iterator.
     * 
     * @return Current Tone
     */
    toneRecord getCurrentToneRecord(){
        if (iterator == toneNum){
            iterator = 0;
            return (toneRecord){};
        }
        toneRecord rec;
        rec.rawData = pgm_read_dword(&toneList[iterator++]);
        return rec;
    }

    public:
    Sound() = default;  

    /**
     * @brief Sets the array of tones. Due to its size, tone array
     * is stored in flash memory.
     * 
     * Structure of tone:
     * {Type of tone, Duration of tone in milliseconds}
     * @param toneBuffer Pointer to the array of tones stored in flash memory
     * @param num Number of tones in the array
     * 
     */
    inline void setToneBuffer(const toneRecord* toneBuffer, uint8_t num){
        toneList = toneBuffer;
        toneNum = num;
    }; 
};

using Sound4 = Sound<4>;
using Melody = Sound<64>;

class Soundboard {

    private:
    // Private Constructor
    Soundboard() = default;

    // delete copy/move
    Soundboard(const Soundboard&) = delete;
    Soundboard& operator=(const Soundboard&) = delete;

    static SoftwareTimer* toneTimer;
    static bool isPlaying;

    static Sound4* currentSound;
    static Melody* currentMelody;
    static toneRecord currentRecord;

    static const uint8_t soundNum = 4;
    static Sound4 soundList[soundNum];
    static const uint8_t melodyNum = 4;
    static Melody melodyList[melodyNum];

    static void initPlaylist();

    public:

    enum sounds {
        sample,
    };

    enum melodies {
        imperialMarch,
        halucination,
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