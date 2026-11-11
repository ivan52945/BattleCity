/* Unit tests of the Utils pool (Lobby<T, N>) - derived from the usage
 * contract in Game_Core/Utils/README.md: specification-based design
 * (equivalence partitions, boundary values, slot state transitions).
 * Every test case traces to a contract point; the mutant it is
 * supposed to catch is named in the comment where it matters.
 *
 * Build & run (repo root):
 *   cmake -S Game_Core/Tests -B Game_Core/Tests/build -G Ninja
 *   cmake --build Game_Core/Tests/build
 *   ctest --test-dir Game_Core/Tests/build --output-on-failure
 *
 * Iteration idiom of the contract (start() / is_nEnd() / next()) is
 * used exactly as the README example shows: the condition is checked
 * before every body step, so next() never runs past the window.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "lobby.hpp"

namespace {

struct Node
{
    int id = 0;
    int life = 1; /* dead when 0 - the README example predicate */
};

constexpr int N = 4;
using Pool = Lobby<Node, N>;

} /* namespace */

/* --- activate(): growth to the boundary, full-pool guard --- */

TEST_CASE("activate_empty_pool")
{
    Pool p;
    Node* slot = p.activate();
    REQUIRE(slot != nullptr);
    CHECK(p.active_n() == 1);
    CHECK(p.sleep_n() == N - 1);
}

TEST_CASE("activate_to_full")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        REQUIRE(p.activate() != nullptr);
    CHECK(p.active_n() == N);
    CHECK(p.sleep_n() == 0);
}

TEST_CASE("activate_on_full")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        (void)p.activate();
    /* mutant target: ">= -> >" (the point-I fix) - reads items[N] */
    CHECK(p.activate() == nullptr);
    CHECK(p.active_n() == N);
    CHECK(p.sleep_n() == 0);
}

TEST_CASE("activate_distinct_slots")
{
    Pool p;
    Node* slots[N];
    for (int i = 0; i < N; ++i)
        slots[i] = p.activate();
    for (int i = 0; i < N; ++i)
        for (int j = i + 1; j < N; ++j)
            CHECK(slots[i] != slots[j]);
}

/* --- sleep(): reservation invisible to passes and counters --- */

TEST_CASE("sleep_reserve_invisible")
{
    Pool p;
    int before_active = p.active_n();
    int before_free = p.sleep_n();

    Node* reserved = p.sleep();
    REQUIRE(reserved != nullptr);

    CHECK(p.active_n() == before_active);
    CHECK(p.sleep_n() == before_free);

    int visits = 0;
    for (Node* s = p.start(); p.is_nEnd(); s = p.next()) {
        (void)s;
        ++visits;
    }
    CHECK(visits == 0);
}

TEST_CASE("sleep_on_empty")
{
    Pool p;
    Node* reserved = p.sleep();
    REQUIRE(reserved != nullptr);
    CHECK(p.active_n() == 0);
    CHECK(p.sleep_n() == N);
}

TEST_CASE("sleep_on_full")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        (void)p.activate();
    CHECK(p.sleep() == nullptr);
}

TEST_CASE("reserve_then_activate")
{
    Pool p;
    Node* reserved = p.sleep();
    REQUIRE(reserved != nullptr);

    Node* activated = p.activate();
    REQUIRE(activated != nullptr);
    CHECK(activated == reserved); /* the reserved slot, same storage */
    CHECK(p.active_n() == 1);
}

TEST_CASE("sleep_hands_tail_until_activated")
{
    Pool p;
    Node* first = p.sleep();
    REQUIRE(first != nullptr);

    /* резервы не накапливаются: до activate() выдаётся тот же слот */
    Node* second = p.sleep();
    REQUIRE(second != nullptr);
    CHECK(second == first);

    first->id = 7;
    Node* activated = p.activate();
    CHECK(activated == first);
    CHECK(p.active_n() == 1);

    /* после активации выдаётся следующий хвостовой слот */
    Node* next_free = p.sleep();
    REQUIRE(next_free != nullptr);
    CHECK(next_free != first);
    CHECK(p.active_n() == 1);
}

/* --- lull(): pop under cursor, unstable order, swap-in step --- */

TEST_CASE("lull_pops_under_cursor")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        p.activate()->id = i;

    (void)p.start();     /* cursor on id0 */
    (void)p.next();      /* cursor on id1 */
    p.lull();            /* id1 popped */

    CHECK(p.active_n() == N - 1);

    /* id1 does not appear until the end of the pass */
    for (Node* s = p.next(); p.is_nEnd(); s = p.next())
        CHECK(s->id != 1);
}

