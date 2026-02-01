
// g++ -O3 cs16zmqPush.cpp -o cs16zmqPush.exe -lzmq
// ./ca16zmqPush.exe

#include <zmq.hpp>
#include <vector>
#include <random>
#include <cstdint>
#include <thread>
#include <chrono>

int main() {
    // 1. ZMQ Setup
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PUSH);
    socket.bind("tcp://*:5555"); // Bind for GNURadio to connect

    // 2. Setup Random Generator (for s16 range: -32768 to 32767)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int16_t> dis(-32768, 32767);

    // 3. Buffer setup (e.g., 1024 samples, 2 bytes each)
    const int num_samples = 1024;
    std::vector<int16_t> buffer(num_samples);

    printf("Starting random S16 source on port 5555...\n");

    while (true) {
        // Fill buffer with random data
        for (int i = 0; i < num_samples; ++i) {
            buffer[i] = dis(gen);
        }

        // 4. Send the raw binary data
        zmq::message_t message(buffer.size() * sizeof(int16_t));
        memcpy(message.data(), buffer.data(), buffer.size() * sizeof(int16_t));
        socket.send(message, zmq::send_flags::none);

        // Optional: Small sleep to simulate rate
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
