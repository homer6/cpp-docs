// Checks claims made by docs/examples/utilities/bitset.md.
#include <bitset>
#include <stdexcept>

#include <gtest/gtest.h>

// "Mental model": bitset<8>{0b1011} has bits 0, 1 and 3 set; bit 0 is least significant.
TEST(Bitset, MentalModelBitIndex) {
    std::bitset<8> b{0b1011};
    EXPECT_TRUE(b[0]);
    EXPECT_TRUE(b[1]);
    EXPECT_FALSE(b[2]);
    EXPECT_TRUE(b[3]);
    EXPECT_EQ(b.to_string(), "00001011");
}

// "Examples": each operation's result on the page's value, in order.
TEST(Bitset, Basics) {
    std::bitset<8> b{0b1011};
    std::bitset<8> c{"00001011"};
    EXPECT_EQ(b, c);  // integer and string constructors agree

    EXPECT_TRUE(b[0]);
    EXPECT_FALSE(b.test(2));
    EXPECT_THROW((void)b.test(8), std::out_of_range);  // test() is bounds-checked
    b.set(4);
    EXPECT_EQ(b, std::bitset<8>{"00011011"});
    b.reset(0);
    EXPECT_EQ(b, std::bitset<8>{"00011010"});
    b.flip();
    EXPECT_EQ(b, std::bitset<8>{"11100101"});
    EXPECT_EQ(b.count(), 5u);
    EXPECT_EQ(b.size(), 8u);
    EXPECT_TRUE(b.any());
    EXPECT_FALSE(b.all());
    EXPECT_FALSE(b.none());

    b |= std::bitset<8>{0b0100};
    EXPECT_EQ(b.to_ulong(), 0b11100101ul);  // 229
    EXPECT_EQ(b.to_string(), "11100101");
}

// "Gotchas": to_string() shows the most significant bit first.
TEST(Bitset, ToStringMostSignificantFirst) {
    EXPECT_EQ(std::bitset<4>{1}.to_string(), "0001");
}

// "Gotchas": to_ulong()/to_ullong() throw std::overflow_error when the value doesn't fit.
TEST(Bitset, ToUlongOverflowThrows) {
    std::bitset<128> big;
    big.set(127);
    EXPECT_THROW((void)big.to_ulong(), std::overflow_error);
    EXPECT_THROW((void)big.to_ullong(), std::overflow_error);
}
