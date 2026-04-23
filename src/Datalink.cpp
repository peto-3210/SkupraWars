#include "Datalink.hpp"

bool Datalink::initialized = false;
bool Datalink::replying = false;
uint8_t Datalink::nodeNum = false;
uint8_t Datalink::nodes[MAX_PARTICIPANTS] = {0};

uint8_t Datalink::datalinkPacket[DATALINK_PACKET_LENGTH] = {0};
uint32_t Datalink::packetReceiveTimeoutUS = 0;
SoftwareTimer* Datalink::dataReceiveTimer = nullptr;

Datalink::replyBufferEntry Datalink::pendingReplyBuffer[PENDING_REPLY_BUFFER_SIZE] = {};
uint8_t Datalink::pendingReplyNumber = 0;

const char* Datalink::errorMsg = nullptr;
uint8_t Datalink::errorData = 0;

//Maxim-DOW CRC-8 precalculated values
static const uint8_t CRC_8_TABLE[256] = 
{
	  0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	 35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	 87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

/**
 * @brief Calculates CRC8 for message.
 * 
 * @param data Data buffer
 * @param direction If false, calculates CRC and appends it to the end 
 *  of buffer. If true, calculates CRC and compares it to the value received from packet.
 * 
 * @return True if received and calculated CRC match, false otherwise.
 *  If direction == false, return value is always true.
 */
bool Datalink::calculateCRC(uint8_t data[DATALINK_PACKET_LENGTH], bool direction){
	uint8_t CRC = 0;

	for (uint8_t i = 0; i < PACKET_LENGTH; i++){
    	CRC = CRC_8_TABLE[CRC ^ data[i]];
	}

    if (direction == false){
        data[2] = CRC;
        return true;
    }
    return data[2] == CRC;
}

void Datalink::initDatalink(Datalink::baudRate rate, bool requireReply){
    if (initialized == false){
        switch (rate){
			case b9600:
				uart_begin(BAUD_RATE_CONSTANT_9600);
				break;

			case b19200:
				uart_begin(BAUD_RATE_CONSTANT_19200);
				break;

			case b115200:
				uart_begin(BAUD_RATE_CONSTANT_115200);
				break;
		}

		dataReceiveTimer = SoftwareTimerPool::acquireTimer();
		packetReceiveTimeoutUS = (1/rate) * 11 * DATALINK_PACKET_LENGTH * 2;
		replying = requireReply;
		initialized = true;
    }
}


/**
 * @brief Sends data through physical interface
 * 
 * @return True if all data were sent, false otherwise
 */
bool Datalink::sendPacket(Packet& packet){
	memcpy(datalinkPacket, packet.rawPacket, PACKET_LENGTH);
	calculateCRC(datalinkPacket, false);
	if (uart_send(datalinkPacket, DATALINK_PACKET_LENGTH) != DATALINK_PACKET_LENGTH){
		setError(sendBufferFull);
		return false;
	};

	if (replying == true){
		pendingReplyBuffer[pendingReplyNumber++] = {packet, SoftwareTimer::getTimestampUs()};
		if (pendingReplyNumber >= PENDING_REPLY_BUFFER_SIZE){
			setError(replyBufferFull);
			return false;
		}
	}
	return true;
}

/**
 * @brief Receives packet through physical interface.
 * 
 * @return Receied packt state.
 */
Datalink::recvPacketState Datalink::recvPacket(Packet& packet){
	//Reply timeout check
	if (replying == true && pendingReplyNumber > 0){
		for (uint8_t i = 0; i < pendingReplyNumber; ++i){

			//Test if the reply matches any of the pending packets
			if (SoftwareTimer::getTimestampUs() - pendingReplyBuffer[i].timestamp > REPLY_TIMEOUT_US){

				//Shift all entries after the found one to the left
				for (uint8_t j = i; j < pendingReplyNumber; ++j){
					pendingReplyBuffer[j] = pendingReplyBuffer[j + 1];
				}
				pendingReplyNumber--;
			}
		}
	}


	//No packet
	if (uart_is_RX_empty() == true){
		return noPacket;
	}


	//Receiving packet
	int recvNum = 0;
	dataReceiveTimer->startTimerUs(packetReceiveTimeoutUS);
	while (recvNum < DATALINK_PACKET_LENGTH && dataReceiveTimer->isDone() == false){
		recvNum += uart_recv(datalinkPacket + recvNum, DATALINK_PACKET_LENGTH - recvNum);
	}
	dataReceiveTimer->stop();


	//Check for errors
	if (recvNum != DATALINK_PACKET_LENGTH){
		setError(notEnoughBytes);
		return packetError;
	}
	if (calculateCRC(datalinkPacket, true) == false){
		setError(crcError);
		return packetError;
	}
	memcpy(packet.rawPacket, datalinkPacket, PACKET_LENGTH);


	//Packet is for this participant
	if (packet.distance == 0){
		//Replying is on
		if (replying == true){

			//Received reply packet
			if (packet.reply == true){
				for (uint8_t i = 0; i < pendingReplyNumber; ++i){

					//Test if the reply matches any of the pending packets
					if (pendingReplyBuffer[i].sentPacket.direction == !packet.direction &&
						pendingReplyBuffer[i].sentPacket.function == packet.function &&
						pendingReplyBuffer[i].sentPacket.rawPayload == packet.rawPayload){

						//Shift all entries after the found one to the left
						for (uint8_t j = i; j < pendingReplyNumber; ++j){
							pendingReplyBuffer[j] = pendingReplyBuffer[j + 1];
						}
						pendingReplyNumber--;
					}
				}
				return noPacket;
			}

			//Received regular packet, send reply
			else {
				packet.reply = true;
				if (packet.direction == false){
					packet.direction = true;
					packet.distance = nodeNum - 1;
				}
				else {
					packet.direction = false;
					packet.distance = 0;
				}

				if (sendPacket(packet) == false){
					return packetError;
				}
				return packetReceived;
			}
		}
		return packetReceived;
	}

	//Sends packet to next participant
	else {
		packet.distance--;
		if (sendPacket(packet) == false){
			return packetError;
		}
		return packetForOtherParticipant;
	}
}

/**
 * @brief Flushes RX buffer
*/
void Datalink::flushRX(){
	uint8_t dummy = 0;
	while (uart_recv(&dummy, 1) > 0);
}

bool Datalink::initTopology(uint8_t participants, uint8_t myPayload){
	nodeNum = participants;
	Packet packet(nodeNum - 1, announceFun, myPayload, false);

	if (sendPacket(packet) == false){
		return false;
	}

	//Read messages from other participants.
	uint8_t receivedNum = 0;
	uint8_t retryNum = 0;
	while (receivedNum < nodeNum - 1){
		recvPacketState state = recvPacket(packet);

		//Check if received mesage is correct
		if (state == packetForOtherParticipant
			&& packet.function == announceFun && packet.announcement.type == topologyInitAnn){
			nodes[receivedNum] = packet.announcement.payload;

			//Check for duplicit identification
			for (uint8_t j = 0; j < receivedNum; ++j){
				if (nodes[j] == nodes[receivedNum]){
					setError(multipleIdentification);
					return false;
				}
			}
			receivedNum++;
		}

		//Waits until packet arrives
		else if (state == noPacket){
			SoftwareTimerPool::busyWaitUs(1000);
			retryNum++;
			if (retryNum > 1000){
				setError(missingReply);
				return false;
			}
		}

		//Error occured
		else {
			return false;
		}
	}

	//Received own packet
	if (recvPacket(packet) == packetReceived && packet.function == announceFun && 
		packet.announcement.type == topologyInitAnn && packet.announcement.payload == myPayload){
			return true;
		}
	setError(ringNotClosed);
	return false;
}


