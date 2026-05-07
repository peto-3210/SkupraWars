#include "hal/uart.h"
#include "Utilities/SoftwareTimer.hpp"

#ifndef DATALINK
#define DATALINK

class Datalink {
    public:

    /*Announcement packet:
    | ANNOUNCEMENT TYPE (2 bits) | PAYLOAD (6 bits) |

    Possible announcement types:
    - Topology initialization: Payload is self-identification payload, which can be used to identify nodes
    - Death announcement: Payload is the ID of my killer and mine
    - Song synchronization: Payload is the current position in the song, which can be used to synchronize music playback
    - Projectile hit: Payload is the position of the hit

    ALL ANNOUNCEMENTS ARE BROADCASTS - THEY ARE RECEIVED BY EVERY DEVICE REGARDLESS OF DISTACE
    */
    union announcePayload{
        uint8_t rawData;
        struct {
            uint8_t payload         :6;
            uint8_t type   :2;
        };
    };

    /*
    Projectile packet:
    | PROJECTILE TYPE (2 bits) | POSITION (6 bits) |
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
            uint8_t value          :7;
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

    union packetPayload {
        uint8_t rawPayload;
        announcePayload announcement;
        projectilePayload projectile;
        hpPayload hp;
    };

    /**
     * @brief Union representing a data packet:
     * 
     * | HEADER (1 byte) | PAYLOAD (1 byte) | CRC (1 byte) |
     * 
     * Header:
     * 
     * |  DISTANCE (3 bits) | REPLY (1 bit) | DUMMY (1 bit)  | DIRECTION (1 bit) | FUNCTION (2 bits) 
     * 
     * @param DISTANCE: Distance to the sender of the packet. 0 means packet from participant with distance 0 (next one in default direction), etc.
     * @param REPLY: Whether the packet is a reply to a previous packet. Reply packets are sent in response to received packets when replying is on, 
     * and have the same function code and payload as the original packet.
     * @param DUMMY: Not implemented
     * @param DIRECTION: Direction of the packet, which determines the direction in which the packet is sent and received. 
     * 0 for packet sent in direction of participant with distance 1, 0 for opposite direction.
     * POSITIVE AND DEFAULT DIRECTION IS CLOCKWISE!
     * @param FUNCTION Function code of the packet, which determines how the payload should be interpreted
     
     * @param PAYLOAD: The payload of the packet, which contains the actual data being transmitted.
     * 
     * @note Only Function and Payload are useful for gameplay logic, the rest is communication-specific
     * @note The CRC part is only visible to the physical layer, and is not included in the Packet struct. 
     * 
    **/
    union Packet {
        uint8_t rawPacket[2];
        struct {
            uint8_t distance    :3; 
            uint8_t reply       :1; // true in case of reply packet
            uint8_t dummy       :1;
            uint8_t direction   :1; // 1 for packet sent in default direction, 1 for opposite direction
            uint8_t function    :2;

            packetPayload payload;

        };
        Packet() = default;
    };

    enum recvPacketState{
        noPacket,
        packetReceived,
        announcementReceived,
        packetForOtherParticipant,
        //packetError,
    };

    private:
    // Private Constructor
    Datalink() = default;

    // delete copy/move
    Datalink(const Datalink&) = delete;
    Datalink& operator=(const Datalink&) = delete;

    typedef enum {
        sendBufferFull,
    } commError;

    static constexpr const char* errorMessages[] = {
        "Send buffer full.",
    };

    static const uint8_t PACKET_LENGTH = 2;
    static const uint8_t DATALINK_PACKET_LENGTH = 3;

    static bool initialized;
    static const char* errorMsg;

    static bool calculateCRC(uint8_t data[DATALINK_PACKET_LENGTH], bool direction);
    
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
     * @brief Sends data through physical interface
     * 
     * @return True if all data were sent, false otherwise
     */
    static bool sendPacket(Packet& packet);
    
    /**
     * @brief Receives packet through physical interface.
     * 
     * @return Receied packt state.
     */
    static recvPacketState recvPacket(Packet& packet); 

    
    /**
     * @brief Flushes RX buffer
     */
    static void flushRX(){
	    uart_flush_rx();
    }


    /**
     * @brief Returns error message
     */
    static const char* getError(){
        return errorMsg;
    }

    /**
     * @brief Initializes physical layer
     * 
     * @param rate Baud rate
     * 
     */
    static void initDatalink(baudRate rate = b115200);

};

#endif