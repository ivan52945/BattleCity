#pragma once

template <typename T, int N>
class Lobby
{
    int current = 0;
    int r_current = 0;
    bool is_sleep = false;
    int n_active = 0;
    T* items[N + 1] = {};
public:
    Lobby() = default;

    Lobby(T items_add[], const int n)
    {
        load(items_add, n);
    }

    void load(T items_add[], int n)
    {
        n = (n <= N) ? n : N;

        for (int i = 0; i < n; ++i)
            items[i + 1] = &items_add[i];

        n_active = 0;
    }

    void clear()
    {
        for (int i = 0; i < N; ++i)
            items[i] = nullptr;

        n_active = 0;
    }

    T* start()
    {
        current = 0;
        is_sleep = false;
        return items[0];
    }

    T* next()
    {
        current += (is_sleep) ? 0 : 1;

        is_sleep = false;

        current = (current < n_active) ? current : n_active;

        return items[current];
    }

    T** r_start()
    {
        return &items[0];
    }

    T *item(int i)
    {
        return (i < n_active) ? items[i] : nullptr;
    }

    T* activate()
    {
        if (n_active >= N)
            return nullptr;

        is_sleep = false;

        T* activated = items[++n_active];
        items[n_active - 1] = activated;
        items[n_active] = nullptr;

        return activated;
    }

    T *sleep()
    {
        return (n_active >= N) ? nullptr : items[n_active + 1];
    }

    int active_n() const
    {
        return n_active;
    }

    int sleep_n() const
    {
        return N-n_active;
    }

    void lull()
    {
        if (n_active < 1)
            return;

        is_sleep = true;

        items[n_active] = items[current];
        items[current] = items[--n_active];
        items[n_active] = nullptr;
    }
};