TEST_CASE("lull_tail_element")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        p.activate()->id = i;

    /* cursor on the last active slot (id3) */
    (void)p.start();
    (void)p.next();
    (void)p.next();
    (void)p.next();
    p.lull();

    CHECK(p.active_n() == N - 1);
    CHECK_FALSE(p.is_nEnd()); /* pass is over, next() is not called */
}

TEST_CASE("next_after_lull_returns_swapped")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        p.activate()->id = i;

    (void)p.start();     /* id0 */
    (void)p.next();      /* id1 */
    p.lull();            /* id1 popped; the tail (id3) took its place */

    Node* swapped_in = p.next(); /* cursor does not move */
    CHECK(swapped_in->id == 3);

    Node* after = p.next();      /* normal advance afterwards */
    CHECK(after->id == 2);
}

TEST_CASE("order_unstable_after_lull")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        p.activate()->id = i;

    (void)p.start();
    (void)p.next();          /* cursor on id1 */
    Node* before = p[1];
    REQUIRE(before->id == 1);

    p.lull();

    CHECK(p[1]->id != 1);    /* another element sits at the index now */
}

/* --- iterator: empty pass, full pass, restart, lull scenario --- */

TEST_CASE("iterate_empty")
{
    Pool p;
    int visits = 0;
    for (Node* s = p.start(); p.is_nEnd(); s = p.next()) {
        (void)s;
        ++visits;
    }
    CHECK(visits == 0);
}

TEST_CASE("iterate_all_active")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        p.activate()->id = i;

    int visits = 0;
    bool seen[N] = {};
    for (Node* s = p.start(); p.is_nEnd(); s = p.next()) {
        CHECK(s->id >= 0);
        CHECK(s->id < N);
        CHECK_FALSE(seen[s->id]); /* visited exactly once */
        seen[s->id] = true;
        ++visits;
    }
    CHECK(visits == N);
    for (int i = 0; i < N; ++i)
        CHECK(seen[i]);
}

TEST_CASE("start_resets_pass")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        p.activate()->id = i;

    /* an aborted pass: one step in, then restart */
    (void)p.start();
    (void)p.next();

    int visits = 0;
    bool seen[N] = {};
    for (Node* s = p.start(); p.is_nEnd(); s = p.next()) {
        CHECK_FALSE(seen[s->id]);
        seen[s->id] = true;
        ++visits;
    }
    CHECK(visits == N);
}

TEST_CASE("lull_during_pass")
{
    Pool p;
    for (int i = 0; i < N; ++i) {
        Node* s = p.activate();
        s->id = i;
        s->life = (i % 2 == 0) ? 1 : 0; /* odd ids are dead */
    }

    /* the README example scenario: kill corpses out mid-pass */
    int alive_visits = 0;
    bool seen[N] = {};
    for (Node* s = p.start(); p.is_nEnd(); s = p.next()) {
        if (s->life == 0) {
            p.lull();
            continue;
        }
        CHECK_FALSE(seen[s->id]);
        seen[s->id] = true;
        ++alive_visits;
    }
    CHECK(alive_visits == 2);        /* id0 and id2 */
    CHECK(seen[0]);
    CHECK(seen[2]);
    CHECK(p.active_n() == 2);
}

/* --- slot reuse and counter invariant --- */

TEST_CASE("lulled_slot_reused")
{
    Pool p;
    for (int i = 0; i < N; ++i)
        p.activate()->id = i;

    (void)p.start();
    Node* victim = p.next();  /* id1 */
    p.lull();

    Node* reused = p.activate();
    CHECK(reused == victim);  /* same slot storage handed out again */
    reused->id = 9;           /* reconfiguration - caller's duty */
    CHECK(p.active_n() == N);
}

TEST_CASE("counters_sum_invariant")
{
    Pool p;
    CHECK(p.active_n() + p.sleep_n() == N);

    (void)p.sleep();
    CHECK(p.active_n() + p.sleep_n() == N);

    (void)p.activate();
    CHECK(p.active_n() + p.sleep_n() == N);

    (void)p.start();
    (void)p.next();
    p.lull();
    CHECK(p.active_n() + p.sleep_n() == N);
}

/* --- degenerate boundary: a pool of one slot --- */

TEST_CASE("pool_of_one")
{
    Lobby<Node, 1> p;
    CHECK(p.activate() != nullptr);
    CHECK(p.activate() == nullptr);
    CHECK(p.sleep() == nullptr);

    (void)p.start();
    p.lull();                 /* the only element popped */
    CHECK(p.active_n() == 0);
    CHECK(p.activate() != nullptr);
}
