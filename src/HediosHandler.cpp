//
// Created by victor on 21/04/25.
//

#include "HediosHandler.h"

#include <cstring>
#include <iostream>
#include <bits/ostream.tcc>

#include "utils.h"
#include "USB/UsbUtils.h"

HediosCommandFD parse_command(uint8_t command) {
    if (command >= HDC_UPDATE_VALUE) {
        return HDC_UPDATE_VALUE;
    }

    if (command > HDC_MAX_VALUE) {
        return HDC_UNKNOWN_COMMAND;
    }

    return (HediosCommandFD)command;
}

void print_packet(HediosPacket packet) {
    char buffer[9];
    fmt_binary(buffer, 9, packet.command, 8);
    std::cout << "Command: " << buffer << std::endl;
    std::cout << "Data: " << std::endl;
    for (int i = 0; i < 4; i++) {
        fmt_binary(buffer, 9, packet.data[i], 8);
        std::cout << "  - " << i << ": " << buffer << std::endl;
    }
}

HediosHandler::HediosHandler() = default;

HediosDeviceState::HediosDeviceState(const HediosDeviceState & other) {
    device_idx = other.device_idx;

    uart_device.info = other.uart_device.info;

}




bool is_allowed_hd_device(const char* description) {
    for (int i = 0; i < ALLOWED_HD_DEVICE_COUNT; i++) {
        if (strcmp(description, ALLOWED_HD_DEVICE_DESCRIPTION[i]) == 0) return true;
    }

    return false;
}

void fmt_value(char *buffer, size_t buffer_size, HediosValueFormat format, unsigned long value) {
    switch (format) {
        case HDVF_8_BIT_BIN:
            fmt_binary(buffer, buffer_size, value, 8);
            break;
        case HDVF_16_BIT_BIN:
            fmt_binary(buffer, buffer_size, value, 16);
            break;
        case HDVF_32_BIT_BIN:
            fmt_binary(buffer, buffer_size, value, 32);
            break;
        case HDVF_UINT_8_HEX:
            sprintf(buffer, "%x", (u_int8_t)value);
            break;
        case HDVF_UINT_16_HEX:
            sprintf(buffer, "%x", (u_int16_t)value);
            break;
        case HDVF_UINT_32_HEX:
            sprintf(buffer, "%x", (u_int32_t)value);
            break;
        case HDVF_UINT_8_DEC:
            sprintf(buffer, "%d", (u_int8_t)value);
            break;
        case HDVF_UINT_16_DEC:
            sprintf(buffer, "%d", (u_int16_t)value);
            break;
        case HDVF_UINT_32_DEC:
            sprintf(buffer, "%d", (u_int32_t)value);
            break;
        case HDVF_STR:
            sprintf(buffer, "%s", (char*)value);
            break;

    }
}

void HediosHandler::find_all_hedios_devices() {

    auto dv_infos = UsbUtils::get_devices_info();

    for (auto& dv_info : dv_infos) {
        if (is_allowed_hd_device(dv_info.Description)) {

            // Check for the presence of the device in the pool
            bool is_already_in_pool = false;
            for (auto& device : state.device_pool) {
                if (device.uart_device.info.Description == dv_info.Description &&
                    device.uart_device.info.ftHandle == dv_info.ftHandle &&
                    device.uart_device.info.LocId == dv_info.LocId) {
                    is_already_in_pool = true;
                }
            }

            // If not in the pool, add it
            if (!is_already_in_pool) {
                HediosDeviceState device;
                auto& uart = device.uart_device;
                uart.info = dv_info;
                device.device_idx = state.device_pool.size();
                state.device_pool.push_back(device);
            }
        }
    }
}

void HediosHandler::connect_device(int device_idx) {
    if (device_idx < 0 || device_idx >= state.device_pool.size()) return;
    auto& device = state.device_pool[device_idx];

    if (!device.uart_device.is_open) {
        device.uart_device.open();
    }

    device.uart_device.connect_uart(1000000);
}

bool HediosHandler::is_valid_device_idx(const int device_idx) const {

    if (device_idx < 0 || device_idx >= state.device_pool.size()) return false;
    return true;
}

void HediosHandler::process_all_devices() {
    int idx = 0;
    for (auto & device: state.device_pool) {
        if (device.uart_device.is_open) {
            process_input_queue(idx, 0);
        }

        idx++;
    }
}

void HediosHandler::process_input_queue(size_t device_idx, size_t packet_count) {

    if (!is_valid_device_idx(device_idx)) return;
    auto& device = state.device_pool[device_idx];
    auto& uart = device.uart_device;

    if (!uart.is_open) return;

    size_t rxQueue = uart.rxQueue();
    size_t possible_packets = rxQueue/5;
    size_t packets_to_process = packet_count == 0 ? possible_packets: std::min(possible_packets, packet_count) ;

    if (packets_to_process == 0) return;

    auto buf = device.uart_device.read_uart(packets_to_process * 5);

    for (size_t i = 0; i < packets_to_process; i++) {
        HediosPacket packet;
        packet.command = buf.data()[i*5];
        char buffer[32];
        fmt_binary(buffer, 32, packet.command, 8);
        // std::cout << "Command: " << buffer << std::endl;

        memcpy(&packet.data, buf.data() + i*5 + 1, 4);
        process_packet(packet, device_idx);
    }
}


