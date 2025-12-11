#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

template <typename T, size_t Size>
class RingBuffer {
private:
    T buffer[Size];
    volatile size_t head = 0;
    volatile size_t tail = 0;
    portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;

    bool isFull() {
        return (head + 1) % Size == tail;
    }

    bool isEmpty() {
        return head == tail;
    }

public:
    bool write(T item) {
        portENTER_CRITICAL(&mutex);
        if (isFull()) {
            portEXIT_CRITICAL(&mutex);
            return false;
        }
        buffer[head] = item;
        head = (head + 1) % Size;
        portEXIT_CRITICAL(&mutex);
        return true;
    }

    bool read(T* item) {
        portENTER_CRITICAL(&mutex);
        if (isEmpty()) {
            portEXIT_CRITICAL(&mutex);
            return false;
        }
        *item = buffer[tail];
        tail = (tail + 1) % Size;
        portEXIT_CRITICAL(&mutex);
        return true;
    }

    size_t available() {
        portENTER_CRITICAL(&mutex);
        size_t count = (head >= tail) ? head - tail : Size - tail + head;
        portEXIT_CRITICAL(&mutex);
        return count;
    }

    void clear() {
        portENTER_CRITICAL(&mutex);
        head = tail = 0;
        portEXIT_CRITICAL(&mutex);
    }
};
