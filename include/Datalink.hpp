#include "hal/uart.h"
#include "SoftwareTimer.hpp"

#ifndef DATALINK
#define DATALINK

class Datalink {
    public:
    enum functionCode{
        checkTopology,
        shootProjectile,
        announceHP,
        reserved
    };

    enum recvPacketState{
        noPacket,
        packetReceived,
        replyReceived,
        packetForOtherParticipant,
        error,
    };

    private:
    // Private Constructor
    Datalink() = default;

    // delete copy/move
    Datalink(const Datalink&) = delete;
    Datalink& operator=(const Datalink&) = delete;

    union Packet {
        uint8_t rawData[2];
        struct {
            uint8_t function    :2;
            uint8_t direction   :1; // 0 for packet sent in direction of participant with distance 0, 1 for opposite direction
            uint8_t reply       :1;
            uint8_t reserved    :1;
            uint8_t distance    :3;

            uint8_t payload;
        };
        Packet() = default;
        Packet(uint8_t distance, functionCode func, uint8_t payload, bool direction, bool reply = false){
            this->function = func;
            this->reserved = 0;
            this->reply = reply;
            this->direction = direction;
            this->distance = distance;
            this->payload = payload;
        }
    };

    enum commError{
        sendBufferFull,
        notEnoughBytes,
        crcError,
        ringNotClosed,
        unknownParticipant,
        missingReply,
        unknownMessage,
    };

    static constexpr const char* errorMessages[7] = {
        "Send buffer full.",
        "Not enough bytes received.",
        "CRC error in message.",
        "Topology ring is not closed.",
        "Missing initialization message from participant with distance:",
        "Missing reply from participant:",
        "Unsolicited reply.",
    };

    static bool initialized;
    static bool replying;
    static uint8_t nodeNum;

    static const uint8_t PACKET_LENGTH = 2;
    static const uint8_t DATALINK_PACKET_LENGTH = 3;
    static uint8_t datalinkPacket[DATALINK_PACKET_LENGTH];

    static uint32_t packetReceiveTimeUS;
    static SoftwareTimer* packetReceiveTimer;
    static const char* errorMsg;
    static uint8_t errorData;

    static bool calculateCRC(uint8_t data[DATALINK_PACKET_LENGTH], bool direction);
    static bool sendPacket(Packet& packet);
    static recvPacketState recvPacket(Packet& packet); 
    static void flushRX();

    public:
    enum baudRate{
        b9600 = 9600,
        b19200 = 19200,
        b115200 = 115200
    };

    /**
     * @brief Initializes physical layer
     * 
     * @param rate Baud rate
     */
    static void initDatalink(baudRate rate);

    /**
     * @brief Function sends packet to tell everyone
     * who is it, then listens for messages from other 
     * participants. Also checks whether the ring
     * is closed.
     * 
     * @param participants Total number of participants in topology, including self.
     * @param myPayload Self-identification payload.
     * @param requireReply Whether to require replies from other participants (except of function code 0)
     */
    static bool initTopology(uint8_t participants, uint8_t myPayload, bool requireReply = false);

};

#endif