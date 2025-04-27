//
// Created by victor on 20/04/25.
//

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "USB/UsbUtils.h"

void listen_loop(FT_HANDLE ftHandle) {
    while (true) {
        DWORD rxBytes = 0;
        FT_STATUS status = FT_GetQueueStatus(ftHandle, &rxBytes);

        if (status == FT_OK && rxBytes > 0) {
            std::vector<char> buffer(rxBytes);
            DWORD bytesRead = 0;
            status = FT_Read(ftHandle, buffer.data(), rxBytes, &bytesRead);

            if (status == FT_OK && bytesRead > 0) {
                std::string received(buffer.begin(), buffer.begin() + bytesRead);
                std::cout << "Received: " << received << std::endl;
            }
        }

        // Sleep a little to avoid busy looping
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main() {

    FT_HANDLE ftHandle;
    FT_STATUS status = FT_Open(1, &ftHandle);

    UsbUtils::connect_uart_to_device(ftHandle, 1000000);

    std::thread listener(listen_loop, ftHandle);
    listener.join();

    return 0;
}