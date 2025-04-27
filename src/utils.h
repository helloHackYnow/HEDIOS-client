//
// Created by victor on 21/04/25.
//

#ifndef UTILS_H
#define UTILS_H


// Expect buffer_size >= width + 1
inline void fmt_binary(char *buffer, size_t buffer_size, unsigned long value, size_t width=8) {
    for (int i = 0; i < width; i++) {
        buffer[i] = (value & (1 << width-i-1)) ? '1' : '0';
    }
    buffer[width] = '\0';
}

#endif //UTILS_H
