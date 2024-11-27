#include <iostream>
#include <queue>
#include <thread>
#include <semaphore.h>
#include <chrono>
#include <random>

std::queue<int> buffer;
sem_t emptySlots;
sem_t fullSlots;
sem_t mutex;

void producer(int jobsToProduce) {
    for (int i = 1; i <= jobsToProduce; ++i) {
        auto start_time = std::chrono::steady_clock::now();  // track waiting time

        while (true) {
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_time = current_time - start_time;

            if (elapsed_time.count() > 10) {
                std::cout << "Producer waited too long (10 seconds), quitting." << std::endl;
                return;  // exit producer if waiting for 10 seconds
            }

            if (sem_trywait(&emptySlots) == 0) {
	        int job = rand() % 10 + 1; // generate random number as job time
		sem_wait(&mutex);  // mutual exclusion check
                buffer.push(job);  // send to buffer
                std::cout << "Produced: " << job << std::endl;

                // 
                sem_post(&fullSlots); // add 1 to the semephore of existing jobs
		sem_post(&mutex);
                break;
            }
        }

    }
}

void consumer() {
    while (true) {
        auto start_time = std::chrono::steady_clock::now();  // track waiting time

        while (true) {
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_time = current_time - start_time;

            if (elapsed_time.count() > 10) {
                std::cout << "Consumer waited too long (10 seconds), quitting." << std::endl;
                return;  // exit consumer if waiting for 10 seconds
            }

            if (sem_trywait(&fullSlots) == 0) {
	        sem_wait(&mutex); // mutual exclusion check
                int data = buffer.front();
                buffer.pop(); // consume job
                std::cout << "Consumed: " << data << std::endl;

                sem_post(&emptySlots); // add 1 to the semephore of empty slots
		sem_post(&mutex);
                // simulate job consumption time
                std::this_thread::sleep_for(std::chrono::seconds(data));
                break;
            }
        }

    }
}

int main() {
    int queueSize;
    int jobsPerProducer;
    int numProducers;
    int numConsumers;
    std::cout << "Enter the size of the queue: ";
    std::cin >> queueSize;
    std::cout << "Enter the number of jobs to generate for each producer: ";
    std::cin >> jobsPerProducer;
    std::cout << "Enter the number of producers: ";
    std::cin >> numProducers;
    std::cout << "Enter the number of consumers: ";
    std::cin >> numConsumers;

    // check input errors and semaphore initial initialization error
    if (queueSize <= 0 || jobsPerProducer <= 0 || numProducers <= 0 || numConsumers <= 0) { std::cerr << "All input values must be positive integers." << std::endl; return 1; }
    if (sem_init(&emptySlots, 0, queueSize) != 0 || sem_init(&fullSlots, 0, 0) != 0 || sem_init(&mutex, 0, 1) != 0) { std::cerr << "Semaphore initialization failed." << std::endl; return 1; }
    // create producer and consumer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < numProducers; ++i) {
        producers.push_back(std::thread(producer, jobsPerProducer));
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < numConsumers; ++i) {
        consumers.push_back(std::thread(consumer));
    }

    // wait for all producer and consumer threads to finish
    for (auto& p : producers) {
        p.join();
    }

    for (auto& c : consumers) {
        c.join();
    }

    // clean up semaphores
    sem_destroy(&emptySlots);
    sem_destroy(&fullSlots);

    return 0;
}
