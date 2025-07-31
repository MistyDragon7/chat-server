#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

std::mutex cout_mutex;

void say_hello(int id)
{
    for (int i = 0; i < 50; ++i)
    {
        {
            std::lock_guard<std::mutex> guard(cout_mutex);
            std::cout << "Thread " << id << ": " << i << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main()
{
    std::thread t1(say_hello, 1);
    std::thread t2(say_hello, 2);
    t1.join();
    t2.join();
}
