#include "hal/uart.h"

class Datalink {

    public:
    static const uint8_t packetLength = 2;

    private:
    // Private Constructor
    Datalink() = default;

    // delete copy/move
    Datalink(const Datalink&) = delete;
    Datalink& operator=(const Datalink&) = delete;

    typedef union {
        uint8_t rawData[2];
        struct {
            uint8_t function    :2;
            uint8_t reserved    :1;
            uint8_t reply       :1;
            uint8_t isFar       :1;
            uint8_t distance    :3;

            uint8_t payload;
        };
    } packet;

    typedef enum {
        ringNotClosed,
        unknownParticipant,
        missingReply,
        crcError,
        unknownMessage,
    } commError;

    const char* errorMessages[5] = {
        "Topology ring is not closed",
        "Missing initialization message from participant with distance:",
        "Missing reply from participant:",
        "CRC error in message from participant:",
        "Unsolicited reply from participant:",
    };

    static bool initialized;
    uint8_t datalinkPacket[packetLength + 1] = {0};

    char* errorMsg = nullptr;
    uint8_t errorData = 0;

    bool calculateCRC(uint8_t data[3], bool direction);
    bool sendData(uint8_t data[packetLength]);
    bool recvData(uint8_t data[packetLength]);

    public:
    typedef enum {
        b9600,
        b19200,
        b115200
    } baudRate;

    /**
     * @brief Initializes physical layer
     * 
     * @param rate Baud rate
     */
    static Datalink& initDatalink(baudRate rate);

    /**
     * @brief Function sends packet to tell everyone
     * who is it, then listens for messages from other 
     * participants. Also checks whether the ring
     * is closed.
     * 
     * @param 
     */

    /**
     * @brief Flushes RX buffer
     */
    void flushRX();
};
