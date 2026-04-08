#include "Datalink.hpp"

class Messenger {

    typedef enum {
        checkTopology,
        shootProjectile,
        announceHP,
        reserved
    } functionCode;
    
        typedef union {
        uint8_t rawData;
        struct {
            uint8_t shipType    :3;
            uint8_t reserved    :5;
        };
    } topologyPayload;

    typedef union {
        uint8_t rawData;
        struct {
            uint8_t position    :6;
            uint8_t type        :2;
        };
    } projectilePayload;

    typedef union {
        uint8_t rawData;
        struct {
            uint8_t hp          :7;
            uint8_t youHitMe    :1;
        };
    } hpPayload;
};