#include "Datalink.hpp"

using recvPacketState = Datalink::recvPacketState;

class Messenger {
    
    private:
    static const uint8_t MAX_NODES = 8;
    static const uint8_t RECEIVED_MESSAGES_BUFFER_SIZE = 8;
    static const uint8_t PENDING_REPLY_BUFFER_SIZE = 4;
    static const uint32_t REPLY_TIMEOUT_US = 10000;

    enum functionCode{
        announceFun,
        shootProjectileFun,
        tellHPFun,
        reservedFun
    };

    enum announcementType{
        topologyInitAnn,
        songSyncAnn,
        positionAnn,
        specialAnn,
    };

    /*Announcement packet:
    | ANNOUNCEMENT TYPE (2 bits) | PAYLOAD (6 bits) |

    Possible announcement types:
    - Topology initialization: Payload is self-identification payload, which can be used to identify nodes
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

    union reservedPayload{
        uint8_t rawData;
        struct {
            uint8_t position    :6;
            uint8_t type        :2;
        };
    };

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
     * 0 for packet sent in direction of participant with distance 1, 0 for opposite direction.
     * DEFAULT DIRECTION IS COUNTER-CLOCKWISE!
     * @param DISTANCE: Distance to the sender of the packet. Highest number (7) means broadcast message, 0 means packet from participant with distance 0, etc.
     * @param PAYLOAD: The payload of the packet, which contains the actual data being transmitted.
     * 
     * @note The CRC part is only visible to the physical layer, and is not included in the Packet struct. 
     * 
    **/
    union Packet {
        uint16_t rawPacket;
        struct {
            uint8_t distance    :3; 
            uint8_t broadcast   :1; // true if packet is broadcast
            uint8_t reply       :1; // true in case of reply packet
            uint8_t direction   :1; // 0 for packet sent in direction of participant with distance 0, 1 for opposite direction
            uint8_t function    :2;
            
            union {
                uint8_t rawPayload;
                announcePayload announcement;
                projectilePayload projectile;
                hpPayload hp;
            };

        };
        Packet() = default;
        Packet(uint8_t distance, uint8_t func, uint8_t rawPayload, bool direction, bool broadcast = false){
            this->function = func;
            this->direction = direction;
            this->broadcast = broadcast;
            this->distance = distance;
            this->rawPayload = rawPayload;
        }
    };

    struct receivedMessages{
        Packet messages[RECEIVED_MESSAGES_BUFFER_SIZE];
        uint8_t messageNum;

        bool addMessage(Packet p){
            if (messageNum == RECEIVED_MESSAGES_BUFFER_SIZE){
                return false;
            }
            messages[messageNum++] = p;
            return true;
        }

        Packet getMessage(){
            return messageNum > 0 ? messages[messageNum--] : (Packet){};
        }

        uint8_t getMessageNum(){
            return messageNum;
        }
    };
    
    struct replyBufferEntry{
        Packet sentPacket;
        uint32_t timestamp;
    };

    typedef enum {
        ringNotClosed,
        unknownParticipant,
        multipleIdentification,
        missingReply,
        replyBufferFull,
    } commError;

    static constexpr const char* errorMessages[] = {
        "Topology ring is not closed.",
        "Missing initialization message from participant.",
        "Received two identical initialization messages."
        "Missing reply from participant.",
        "Pending reply buffer full.",
    };

    static bool initialized;
    static const char* errorMsg;
    static receivedMessages messageBuffer;

    static bool replying;
    static replyBufferEntry pendingReplyBuffer[PENDING_REPLY_BUFFER_SIZE];
    static uint8_t pendingReplyNumber;

    static uint8_t nodeNum;
    static uint8_t nodes[MAX_NODES -1];
    static bool nodeActive[MAX_NODES -1];


    /**
     * @brief Sends messages 
     * 
     * @param packet packet to send
     * @returns False if error occured, true otherwise
     */
    static bool sendMessage(Packet& packet);

    /**
     * @brief Receives messages from other nodes. 
     * Packets will be stored in internall buffer.
     * 
     * @param promiscuousMode If true, packets for another nodes will be
     * received as normal packets.
     * @returns False in case of error, true otherwise
     */
    static bool recvMessages(bool promiscuousMode = false);

    /**
     * @brief Sets error message
     * 
     * @param err Error message
     */
    static void setError(commError err){
        errorMsg = errorMessages[err];
    }

    public:
    /**
     * @brief Function sends packet to tell everyone
     * who is it, then listens for messages from other 
     * nodes. Also checks whether the ring
     * is closed.
     * 
     * @param nodes Total number of nodes in topology, including self.
     * @param myId Self-identification payload.
     * @param requireReply Whether to require replies from other nodes 
     */
    static bool initTopology(uint8_t nodes, uint8_t myId, bool requireReply = false);

    /**
     * @brief Disables participant
     * 
     * @param nodeId Participant ID
     */
    static void disableNode(uint8_t nodeId);

    /**
     * @brief sends message to the neighbour
     * 
     * @param packet packet to send
     * @param direction true for packet sent in direction of participant with distance 0, false for opposite direction
     */
    static bool sendToNeighbour(Packet& packet, bool direction);
};
