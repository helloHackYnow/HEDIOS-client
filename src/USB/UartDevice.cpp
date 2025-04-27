//
// Created by victor on 20/04/25.
//

#include "UartDevice.h"

#include <iostream>
#include <bits/ostream.tcc>

#include "UsbUtils.h"
#include "../utils.h"

UartDevice::UartDevice() = default;


UartDevice::UartDevice(FT_DEVICE_LIST_INFO_NODE info){
    this->info = info;
    this->is_open = info.ftHandle != NULL;
}

UartDevice::UartDevice(UartDevice &&other) {
    this->info = other.info;
    this->is_open = other.is_open;


    this->uart_info.is_connected = other.uart_info.is_connected;
    this->uart_info.baud_rate = other.uart_info.baud_rate;
    this->uart_info.r_timeout = other.uart_info.r_timeout;
    this->uart_info.w_timeout = other.uart_info.w_timeout;
}


void UartDevice::open() {
    if (is_open) {
        std::cout << "Device " << this->info.Description << " already open" << std::endl;
        return;
    }

    FT_STATUS status = FT_OpenEx(reinterpret_cast<PVOID>(this->info.LocId), FT_OPEN_BY_LOCATION, &this->info.ftHandle );
    if (status != FT_OK) {
        std::cout << "Error opening device " << this->info.Description << std::endl;
        return;
    }

    this->is_open = true;
}

void UartDevice::close() {
    if (!is_open) {
        std::cout << "Device " << this->info.Description << " already closed" << std::endl;
        return;
    }

    FT_STATUS status = FT_Close(this->info.ftHandle);
    if (status != FT_OK) {
        std::cout << "Error closing device " << this->info.Description << std::endl;
        return;
    }

    this->is_open = false;
    this->uart_info.is_connected = false;
    this->info.ftHandle = NULL;
    this->uart_info.rxQueue = 0;
}

void UartDevice::connect_uart(int baud_rate) {
    if (!is_open) {
        std::cout << "Device " << this->info.Description << " not open" << std::endl;
        return;
    }

    UsbUtils::connect_uart_to_device(this->info.ftHandle, baud_rate);

    this->uart_info.is_connected = true;
    this->uart_info.baud_rate = baud_rate;
}

void UartDevice::set_timeout(int r_timeout, int w_timeout) {
    if (!is_open) {
        std::cout << "Device " << this->info.Description << " not open" << std::endl;
        return;
    }

    FT_STATUS status = FT_SetTimeouts(this->info.ftHandle, r_timeout, w_timeout);

    if (status != FT_OK) {
        std::cout << "Error setting timeouts for device " << info.Description << std::endl;
        return;
    }

    this->uart_info.r_timeout = r_timeout;
    this->uart_info.w_timeout = w_timeout;
}

std::vector<char> UartDevice::read_uart(const int bytes) const {
    if (!is_open) {
        std::cout << "Device " << this->info.Description << " not open" << std::endl;
        return {};
    }

    if (!uart_info.is_connected) {
        std::cout << "Device " << this->info.Description << " not connected" << std::endl;
        return {};
    }

    std::vector<char> buf(bytes);
    DWORD bytesRead;
    FT_STATUS status = FT_Read( info.ftHandle, buf.data(), bytes, &bytesRead);

    if (status != FT_OK) {
        std::cout << "Error reading from device " << info.Description << std::endl;
        return {};
    }

    return buf;
}

void UartDevice::write_uart(std::vector<char> data) const {
    if (!is_open) {
        std::cout << "Device " << this->info.Description << " not open" << std::endl;
        return;
    }

    if (!uart_info.is_connected) {
        std::cout << "Device " << this->info.Description << " not connected" << std::endl;
        return;
    }

    DWORD bytesWritten;
    FT_STATUS status;
    for (auto ele: data) {
        status = FT_Write(info.ftHandle, &ele, 1, &bytesWritten);
    }

    char buffer[32];

    //std::cout << "Writing to device " << info.Description << std::endl;

    for (auto& byte : data) {
        fmt_binary(buffer, 32, byte, 8);
        //std::cout << std::hex << buffer << std::endl;
    }

    if (status != FT_OK) {
        std::cout << "Error writing to device " << info.Description << std::endl;
        return;
    }
}

void UartDevice::update_rxQueue() {
    if (!is_open) {
        std::cout << "Device " << this->info.Description << " not open" << std::endl;
        return;
    }

    if (!uart_info.is_connected) {
        std::cout << "Device " << this->info.Description << " not connected" << std::endl;
        return;
    }

    FT_STATUS status = FT_GetQueueStatus(info.ftHandle, (DWORD*)&this->uart_info.rxQueue);
    if (status != FT_OK) {
        std::cout << "Error getting queue status for device " << info.Description << std::endl;
    }
}

size_t UartDevice::rxQueue() const {
    return this->uart_info.rxQueue;
}







