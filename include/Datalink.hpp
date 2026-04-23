#include "hal/uart.h"
#include "SoftwareTimer.hpp"

#ifndef DATALINK
#define DATALINK

class Datalink {
    public:
    enum functionCode{
        announceFun,
        shootProjectileFun,
        tellHPFun,
        specialFun
    };

    enum announcementType{
        topologyInitAnn,
        songSyncAnn,
        projectileHitAnn,
        reservedAnn,
    };

    enum recvPacketState{
        noPacket,
        packetReceived,
        packetForOtherParticipant,
        packetError,
    };

    /*Announcement packet:
    | ANNOUNCEMENT TYPE (2 bits) | PAYLOAD (6 bits) |

    Possible announcement types:
    - Topology initialization: Payload is self-identification payload, which can be used to identify participants
    - Song synchronization: Payload is the current position in the song, which can be used to synchronize music playback
    - Projectile hit: Payload is the position of the hit
    */
    union announcePayload{
        uint8_t rawData;
        struct {
            uint8_t payload         :6;
            announcementType type   :2;
        };
    };

    /*
    Projectile packet:
    | PROJECTILE TYPE (2 bits) | POSITION (6 bits) |

    Possible projectile types:
    - Torpedo: Normal projectile
    - Missisle: Slow but powerful with splash damage
    - Laser: Instant projectile with long cooldown
    - Grenade: Weaker, but fired in burst of 5
    */
    union projectilePayload{
        uint8_t rawData;
        struct {
            uint8_t position    :6;
            uint8_t type        :2;
        };
    };

    /*
    HP packet:
    | YOU_HIT_ME (1 bit) | HP (7 bits) |

    YOU_HIT_ME bit is set to 1 when the sender is hit by a projectile from receiver, and 0 otherwise.
    */
    union hpPayload{
        uint8_t rawData;
        struct {
            uint8_t hp          :7;
            uint8_t youHitMe    :1;
        };
    };

    union specialPayload{
        uint8_t rawData;
        struct {
            uint8_t position    :6;
            uint8_t type        :2;
        };
    };





    private:
    // Private Constructor
    Datalink() = default;

    // delete copy/move
    Datalink(const Datalink&) = delete;
    Datalink& operator=(const Datalink&) = delete;

    /**
     * @brief Union representing a data packet:
     * 
     * | HEADER (1 byte) | PAYLOAD (1 byte) | CRC (1 byte) |
     * 
     * Header:
     * 
     * | FUNCTION (2 bits) | DUMMY (1 bit) | REPLY (1 bit) | DIRECTION (1 bit) | DISTANCE (3 bits)
     * 
     * @param FUNCTION Function code of the packet, which determines how the payload should be interpreted
     * @param DUMMY: Dummy bit, reserved for future use
     * @param REPLY: Whether the packet is a reply to a previous packet. Reply packets are sent in response to received packets when replying is on, 
     * and have the same function code and payload as the original packet.
     * @param DIRECTION: Direction of the packet, which determines the direction in which the packet is sent and received. 
     * 0 for packet sent in direction of participant with distance 0, 1 for opposite direction.
     * @param DISTANCE: Distance to the sender of the packet. Highest number (7) means broadcast message, 0 means packet from participant with distance 0, etc.
     * @param PAYLOAD: The payload of the packet, which contains the actual data being transmitted.
     * 
     * @note The CRC part is only visible to the physical layer, and is not included in the Packet struct. 
     * 
    **/
    union Packet {
        uint8_t rawPacket[2];
        struct {
            uint8_t function    :2;
            uint8_t dummy       :1;
            uint8_t reply       :1;
            uint8_t direction   :1; // 0 for packet sent in direction of participant with distance 0, 1 for opposite direction
            uint8_t distance    :3; // Highest number means broadcast message

            union {
                uint8_t rawPayload;
                announcePayload announcement;
                projectilePayload projectile;
                hpPayload hp;
                specialPayload special;
            };
        };
        Packet() = default;
        Packet(uint8_t distance, functionCode func, uint8_t rawPayload, bool direction, bool reply = false){
            this->function = func;
            this->dummy = 0;
            this->reply = reply;
            this->direction = direction;
            this->distance = distance;
            this->rawPayload = rawPayload;
        }
    };

    struct replyBufferEntry{
        Packet sentPacket;
        uint32_t timestamp;
    };

    typedef enum {
        sendBufferFull,
        notEnoughBytes,
        crcError,
        ringNotClosed,
        unknownParticipant,
        multipleIdentification,
        missingReply,
        replyBufferFull,
    } commError;

    static constexpr const char* errorMessages[] = {
        "Send buffer full.",
        "Not enough bytes received.",
        "CRC error in message.",
        "Topology ring is not closed.",
        "Missing initialization message from participant.",
        "Received two identical initialization messages."
        "Missing reply from participant.",
        "Pending reply buffer full.",
    };

    static const uint8_t PACKET_LENGTH = 2;
    static const uint8_t DATALINK_PACKET_LENGTH = 3;
    static const uint8_t MAX_PARTICIPANTS = 8;
    
    static const uint8_t PENDING_REPLY_BUFFER_SIZE = 4;
    static const uint32_t REPLY_TIMEOUT_US = 10000;

    static bool initialized;
    static bool replying;
    static uint8_t nodeNum;
    static uint8_t nodes[MAX_PARTICIPANTS];
    
    static uint8_t datalinkPacket[DATALINK_PACKET_LENGTH];
    static uint32_t packetReceiveTimeoutUS;
    static SoftwareTimer* dataReceiveTimer;

    static replyBufferEntry pendingReplyBuffer[PENDING_REPLY_BUFFER_SIZE];
    static uint8_t pendingReplyNumber;

    static const char* errorMsg;
    static uint8_t errorData;

    static bool calculateCRC(uint8_t data[DATALINK_PACKET_LENGTH], bool direction);
    static bool sendPacket(Packet& packet);
    static recvPacketState recvPacket(Packet& packet); 
    static void flushRX();
    
    /**
     * @brief Sets error message
     * 
     * @param err Error message
     */
    static void setError(commError err){
        errorMsg = errorMessages[err];
    }

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
     * @param requireReply Whether to require replies from other participants 
     * 
     * @note Replying is always off for the broadcast packets
     */
    static void initDatalink(baudRate rate, bool requireReply = false);

    /**
     * @brief Function sends packet to tell everyone
     * who is it, then listens for messages from other 
     * participants. Also checks whether the ring
     * is closed.
     * 
     * @param participants Total number of participants in topology, including self.
     * @param myPayload Self-identification payload.
     */
    static bool initTopology(uint8_t participants, uint8_t myPayload);

};

#endif