#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iomanip>

class CollatzCalculator {
private:
    const int TOTAL_NUMBERS = 10'000'000;
    std::atomic<long long> totalSteps{0};
    std::atomic<int> numbersProcessed{0};
    std::queue<int> taskQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool finished = false;
    std::chrono::steady_clock::time_point startTime;

    // Обчислення кроків Коллатца для одного числа
    int collatzSteps(long long n) {
        int steps = 0;
        while (n != 1) {
            if (n % 2 == 0) {
                n = n / 2;
            } else {
                n = 3 * n + 1;
            }
            steps++;
        }
        return steps;
    }

public:
    void workerThread(int threadId) {
        while (true) {
            int number = -1;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this] { return !taskQueue.empty() || finished; });
                
                if (finished && taskQueue.empty()) {
                    return;
                }
                
                number = taskQueue.front();
                taskQueue.pop();
            }
            
            int steps = collatzSteps(number);
            totalSteps += steps;
            numbersProcessed++;
            
            // Прогрес (кожні 1% для демонстрації)
            if (numbersProcessed % (TOTAL_NUMBERS / 100) == 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime).count();
                double percent = (double)numbersProcessed / TOTAL_NUMBERS * 100;
                std::cout << "\rПрогрес: " << std::fixed << std::setprecision(1) 
                          << percent << "% | Час: " << elapsed << " мс" << std::flush;
            }
        }
    }

    void run(int numThreads) {
        std::cout << "Гіпотеза Коллатца - обчислення для чисел 1.." << TOTAL_NUMBERS << std::endl;
        std::cout << "Кількість потоків: " << numThreads << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        // Заповнюємо чергу числами
        for (int i = 1; i <= TOTAL_NUMBERS; i++) {
            taskQueue.push(i);
        }
        
        startTime = std::chrono::steady_clock::now();
        
        // Запускаємо потоки
        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back(&CollatzCalculator::workerThread, this, i);
        }
        
        // Чекаємо завершення всіх потоків
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        double averageSteps = (double)totalSteps / numbersProcessed;
        
        std::cout << "\n\nЗагальний час виконання: " << elapsedMs << " мс" << std::endl;
        std::cout << "Всього оброблено чисел: " << numbersProcessed << std::endl;
        std::cout << "Сумарна кількість кроків: " << totalSteps << std::endl;
        std::cout << "Середня кількість кроків: " << std::fixed << std::setprecision(2) << averageSteps << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        // Демонстрація для перших чисел
        std::cout << "\nПеревірка для перших 10 чисел:" << std::endl;
        for (int i = 1; i <= 10; i++) {
            std::cout << "Число " << std::setw(3) << i << " -> " 
                      << std::setw(3) << collatzSteps(i) << " кроків" << std::endl;
        }
    }
};

int main() {
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    
    CollatzCalculator calculator;
    calculator.run(numThreads);
    
    return 0;
}