#pragma once

#include <cstdint>

#include "symtab.hh"
#include "types.hh"

namespace Tailslide {

const int32_t TOTAL_LSO_MEMORY = 0x4000;
const int32_t LSO_VERSION_NUM = 0x0200;

enum LSORegisters {
  LREG_TM,  // top of memory
  LREG_IP,  // instruction pointer
  LREG_VN,  // version number
  LREG_BP,  // base pointer - what local variables are referenced from
  LREG_SP,  // stack pointer - where the top of the stack is
  LREG_HR,  // heap register - where in memory does the heap start
  LREG_HP,  // heap pointer - where is the top of the heap?
  LREG_CS,  // current state
  LREG_NS,  // next state - what state are we switching to?
  // CE IE and ER are NOT used in version 2 bytecode.
  // repopulated whenever state is switched as they related to the _state_.
  LREG_CE,  // current events - what events are waiting to be handled?
  LREG_IE,  // in event - which event handler are we currently in?
  LREG_ER,  // event register - what events do we have active handlers for?
  LREG_FR,  // fault register - which errors are currently active?
  LREG_SLR,  // sleep register - are we sleeping? For how long?
  LREG_GVR,  // global variable register - where do global variables start
  LREG_GFR,  // global function register - where do global functions start
  LREG_PR,  // parameter register - data passed to script from launcher
  LREG_ESR,  // energy supply register - how much energy do we have on board?
  LREG_SR,  // state register - where do state details start

  // 64-bit registers, only exist in version 2 bytecode
  LREG_NCE,  // 64 bit current events - what events are waiting to be handled?
  LREG_NIE,  // 64 bit in event - which event handler are we currently in?
  LREG_NER,  // 64 bit event register - what events do we have active handlers for?
  LREG_MAX
};

const int32_t LSORegisterOffsets[LREG_MAX + 1] = {
    0,  // TM
    4,  // IP
    8,  // VN
    12,  // BP
    16,  // SP
    20,  // HR
    24,  // HP
    28,  // CS
    32,  // NS
    36,  // CE
    40,  // IE
    44,  // ER
    48,  // FR
    52,  // SLR
    56,  // GVR
    60,  // GFR
    64,  // PR
    68,  // ESR
    72,  // SR
    76,  // NCE
    84,  // NIE
    92,  // NER
    100, // end of registers
};

typedef enum {
  LSOH_STATE_ENTRY = 0,
  LSOH_STATE_EXIT = 1,
  LSOH_TOUCH_START = 2,
  LSOH_TOUCH = 3,
  LSOH_TOUCH_END = 4,
  LSOH_COLLISION_START = 5,
  LSOH_COLLISION = 6,
  LSOH_COLLISION_END = 7,
  LSOH_LAND_COLLISION_START = 8,
  LSOH_LAND_COLLISION = 9,
  LSOH_LAND_COLLISION_END = 10,
  LSOH_TIMER = 11,
  LSOH_LISTEN = 12,
  LSOH_REZ = 13,
  LSOH_SENSOR = 14,
  LSOH_NO_SENSOR = 15,
  LSOH_CONTROL = 16,
  LSOH_MONEY = 17,
  LSOH_EMAIL = 18,
  LSOH_AT_TARGET = 19,
  LSOH_NOT_AT_TARGET = 20,
  LSOH_AT_ROT_TARGET = 21,
  LSOH_NOT_AT_ROT_TARGET = 22,
  LSOH_RTPERMISSIONS = 23,
  LSOH_INVENTORY = 24,
  LSOH_ATTACH = 25,
  LSOH_DATASERVER = 26,
  LSOH_LINK_MESSAGE = 27,
  LSOH_MOVING_START = 28,
  LSOH_MOVING_END = 29,
  LSOH_OBJECT_REZ = 30,
  LSOH_REMOTE_DATA = 31,
  LSOH_HTTP_RESPONSE = 32,
  LSOH_HTTP_REQUEST = 33,
  LSOH_MAX = 34,
} LSOHandlerType;

const char * const LSOHandlerNames[LSOH_MAX] = {
    "state_entry",  // LSOH_STATE_ENTRY
    "state_exit",  // LSOH_STATE_EXIT
    "touch_start",  // LSOH_TOUCH_START
    "touch",  // LSOH_TOUCH
    "touch_end",  // LSOH_TOUCH_END
    "collision_start",  // LSOH_COLLISION_START
    "collision",  // LSOH_COLLISION
    "collision_end",  // LSOH_COLLISION_END
    "land_collision_start",  // LSOH_LAND_COLLISION_START
    "land_collision",  // LSOH_LAND_COLLISION
    "land_collision_end",  // LSOH_LAND_COLLISION_END
    "timer",  // LSOH_TIMER
    "listen",  // LSOH_LISTEN
    "rez",  // LSOH_REZ
    "sensor",  // LSOH_SENSOR
    "no_sensor",  // LSOH_NO_SENSOR
    "control",  // LSOH_CONTROL
    "money",  // LSOH_MONEY
    "email",  // LSOH_EMAIL
    "at_target",  // LSOH_AT_TARGET
    "not_at_target",  // LSOH_NOT_AT_TARGET
    "at_rot_target",  // LSOH_AT_ROT_TARGET
    "not_at_rot_target",  // LSOH_NOT_AT_ROT_TARGET
    "runtime_permissions",  // LSOH_RTPERMISSIONS
    "inventory",  // LSOH_INVENTORY
    "attach",  // LSOH_ATTACH
    "dataserver",  // LSOH_DATASERVER
    "link_message",  // LSOH_LINK_MESSAGE
    "moving_start",  // LSOH_MOVING_START
    "moving_end",  // LSOH_MOVING_END
    "object_rez",  // LSOH_OBJECT_REZ
    "remote_data",  // LSOH_REMOTE_DATA
    "http_response",  // LSOH_HTTP_RESPONSE
    "http_request",  // LSOH_HTTP_REQUEST
};

const uint32_t LSOTypeDataSize[LST_MAX] = {
    0,  // null
    4,  // int
    4,  // float
    4,  // string (just a pointer)
    4,  // key (just a pointer)
    12, // vector
    16, // quaternion
    4,  // list (just a pointer)
    0,  // error!
};

/// pack two types into a single byte (common for LSO)
inline uint8_t pack_lso_types(LSLIType left, LSLIType right) {
  return right | (left << 4);
}

}