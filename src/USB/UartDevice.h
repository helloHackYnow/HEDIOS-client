//
// Created by victor on 20/04/25.
//

#ifndef DEVICE_H
#define DEVICE_H

#include <vector>
#include <mutex>

#include "../FTDI/ftd2xx.h"


struct UART_info {
    bool is_connected;
    int baud_rate;
    size_t rxQueue = 0;
    int r_timeout = 0;
    int w_timeout = 0;
    std::mutex lock_read;
    std::mutex lock_write;
};

class UartDevice {
public:
    UartDevice();
    explicit UartDevice(FT_DEVICE_LIST_INFO_NODE info);
    UartDevice(UartDevice&& other);

    void open();
    void close();
    void connect_uart(int baud_rate);

    void set_timeout(int r_timeout, int w_timeout);

    std::vector<char> read_uart(int bytes) const;
    void write_uart(std::vector<char> data) const;
    void update_rxQueue();

    size_t rxQueue() const;


public: // Data
    FT_DEVICE_LIST_INFO_NODE info{};
    bool is_open{};
    UART_info uart_info{};

};



#endif //DEVICE_H
