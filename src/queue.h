#pragma once

#include <iostream>
#include <cstdlib>
#include <unistd.h>
using namespace std;

// define default capacity of the queue
#define M_QUEUE_SIZE 64

// Class for queue
template <class X>
class MQueue
{
    pthread_mutex_t lock;   // Mutex for thread safe functions
	X *arr;			// array to store queue elements
	int capacity;	// maximum capacity of the queue
	int front;		// front points to front element in the queue (if any)
	int rear;		// rear points to last element in the queue
    int count;		// current size of the queue

    int dequeue();
    int dequeueSafe();
public:
    MQueue(int size = M_QUEUE_SIZE);		// constructor
    ~MQueue();

    int enqueue(X x);
    int enqueueSafe(X x);
    X peek();
    X peekSafe();
	int size();
    bool isEmpty();
	bool isFull();
};

// Constructor to initialize queue
template <class X>
MQueue<X>::MQueue(int size)
{
	arr = new X[size];
	capacity = size;
	front = 0;
	rear = -1;
	count = 0;

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        exit(EXIT_FAILURE);
    }

}
template <class X>
MQueue<X>::~MQueue(){
    pthread_mutex_destroy(&lock);
}
// Utility function to remove front element from the queue
template <class X>
int MQueue<X>::dequeue()
{
	// check for queue underflow
	if (isEmpty())
    {
		return -1;
	}

	//arr[front] = NULL;
	front = (front + 1) % capacity;
	count--;
	return 0;
}

// Utility function to add an item to the queue
template <class X>
int MQueue<X>::enqueue(X item)
{
	// check for queue overflow
	if (isFull()) 
    {
        dequeue();
	}

	rear = (rear + 1) % capacity;
	arr[rear] = item;
	count++;
	return 0;
}

// Utility function to remove front element from the queue
template <class X>
int MQueue<X>::dequeueSafe()
{
    // check for queue underflow
    if (isEmpty())
    {
        return -1;
    }

    front = (front + 1) % capacity;
    count--;
    return 0;
}

// Utility function to add an item to the queue (Thread Safe)
template <class X>
int MQueue<X>::enqueueSafe(X item)
{
    // check for queue overflow
    if (isFull())
    {
        dequeueSafe();
    }

    pthread_mutex_lock(&lock);
    rear = (rear + 1) % capacity;
    arr[rear] = item;
    count++;
    pthread_mutex_unlock(&lock);
    return 0;
}

// Utility function to return front element in the queue
template <class X>
X MQueue<X>::peek()
{
	if (isEmpty()) 
	{
		//cout << "UnderFlow\nProgram Terminated\n";
        return X();
	}
    return arr[front];
}

// Utility function to return front element in the queue (Thread Safe)
template <class X>
X MQueue<X>::peekSafe()
{
    if (isEmpty())
    {
        //cout << "UnderFlow\nProgram Terminated\n";
        return X();
    }

    pthread_mutex_lock(&lock);
    X x = arr[front];//.clone();
    pthread_mutex_unlock(&lock);
    return x;
}

// Utility function to return the size of the queue
template <class X>
int MQueue<X>::size()
{
	return count;
}

// Utility function to check if the queue is empty or not
template <class X>
bool MQueue<X>::isEmpty()
{
    return (size() == 0);
}

// Utility function to check if the queue is full or not
template <class X>
bool MQueue<X>::isFull()
{
	return (size() == capacity);
}

//Possible Improvment Links
//https://github.com/lcit/FIFO/blob/master/FIFO.hpp
//https://gist.github.com/holtgrewe/8728757
//https://stackoverflow.com/questions/36762248/why-is-stdqueue-not-thread-safe
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class SharedQueue
{
public:
    SharedQueue();
    ~SharedQueue();

    T& front();
    T& peekSafe()
    {
        return front();
    }

    void pop_front();

    void push_back(const T& item);
    void push_back(T&& item);

    int size();
    bool empty();

private:
    std::deque<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

template <typename T>
SharedQueue<T>::SharedQueue(){}

template <typename T>
SharedQueue<T>::~SharedQueue(){}

template <typename T>
T& SharedQueue<T>::front()
{
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty())
    {
        cond_.wait(mlock);
    }
    return queue_.front();
}

template <typename T>
void SharedQueue<T>::pop_front()
{
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty())
    {
        cond_.wait(mlock);
    }
    queue_.pop_front();
}

template <typename T>
void SharedQueue<T>::push_back(const T& item)
{
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push_back(item);
    mlock.unlock();     // unlock before notificiation to minimize mutex con
    cond_.notify_one(); // notify one waiting thread

}

template <typename T>
void SharedQueue<T>::push_back(T&& item)
{
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push_back(std::move(item));
    mlock.unlock();     // unlock before notificiation to minimize mutex con
    cond_.notify_one(); // notify one waiting thread

}

template <typename T>
int SharedQueue<T>::size()
{
    std::unique_lock<std::mutex> mlock(mutex_);
    int size = queue_.size();
    mlock.unlock();
    return size;
}
