#include "UsbUtils.h"

#include <iostream>
#include <bits/ostream.tcc>


std::vector<FT_DEVICE_LIST_INFO_NODE> UsbUtils::get_devices_info() {
    DWORD device_count;

    FT_STATUS ft_status = FT_CreateDeviceInfoList(&device_count);

    if (ft_status != FT_OK) {
        return {};
    }

    auto* device_list = new FT_DEVICE_LIST_INFO_NODE[device_count];
    ft_status = FT_GetDeviceInfoList(device_list, &device_count);

    if (ft_status != FT_OK) {
        return {};
    }

    std::vector<FT_DEVICE_LIST_INFO_NODE> devices;
    devices.reserve(device_count);

    for (int i = 0; i < device_count; i++) {
        devices.push_back(device_list[i]);
    }

    delete[] device_list;

    return devices;
}

void UsbUtils::connect_uart_to_device(FT_HANDLE handle, int baud_rate) {
    FT_STATUS status;
    status = FT_SetBaudRate(handle, baud_rate);
    if (status != FT_OK) {
        std::cout << "Error setting baud rate" << std::endl;
        return;
    }
    status = FT_SetDataCharacteristics(handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    if (status != FT_OK) {
        std::cout << "Error setting data characteristics" << std::endl;
    }

}

void UsbUtils::disconnect_uart_from_device(FT_HANDLE handle) {
    FT_Close(handle);
}

std::vector<char> UsbUtils::read_from_device(FT_HANDLE handle) {
    DWORD rxQueue;
    FT_GetQueueStatus(handle, &rxQueue);
    if (rxQueue > 0) {
        // 2) Read them all
        std::vector<char> buf(rxQueue);
        DWORD bytesRead;
        FT_Read(
            handle,
            buf.data(),
            rxQueue,
            &bytesRead
        );

        return buf;
    }

    return {};
}


