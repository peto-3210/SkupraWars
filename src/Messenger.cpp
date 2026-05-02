#include "Messenger.hpp"

bool Messenger::initialized = false;
bool Messenger::replying = false;
const char* Messenger::errorMsg = nullptr;
Messenger::receivedMessages Messenger::messageBuffer = {};

Messenger::replyBufferEntry Messenger::pendingReplyBuffer[PENDING_REPLY_BUFFER_SIZE] = {};
uint8_t Messenger::pendingReplyNumber = 0;

uint8_t Messenger::nodeNum = 0;
uint8_t Messenger::nodes[MAX_NODES - 1] = {};
bool Messenger::nodeActive[MAX_NODES - 1] = {};


bool Messenger::sendMessage(Packet& packet){
    if (Datalink::sendPacket(packet.rawPacket) == false){
        return false;
    }

    if (replying == true){
        pendingReplyBuffer[pendingReplyNumber++] = {packet, SoftwareTimer::getTimestampUs()};
        if (pendingReplyNumber >= PENDING_REPLY_BUFFER_SIZE){
            setError(replyBufferFull);
            return false;
        }
    }
    return true;
}



bool Messenger::recvMessages(bool promiscuousMode){
    Packet packet;
    recvPacketState state;
    do {
        state = Datalink::recvPacket(packet.rawPacket);

        //Reply timeout check
        if (replying == true && pendingReplyNumber > 0){
            for (uint8_t i = 0; i < pendingReplyNumber; ++i){

                //Test if the reply matches any of the pending packets
                //TODO: something
                if (SoftwareTimer::getTimestampUs() - pendingReplyBuffer[i].timestamp > REPLY_TIMEOUT_US){

                    //Shift all entries after the found one to the left
                    for (uint8_t j = i; j < pendingReplyNumber; ++j){
                        pendingReplyBuffer[j] = pendingReplyBuffer[j + 1];
                    }
                    pendingReplyNumber--;
                }
            }
        }

        switch (state){
            case recvPacketState::packetError:
                return false;

            case recvPacketState::packetReceived:
                //Replying is on
                if (replying == true){
                    //Received reply packet, ignore unsolicited reply
                    if (packet.reply == true){
                        for (uint8_t i = 0; i < pendingReplyNumber; ++i){

                            //Test if the reply matches any of the pending packets
                            if (pendingReplyBuffer[i].sentPacket.direction == !packet.direction &&
                                pendingReplyBuffer[i].sentPacket.function == packet.function &&
                                pendingReplyBuffer[i].sentPacket.rawPayload == packet.rawPayload){

                                //Shift all entries after the found one to the left
                                pendingReplyNumber--;
                                for (uint8_t j = i; j < pendingReplyNumber; ++j){
                                    pendingReplyBuffer[j] = pendingReplyBuffer[j + 1];
                                }
                                break;
                            }
                        }
                    }

                    //Received regular packet, send reply
                    else if (packet.broadcast == false) {
                        packet.reply = true;
                        if (packet.direction == false){
                            packet.direction = true;
                            packet.distance = nodeNum - 1;
                        }
                        else {
                            packet.direction = false;
                            packet.distance = 0;
                        }

                        if (Datalink::sendPacket(packet.rawPacket) == false){
                            return -1;
                        }
                    }
                }

                else {
                    if (packet.reply == false){
                        messageBuffer.addMessage(packet);
                    }
                }
                break;

            case recvPacketState::packetForOtherParticipant:
                if (promiscuousMode == true){
                    messageBuffer.addMessage(packet);
                }

            default: break;
        }
    } while (state != recvPacketState::noPacket);
    return true;
}



bool Messenger::initTopology(uint8_t nodesNumber, uint8_t myId, bool requireReply){
	nodeNum = nodesNumber;
	Packet packet(nodeNum - 1, announceFun, myId, true);

	if (sendMessage(packet) == false){
		return false;
	}

	//Read messages from other nodes.
	uint8_t receivedNum = 0;
	uint8_t retryNum = 0;

	while (receivedNum < nodeNum - 1){
		if (recvMessages() == false){
            return false;
        };

        if (messageBuffer.getMessageNum() == 0){
            SoftwareTimerPool::busyWaitUs(1000);
			retryNum++;
			if (retryNum > 1000){
				setError(missingReply);
				return false;
			}
        }

        else {
            Packet a = messageBuffer.getMessage();
            if (a.function == announceFun && packet.broadcast == true && a.announcement.type == topologyInitAnn){
                nodes[receivedNum] = a.announcement.payload;
                nodeActive[receivedNum] = true;

                //Check for duplicit identification
                for (uint8_t j = 0; j < receivedNum; ++j){
                    if (nodes[j] == nodes[receivedNum]){
                        setError(multipleIdentification);
                        return false;
                    }
                }
                receivedNum++;
            }
        }
	}

	
    if (recvMessages() == false){
        return false;
    }
    packet = messageBuffer.getMessage();
    //Received own packet - set replying and return true
	if (packet.function == announceFun && packet.broadcast == true &&
		packet.announcement.type == topologyInitAnn && packet.announcement.payload == myId){
            replying = requireReply;
			return true;
		}
	setError(ringNotClosed);
	return false;
}

void Messenger::disableNode(uint8_t nodeId){
    for (uint8_t i = 0; i < nodeNum; ++i){
        if (nodes[i] == nodeId){
            nodeActive[i] == false;
        }
    }
}

bool Messenger::sendToNeighbour(Packet& packet, bool direction){
    packet.direction = direction;
    uint8_t iterator;
    //Find the next active neighbour
    if (direction == false){
        iterator = 0;
        while (iterator < nodeNum && nodeActive[iterator] == false){
            ++iterator;
        }
        if (iterator == nodeNum){
            return false;
        }
    }

    else {
        iterator = nodeNum - 1;
        while (iterator < 0 && nodeActive[iterator] == false){
            --iterator;
        }
        if (iterator == 0 && nodeActive[iterator] == false){
            return false;
        }
    }

    packet.distance = iterator;
    return sendMessage(packet);
}
