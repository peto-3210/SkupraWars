#include "Communication/Messenger.hpp"

bool Messenger::initialized = false;
bool Messenger::replying = false;
const char* Messenger::errorMsg = nullptr;
Messenger::receivedMessages Messenger::messageBuffer = {};
Messenger::receivedMessages Messenger::announcementBuffer = {};

Messenger::replyBufferEntry Messenger::pendingReplyBuffer[PENDING_REPLY_BUFFER_SIZE] = {};
uint8_t Messenger::pendingReplyNumber = 0;

uint8_t Messenger::myId = 0;
uint8_t Messenger::neighbourNum = 0;
uint8_t Messenger::neighbours[MAX_NEIGHBOURS] = {};
bool Messenger::neighbourActive[MAX_NEIGHBOURS] = {};


bool Messenger::sendMessage(Packet& packet){
    if (Datalink::sendPacket(packet) == false){
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
    Packet packet = {0};
    recvPacketState state;
    do {
        state = Datalink::recvPacket(packet);

        //Reply timeout check
        if (replying == true && pendingReplyNumber > 0){
            for (uint8_t i = 0; i < pendingReplyNumber; ++i){

                //Test if the reply matches any of the pending packets
                //TODO: something if no packet is matched
                if (SoftwareTimer::getTimestampUs() - pendingReplyBuffer[i].timestamp > REPLY_TIMEOUT_US){

                    //Shift all entries after the found one to the left
                    for (uint8_t j = i; j < pendingReplyNumber; ++j){
                        pendingReplyBuffer[j] = pendingReplyBuffer[j + 1];
                    }
                    pendingReplyNumber--;
                    i--;
                }
            }
        }

        switch (state){

            case recvPacketState::packetReceived:
                //Replying is on
                /*if (replying == true){
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
                    else if (packet.announcement == false) {
                        messageBuffer.addMessage(packet);

                        packet.reply = true;
                        if (packet.direction == false){
                            packet.direction = true;
                            packet.distance = neighbourNum - 1;
                        }
                        else {
                            packet.direction = false;
                            packet.distance = 0;
                        }

                        if (Datalink::sendPacket(packet) == false){
                            return false;
                        }
                    }
                }

                else {
                    if (packet.reply == false){
                        messageBuffer.addMessage(packet);
                    }
                }
                break;*/

                messageBuffer.addMessage(packet);
                break;

            case recvPacketState::announcementReceived:
                announcementBuffer.addMessage(packet);
                break;

            case recvPacketState::packetForOtherParticipant:
                if (promiscuousMode == true){
                    messageBuffer.addMessage(packet);
                }
                break;

            default: break;
        }
    } while (state != recvPacketState::noPacket);
    return true;
}



bool Messenger::initTopology(uint8_t nodeNum, uint8_t self, bool requireReply){
	neighbourNum = nodeNum - 1;
    myId = self;
	Packet packet = {0};
    packet.distance = 7;
    packet.function = announceFun;
    packet.direction = true;
    packet.payload.announcement.payload = (neighbourNum << 3) | myId;
    uart_flush_rx();

	if (sendMessage(packet) == false){
		return false;
	}

	//Read messages from other neighbours.
	uint8_t receivedNum = 0;
	uint32_t retryNum = 0;

	while (receivedNum < neighbourNum){
		recvMessages();

        if (announcementBuffer.getMessageNum() == 0){
            SoftwareTimerPool::busyWaitUs(1000);
			retryNum++;
			if (retryNum > 1000000){
				setError(missingReply);
				return false;
			}
        }

        else {
            Packet ann = announcementBuffer.getMessage();
            if (ann.function == announceFun &&
                ann.payload.announcement.type == topologyInitAnn){
                neighbours[receivedNum] = ann.payload.announcement.payload & 0b111;
                neighbourActive[receivedNum] = true;

                //Check for duplicit identification (including myself) and correct participant num
                if (myId == neighbours[receivedNum] || (ann.payload.announcement.payload >> 3) != neighbourNum){
                        setError(multipleIdentification);
                        return false;
                    }
                for (uint8_t j = 0; j < receivedNum; ++j){
                    if (neighbours[j] == neighbours[receivedNum]){
                        setError(multipleIdentification);
                        return false;
                    }
                }
                receivedNum++;
            }
        }
	}

	while (announcementBuffer.getMessageNum() == 0){
        recvMessages();
        SoftwareTimerPool::busyWaitUs(1000);
        retryNum++;
        if (retryNum > 1000000){
            setError(missingReply);
            return false;
        }
    }

    packet = announcementBuffer.getMessage();
    //Received own packet - set replying and return true
	if (packet.function == announceFun &&
		packet.payload.announcement.type == topologyInitAnn && 
        packet.payload.announcement.payload == ((neighbourNum << 3) | myId)){
            replying = requireReply;
			return true;
		}
	setError(ringNotClosed);
	return false;
}

uint8_t Messenger::findNeighbour(bool direction, uint8_t* id){
    uint8_t iterator;

    //Find the next active neighbour forwards
    if (direction == true){
        iterator = 0;
        while (iterator < neighbourNum && neighbourActive[iterator] == false){
            ++iterator;
        }
        if (iterator == neighbourNum){
            return MAX_NEIGHBOURS;
        }
    }

    //Find the next active neighbour backwards
    else {
        iterator = neighbourNum - 1;
        while (iterator > 0 && neighbourActive[iterator] == false){
            --iterator;
        }
        if (iterator == 0 && neighbourActive[iterator] == false){
            return MAX_NEIGHBOURS;
        }
    }

    if (id != nullptr){
        *id = neighbours[iterator];
    }
    return iterator;
}

void Messenger::disableNeighbour(uint8_t neighbourId){
    for (uint8_t i = 0; i < neighbourNum; ++i){
        if (neighbours[i] == neighbourId){
            neighbourActive[i] = false;
        }
    }
}

bool Messenger::sendToNeighbour(Packet& packet, bool direction){
    packet.direction = direction;
    packet.distance = findNeighbour(direction);
    if (packet.distance == MAX_NEIGHBOURS){
        return false;
    }
    return sendMessage(packet);
}

bool Messenger::sendAnnouncement(Packet& packet){
    packet.distance = neighbourNum - 1;
    return sendMessage(packet);
}

bool Messenger::sendProjectile(WeaponType projectile, uint8_t position){
    Packet p = {0};
    bool direction = position < HALF_SCREEN_LENGTH;
    if (direction == false){
        position -= HALF_SCREEN_LENGTH;
    }

    p.function = shootProjectileFun;
    p.payload.projectile.type = projectile;
    p.payload.projectile.position = position;
    return sendToNeighbour(p, direction);
}

bool Messenger::sendHP(uint8_t hp, uint8_t position){
    Packet p = {0};
    p.function = tellHPFun;
    p.payload.hp.value = hp;

    //Anounce death
    if (hp == 0){
        p.function = announceFun;
        p.payload.announcement.type = deathAnn;
        uint8_t id = 0;
        findNeighbour(true, &id);
        p.payload.announcement.payload = (id << 3) | myId;
        return sendAnnouncement(p);
    }

    p.payload.hp.youHitMe = position < HALF_SCREEN_LENGTH;
    if (sendToNeighbour(p, true) == false){
        return false;
    }
    
    SoftwareTimerPool::busyWaitUs(1000);
    p.payload.hp.youHitMe = !p.payload.hp.youHitMe;
    return sendToNeighbour(p, false);
}

uint8_t Messenger::getPacket(packetPayload& payload, bool& direction){
    if (messageBuffer.getMessageNum() == 0){
        return 0;
    }
    Packet newPacket = messageBuffer.getMessage();
    payload = newPacket.payload;
    return newPacket.function;
}

bool Messenger::getAnnouncement(packetPayload& payload){
    if (announcementBuffer.getMessageNum() == 0){
        return false;
    }
    Packet newPacket = announcementBuffer.getMessage();
    payload = newPacket.payload;
    return true;
}

void Messenger::commLoop(){
    recvMessages();
}