void HediosHandler::process_packet(HediosPacket packet, size_t device_idx) {
    auto command = parse_command(packet.command);
    auto& device = state.device_pool[device_idx];

    switch (command) {
        case HDC_UPDATE_VALUE:

            if (is_valid_device_idx(device_idx)) {

                u_int8_t slot_idx = packet.command - HDC_UPDATE_VALUE;
                if (slot_idx < device.used_slot) {
                    device.slot_value[slot_idx] = 0;

                    for (int i = 0; i < 4; i++) {
                        device.slot_value[slot_idx] |= packet.data[i] << 8*i;
                    }
                }
            }
            break;

        case HDC_PONG:
            std::cout << "Pong" << std::endl;
            break;

        case HDC_SLOT_COUNT:
            std::cout << "Slot count of device " << device.uart_device.info.Description << " : " << (size_t)packet.data[0] << std::endl;
            device.detected_slot_count = packet.data[0];

            while (device.used_slot < device.detected_slot_count) {
                sprintf(device.slot_label[device.used_slot], "Slot %d", device.used_slot);
                device.slot_format[device.used_slot] = HDVF_32_BIT_BIN;
                device.slot_value[device.used_slot] = 0;
                device.used_slot++;
            }
            break;

        case HDC_UNKNOWN_COMMAND:
            std::cout << "The device did not recognise the command" << std::endl;
            break;

        case HDC_ACTION_COUNT: {

            print_packet(packet);

            int var_action_count = packet.data[0];
            int varless_action_count = packet.data[1];

            std::cout << "Action count of device " << device.uart_device.info.Description << " : " << std::endl;
            std::cout << "  - Var actions       : " << var_action_count << std::endl;
            std::cout << "  - Varless actions   : " << varless_action_count << std::endl;

            device.var_action_count = var_action_count;
            device.varless_action_count = varless_action_count;
            break; }

        default:
            std::cout << "Unsupported or unknown command" << std::endl;
            char buffer[9];
            fmt_binary(buffer, 9, packet.command, 8);
            std::cout << "  - Command: " << buffer << std::endl;
            std::cout << "  - Data: " << std::endl;
            for (int i = 0; i < 4; i++) {
                fmt_binary(buffer, 9, packet.data[i], 8);
                std::cout << "   " << i <<": " << buffer << std::endl;
            }
            break;
    }
}


void HediosHandler::send_packet(HediosPacket packet, size_t device_idx) const {
    auto& device = state.device_pool[device_idx];

    if (!device.uart_device.is_open) return;
    if (!device.uart_device.uart_info.is_connected) return;

    device.uart_device.write_uart({
        (char) packet.command, (char) packet.data[0], (char) packet.data[1], (char) packet.data[2],
        (char) packet.data[3]
    });
}

void HediosHandler::send_reset(size_t device_idx) {
    auto& device = state.device_pool[device_idx];

    if (!device.uart_device.is_open) return;
    if (!device.uart_device.uart_info.is_connected) return;

    device.uart_device.write_uart({static_cast<char>(HTD_RESET), 0, 0, 0, 0});
}

void HediosHandler::send_ping(size_t device_idx) {
    auto& device = state.device_pool[device_idx];
    if (!device.uart_device.is_open) return;
    if (!device.uart_device.uart_info.is_connected) return;

    device.uart_device.write_uart({static_cast<char>(HTD_PING), 0, 0, 0, 0});
}

void HediosHandler::detect_slot_count(size_t device_idx) {
    auto& device = state.device_pool[device_idx];
    if (!device.uart_device.is_open) return;
    if (!device.uart_device.uart_info.is_connected) return;

    device.uart_device.write_uart({static_cast<char>(HTD_ASK_SLOT_COUNT), 0, 0, 0, 0});
}

void HediosHandler::ask_all_slot_update(size_t device_idx) {
    auto& device = state.device_pool[device_idx];
    if (!device.uart_device.is_open) return;
    if (!device.uart_device.uart_info.is_connected) return;

    device.uart_device.write_uart({static_cast<char>(HTD_UPDATE_ALL_SLOTS), 0, 0, 0, 0});
}

void HediosHandler::ask_specific_slot_update(size_t device_idx, int slot_idx) {
    auto& device = state.device_pool[device_idx];
    if (!device.uart_device.is_open) return;
    if (!device.uart_device.uart_info.is_connected) return;

    device.uart_device.write_uart({static_cast<char>(HTD_UPDATE_SLOT), (char)slot_idx, 0, 0, 0});
}


void HediosHandler::detect_action_count(size_t device_idx) {
    auto& device = state.device_pool[device_idx];
    if (!device.uart_device.is_open) return;
    if (!device.uart_device.uart_info.is_connected) return;

    device.uart_device.write_uart({static_cast<char>(HTD_ASK_ACTION_COUNT), 0, 0, 0, 0});
}


