#ifndef OC_LAB_CHANNEL_BUFFERED_CHANNEL_H
#define OC_LAB_CHANNEL_BUFFERED_CHANNEL_H

#include <mutex>
#include <condition_variable>
#include <queue>

std::mutex writeLock;
std::mutex readLock;

std::condition_variable writeVar;
std::condition_variable readVar;

template<class T>
class BufferedChannel {
private:
    int size;
    bool canWrite = true;
    bool canRead = true;
    bool isClosed = false;
    std::queue<T> bufferQueue;
    void check() {
        canWrite = bufferQueue.size() < size;
        canRead = !bufferQueue.empty();
    }

public:
    explicit BufferedChannel(int size_) {
        size = size_;
        check();
    }

    void send(T value) {
        if (isClosed) {
            throw std::runtime_error("Access is denied.");
        }
        std::unique_lock<std::mutex> locker(writeLock);
        while (!canWrite) {
            writeVar.wait(locker);
        }
        bufferQueue.push(value);
        check();
        writeVar.notify_one();
        readVar.notify_one();
    }

    void close() {
        isClosed = true;
    }

    std::pair<T, bool> recv() {
        if (isClosed && !canRead) {
            return { T(), false };
        }
        std::unique_lock<std::mutex> locker(readLock);
        while (!canRead) {
            writeVar.wait(locker);
        }
        T head = bufferQueue.front();
        bufferQueue.pop();
        check();
        writeVar.notify_one();
        readVar.notify_one();
        return { head, false };
    }
};

#endif //OC_LAB_CHANNEL_BUFFERED_CHANNEL_H
