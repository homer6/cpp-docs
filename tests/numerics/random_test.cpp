// Checks claims made by docs/examples/numerics/random.md.
#include <algorithm>
#include <concepts>
#include <iterator>
#include <numeric>
#include <random>
#include <vector>
#include <version>

#include "support/feature.hpp"

#include <gtest/gtest.h>

// "Reproducible runs": the same seed gives the same sequence; the engine
// algorithm is fully specified (the standard fixes mt19937's 10000th output).
TEST(Random, FixedSeedIsReproducible) {
    std::mt19937 a{12345}, b{12345};
    for (int i = 0; i < 100; ++i) EXPECT_EQ(a(), b());
    std::mt19937 d;  // default seed 5489
    d.discard(9999);
    EXPECT_EQ(d(), 4123659995u);
}

// "Gotchas: distribution ranges": uniform_int is inclusive of b; uniform_real is [a, b).
TEST(Random, DistributionRanges) {
    std::mt19937 gen{1};
    std::uniform_int_distribution<int> die{1, 6};
    bool seen[7]{};
    for (int i = 0; i < 1000; ++i) {
        int r = die(gen);
        ASSERT_GE(r, 1);
        ASSERT_LE(r, 6);
        seen[r] = true;
    }
    EXPECT_TRUE(seen[1] && seen[6]);  // both ends reachable
    std::uniform_real_distribution<double> real{0.0, 1.0};
    for (int i = 0; i < 1000; ++i) {
        double r = real(gen);
        ASSERT_GE(r, 0.0);
        ASSERT_LT(r, 1.0);
    }
}

// "Distributions": bernoulli{0.25} is true about a quarter of the time.
TEST(Random, BernoulliProbability) {
    std::mt19937 gen{7};
    std::bernoulli_distribution d4{0.25};
    int hits = 0;
    for (int i = 0; i < 10000; ++i) hits += d4(gen);
    EXPECT_NEAR(hits / 10000.0, 0.25, 0.03);
}

// "Shuffle and sample": shuffle permutes; sample picks 5 distinct elements in order.
TEST(Random, ShuffleAndSample) {
    std::mt19937 gen{42};
    std::vector<int> v(20);
    std::iota(v.begin(), v.end(), 0);
    auto orig = v;
    std::shuffle(v.begin(), v.end(), gen);
    EXPECT_TRUE(std::is_permutation(v.begin(), v.end(), orig.begin()));
    std::vector<int> out;
    std::sample(orig.begin(), orig.end(), std::back_inserter(out), 5, gen);
    EXPECT_EQ(out.size(), 5u);
    EXPECT_TRUE(std::is_sorted(out.begin(), out.end()));  // sample is stable for forward iterators
}

// "Engines & distributions": mt19937 is 32-bit, mt19937_64 is 64-bit.
TEST(Random, EngineWidths) {
    static_assert(std::mt19937::max() == 0xFFFFFFFFu);
    static_assert(std::mt19937_64::max() == 0xFFFFFFFFFFFFFFFFull);
    static_assert(std::uniform_random_bit_generator<std::minstd_rand>);
}

// "Counter-based streams: std::philox_engine (C++26)": the predefined engines,
// their parameters, and the standard's 10000th-output check values.
TEST(Random, PhiloxPredefined) {
    CPPDOCS_REQUIRE(__cpp_lib_philox_engine);
#ifdef __cpp_lib_philox_engine
    static_assert(std::philox4x32::word_count == 4 && std::philox4x32::round_count == 10);
    static_assert(std::philox4x64::word_count == 4 && std::philox4x64::round_count == 10);
    static_assert(std::uniform_random_bit_generator<std::philox4x32>);
    std::philox4x32 e32;
    e32.discard(9999);
    EXPECT_EQ(e32(), 1955073260u);
    std::philox4x64 e64;
    e64.discard(9999);
    EXPECT_EQ(e64(), 3409172418970261260ull);
    EXPECT_LT(sizeof(std::philox4x32), sizeof(std::mt19937));  // a few words vs 5000 bytes
#endif
}

// "Counter-based streams": set_counter jumps to a block (last element is the low
// word); a different counter gives a different stream; works with distributions.
TEST(Random, PhiloxSetCounter) {
    CPPDOCS_REQUIRE(__cpp_lib_philox_engine);
#ifdef __cpp_lib_philox_engine
    std::philox4x32 gen{42};
    std::uniform_int_distribution<int> die{1, 6};
    int roll = die(gen);
    EXPECT_GE(roll, 1);
    EXPECT_LE(roll, 6);

    std::philox4x32 a{42}, b{42};
    a.discard(4 * 1000);
    b.set_counter({0, 0, 0, 1000});
    for (int i = 0; i < 8; ++i) EXPECT_EQ(a(), b());

    // "Gotchas": {k, 0, 0, 0} is somewhere else entirely.
    std::philox4x32 c{42}, d{42};
    c.discard(4 * 1000);
    d.set_counter({1000, 0, 0, 0});
    EXPECT_NE(c(), d());

    // Disjoint stream for a second worker.
    std::philox4x32 w0{42}, w1{42};
    w1.set_counter({0, 1, 0, 0});
    EXPECT_NE(w0(), w1());

    // "Gotchas": set_counter discards buffered outputs (the next call starts a block).
    std::philox4x32 e{42}, f{42};
    e();                          // e now has 3 buffered outputs of block 0
    e.set_counter({0, 0, 0, 0});
    for (int i = 0; i < 4; ++i) EXPECT_EQ(e(), f());
#endif
}
