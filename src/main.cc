#include "buffer.hh"
#include "producer.hh"
#include "consumer.hh"

#include <thread>

int main() {
    RingBuffer queue;

    const uint32_t items = 1000;

    std::thread producer(busy_producer<RingBuffer>, std::ref(queue), items);
    std::thread consumer(busy_consumer<RingBuffer>, std::ref(queue), items);

    producer.join();
    consumer.join();

    return 0;
}