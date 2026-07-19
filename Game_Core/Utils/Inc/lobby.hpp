#pragma once
#include <cstdint>

template <typename T>
void swap_pointers(T*& a, T*& b) {
    auto x = reinterpret_cast<uintptr_t>(a);
    auto y = reinterpret_cast<uintptr_t>(b);

    x ^= y; // x = a ^ b
    y ^= x; // y = b ^ (a ^ b) = a
    x ^= y; // x = (a ^ b) ^ a = b

    a = reinterpret_cast<T*>(x);
    b = reinterpret_cast<T*>(y);
}

template <typename T, int N>
class Lobby
{
    int current = 0;
    int n_active = 0;
    bool is_sleep = false;
    T* items[N] = {};
    T entities[N] = {};
public:
    Lobby()
    {
        for (int i = 0; i < N; ++i)
            items[i] = &entities[i];

        n_active = 0;
    }

    T* start()
    {
        current = 0;
        is_sleep = false;
        return items[0];
    }

    bool is_nEnd() const
    {
        return current < n_active;
    }

    T* next()
    {
        current += (is_sleep) ? 0 : 1;

        is_sleep = false;

        return items[current];
    }

    T* operator[](size_t i)
    {
        return items[i];
    }

    T* activate()
    {
        if (n_active > N)
            return nullptr;

        is_sleep = false;

        return items[n_active++];
    }

    T *sleep()
    {
        return (n_active > N) ? nullptr : items[n_active];
    }

    int active_n() const
    {
        return n_active;
    }

    int sleep_n() const
    {
        return N - n_active;
    }

    void lull()
    {
        if (n_active < 1)
            return;

        n_active -= 1;

        if (current >=  n_active)
            return;

        swap_pointers(items[current], items[n_active]);

        is_sleep = true;
    }
};
