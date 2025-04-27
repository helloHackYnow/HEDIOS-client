//
// Created by victor on 20/04/25.
//

#ifndef USBUTILS_H
#define USBUTILS_H

#include "../../FTDI/ftd2xx.h"
#include <vector>

namespace UsbUtils {
    std::vector<FT_DEVICE_LIST_INFO_NODE> get_devices_info();
    void connect_uart_to_device(FT_HANDLE handle, int baud_rate);
    void disconnect_uart_from_device(FT_HANDLE handle);

    std::vector<char> read_from_device(FT_HANDLE handle);

}
#endif //USBUTILS_H
