//
// Created by victor on 21/04/25.
//

#ifndef HEDIOSHANDLER_H
#define HEDIOSHANDLER_H
#include <cstdint>
#include <vector>
#include "USB/UartDevice.h"


/*
 * The command is the first byte sent by the device. This command is followed by 4 bytes of data.
 * Each HediosPacket is 40 bits.
 * There are 256 possible commands (8 bits).
 * 128 of them are allocated for the device to send data to the client.
 *
 * How is the command byte processed:
 *      - If MSB is 1: HD_UPDATE_VALUE, the 7 remaining bits are the ID of the updated value.
 *      - The remaining commands are parsed
 */

inline char ALLOWED_HD_DEVICE_DESCRIPTION[16][64] = {
    {"Alchitry Au V2 B"},
};

inline int ALLOWED_HD_DEVICE_COUNT = 1;

bool is_allowed_hd_device(const char* description);


// Identifier for command coming from the device
enum HediosCommandFD {

    HDC_UNKNOWN, // The command received was corrupted / the device sent an invalid command
    HDC_PING = 1, // 0b00000001 The endpoint is pinging the client (TODO)
    HDC_DONE = 2, // 0b00000010 Signal that the device is ready to accept another command ( TODO )
    HDC_PONG = 3, // 0b00000011 Standard response if a ping was sent
    HDC_LOG = 4,  // 0b00000100 The next byte (LSB / data[0]) contains the id of the log string ( TODO : implement the log logic )
    HDC_SLOT_COUNT = 5, // 0b00000101 The next byte (LSB / data[0]) contains the number of slots (max 128)
    HDC_ACTION_COUNT = 6, // 0b00000110 data[0] contains VAR_ACTION_COUNT, data[1] contains VARLESS_ACTION_COUNT
    // (TODO : fill this gap)
    HDC_ERROR = 8,// 0b00001000 The next byte (LSB / data[0]) contains the id of the error string ( TODO )
    HDC_INVALID_SLOT = 9,// 0b00001001 Endpoint's response if the update slot request was invalid
    HDC_INVALID_ACTION = 10,// 0b00001010 Endpoint's response if the configurable action had an invalid code
    HDC_UNKNOWN_COMMAND = 11,// 0b00001100 Endpoint's response if the command sent by the client was unknown
    HDC_UPDATE_VALUE = 1<<7, // 128 32-bit value slot (0b1xxxxxxx)

};

inline constexpr size_t HDC_MAX_VALUE = HDC_UNKNOWN_COMMAND;

// Identifier for command addressed to the device
enum HediosCommandTD {
    HTD_PING = 1, // 0b00000001
    HTD_UPDATE_SLOT = 2, // 0b00000010
    HTD_UPDATE_ALL_SLOTS = 3, // 0b00000011
    HTD_ASK_SLOT_COUNT = 4, // 0b00000100
    HTD_ASK_ACTION_COUNT = 5, // 0b00000101 The first data byte of the awnser if for the VAR_ACTION_COUNT, the 2nd for the VARLESS_ACTION_COUNT
    HTD_SEND_ACTION = 1<<7, // 0b1xxxxxxx the seven LSB indicate the id of the action
    HTD_RESET = 0b01010101
};



enum HediosValueFormat {
    HDVF_UINT_8_HEX,
    HDVF_UINT_16_HEX,
    HDVF_UINT_32_HEX,

    HDVF_UINT_8_DEC,
    HDVF_UINT_16_DEC,
    HDVF_UINT_32_DEC,

    HDVF_8_BIT_BIN,
    HDVF_16_BIT_BIN,
    HDVF_32_BIT_BIN,

    HDVF_STR
};

// Expect 33 char buffer for the longest
void fmt_value(char *buffer, size_t buffer_size, HediosValueFormat format, unsigned long value);

struct HediosDeviceState {

    UartDevice uart_device;
    int used_slot = 0;
    int device_idx = -1;
    char slot_label[128][32] = {};
    unsigned long slot_value[128] = {};
    bool is_slot_settable[128] = {};
    int var_action_count = -1;
    int varless_action_count = -1;

    // Filled when processing a HDC_SLOT_COUNT packet
    int detected_slot_count = -1;
    HediosValueFormat slot_format[128] = {};

    HediosDeviceState() = default;
    HediosDeviceState(const HediosDeviceState&);
    HediosDeviceState(HediosDeviceState&&) noexcept = default;
};

/**
 * Represents a data packet structure for the Hedios communication protocol.
 */
struct HediosPacket {
    uint8_t command;
    uint8_t data[4]; // data[0] LSB, data[3] MSB
};

struct HediosHandlerState {
    std::vector<HediosDeviceState> device_pool;

};

HediosCommandFD parse_command(uint8_t command);
void print_packet(HediosPacket packet);

class HediosHandler {
public:
    HediosHandler();

    void find_all_hedios_devices();
    void connect_device(int device_idx);
    bool is_valid_device_idx(int device_idx) const;

    void process_all_devices();

    void process_input_queue(size_t device_idx, size_t packet_count = 0);

    void process_packet(HediosPacket packet, size_t device_idx = -1);

    void detect_slot_count(size_t device_idx);
    void detect_action_count(size_t device_idx);
    void ask_all_slot_update(size_t device_idx);
    void ask_specific_slot_update(size_t device_idx, int slot_idx);

    void send_packet(HediosPacket packet, size_t device_idx) const;
    void send_reset(size_t device_idx);
    void send_ping(size_t device_idx);
public: // Data
    HediosHandlerState state;
};





#endif //HEDIOSHANDLER_H
