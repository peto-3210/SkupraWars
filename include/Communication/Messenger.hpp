#include "Datalink.hpp"
#include "Gameplay.hpp"

#ifndef MESSENGER
#define MESSENGER

using recvPacketState = Datalink::recvPacketState;
using Packet = Datalink::Packet;
using packetPayload = Datalink::packetPayload;
class Messenger {
    
    private:
    static const uint8_t MAX_NEIGHBOURS = 7;
    static const uint8_t RECEIVED_MESSAGES_BUFFER_SIZE = 8;
    static const uint8_t PENDING_REPLY_BUFFER_SIZE = 4;
    static const uint32_t REPLY_TIMEOUT_US = 10000;
    static const uint8_t HALF_SCREEN_LENGTH = 64;

    enum functionCode{
        announceFun,
        shootProjectileFun,
        tellHPFun,
        tellPositionFun
    };

    /*
     Announcement payload compositions:
     
     topologyInitAnn:   NUMBER OF PARTICIPANTS (3 bits) | PARTICIPANT ID (3 bits)
     deathAnn:          KILLER ID (3 bits) | VICTIM ID (3 bits)
     debufAnn:          AUTHOR ID (3 bits) | DEBUF TYPE (3 bits)
    */
    enum announcementType{
        topologyInitAnn,
        deathAnn,
        debufAnn,
        songSyncAnn,
    };

    struct receivedMessages{
        Packet messages[RECEIVED_MESSAGES_BUFFER_SIZE];
        uint8_t messageNum;
        uint8_t messageReadIterator;

        bool addMessage(Packet p){
            if (messageNum == RECEIVED_MESSAGES_BUFFER_SIZE){
                return false;
            }
            messages[messageNum++] = p;
            return true;
        }

        Packet getMessage(){
            if (messageNum == 0){
                return (Packet){};
            }
            Packet p = messages[messageReadIterator++];
            if (messageReadIterator >= messageNum){
                messageReadIterator = 0;
                messageNum = 0;
            }
            return p;
        }

        uint8_t getMessageNum(){
            return messageNum - messageReadIterator;
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
        "Received two identical initialization messages.",
        "Missing reply from participant.",
        "Pending reply buffer full.",
    };

    static bool initialized;
    static const char* errorMsg;
    static receivedMessages messageBuffer;
    static receivedMessages announcementBuffer;

    static bool replying;
    static replyBufferEntry pendingReplyBuffer[PENDING_REPLY_BUFFER_SIZE];
    static uint8_t pendingReplyNumber;

    static uint8_t myId;
    static uint8_t neighbourNum;
    static uint8_t neighbours[MAX_NEIGHBOURS];
    static bool neighbourActive[MAX_NEIGHBOURS];


    /**
     * @brief Sends messages 
     * 
     * @param packet packet to send
     * @returns False if error occured, true otherwise
     */
    static bool sendMessage(Packet& packet);

    /**
     * @brief Receives messages from other neighbours. 
     * Packets will be stored in internall buffer.
     * 
     * @param promiscuousMode If true, packets for another neighbours will be
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

    /**
     * @brief Finds closest neighbour in specified direction
     * 
     * @param direction true for default, false for reverse
     * @param id Fills in ID of neighbour
     * @return Distance of neighbour (always in default direction), or MAX_NEIGHBOURS if no neighbour is active
     */
    static uint8_t findNeighbour(bool direction, uint8_t* id = nullptr);

    /**
     * @brief sends message to the neighbour
     * 
     * @param packet packet to send
     * @param direction true for packet sent in direction of participant with distance 0, false for opposite direction
     */
    static bool sendToNeighbour(Packet& packet, bool direction);
        
    /**
     * @brief Sends announcement packet, setting the distance to reach everyone in topology
     * 
     * @param packet Packet to send
     */
    static bool sendAnnouncement(Packet& packet);


    public:
    /**
     * @brief Function sends packet to tell everyone
     * who is it, then listens for messages from other 
     * neighbours. Also checks whether the ring
     * is closed.
     * 
     * @param nodeNum Total number of nodes in topology, including self.
     * @param myId Self-identification payload.
     * @param requireReply Whether to require replies from other neighbours 
     */
    static bool initTopology(uint8_t nodeNum, uint8_t myId, bool requireReply = false);

    /**
     * @brief Disables participant
     * 
     * @param neighbourId Participant ID
     */
    static void disableNeighbour(uint8_t neighbourId);

    /**
     * @brief Sends message about projectle
     * 
     * @param type Projectile type
     * @param position Projectile position
     * 
     * @returns False if error occured, true otherwise
     */
    static bool sendProjectile(WeaponType projectile, uint8_t position);

    /**
     * @brief Sends message about HP and who hit me
     * 
     * @param newHP Current HP
     * @param position Current position
     * 
     * @returns False if error occured, true otherwise
     */
    static bool sendHP(uint8_t hp, uint8_t position);

    /**
     * @brief Retrieves single packet from messageBuffer
     * 
     * @param packet Packet that will be filled with data
     * @return Function code, or 0 if no packet is available
     */
    static uint8_t getPacket(packetPayload& p);

    /**
     * @brief Retrieves single packet from announcementBuffer
     * 
     * @param packet Packet that will be filled with data
     * @return True if packet is available, false otherwise
     */
    static bool getAnnouncement(packetPayload& p);

    /**
     * @brief Receives and forwards packets, should be called in loop
     */
    static void commLoop();
};


#endif