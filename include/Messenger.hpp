#include "Datalink.hpp"

class Messenger {
    
        union topologyPayload{
        uint8_t rawData;
        struct {
            uint8_t shipType    :3;
            uint8_t reserved    :5;
        };
    };

    union projectilePayload{
        uint8_t rawData;
        struct {
            uint8_t position    :6;
            uint8_t type        :2;
        };
    };

    union hpPayload{
        uint8_t rawData;
        struct {
            uint8_t hp          :7;
            uint8_t youHitMe    :1;
        };
    };
};