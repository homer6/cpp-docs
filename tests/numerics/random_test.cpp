// Checks claims made by docs/examples/numerics/random.md.
#include <algorithm>
#include <concepts>
#include <iterator>
#include <numeric>
#include <random>
#include <vector>

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
