#pragma once
#include <cstddef>
#include <cstdint>
#include <utility> /* std::swap */

/* The per-slot display stamping below only writes a pointer member
 * of the pooled entities; the interface itself stays undeclared. */
class Disp_Iface;

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

    /* Stamp every slot of the pool with the display of the owning
     * Game. Slot objects are created once with the pool and only
     * reused afterwards, so the single stamp in Game::init() covers
     * their whole lifetime (before the first print()/clear()). */
    void set_disp(Disp_Iface* d)
    {
        for (int i = 0; i < N; ++i)
            items[i]->disp = d;
    }

    T* activate()
    {
        /* Full pool: no slot to hand out (items[N] is past the end). */
        if (n_active >= N)
            return nullptr;

        is_sleep = false;

        return items[n_active++];
    }

    T *sleep()
    {
        /* Full pool: no free slot to reserve; the caller must bail. */
        return (n_active >= N) ? nullptr : items[n_active];
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

        std::swap(items[current], items[n_active]);

        is_sleep = true;
    }
};
