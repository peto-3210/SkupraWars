#include "hal/uart.h"
#include "SoftwareTimer.hpp"

#ifndef DATALINK
#define DATALINK

class Datalink {
    public:

    enum recvPacketState{
        noPacket,
        packetReceived,
        packetForOtherParticipant,
        packetError,
    };

    private:
    // Private Constructor
    Datalink() = default;

    // delete copy/move
    Datalink(const Datalink&) = delete;
    Datalink& operator=(const Datalink&) = delete;

    typedef enum {
        sendBufferFull,
        crcError,
    } commError;

    static constexpr const char* errorMessages[] = {
        "Send buffer full.",
        "CRC error in message."
    };

    static const uint8_t PACKET_LENGTH = 2;
    static const uint8_t DATALINK_PACKET_LENGTH = 3;

    static bool initialized;
    static const char* errorMsg;
    
    static uint8_t datalinkPacket[DATALINK_PACKET_LENGTH];

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
    static bool sendPacket(uint16_t& packet);
    
    /**
     * @brief Receives packet through physical interface.
     * The first 3 bits represent address, 4-th bit is for broadcast
     * 
     * @return Receied packt state.
     */
    static recvPacketState recvPacket(uint16_t& packet); 

    
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
     * @note Replying is always off for the broadcast packets
     */
    static void initDatalink(baudRate rate);

};

#endif