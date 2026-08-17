#ifndef RNG_HPP
#define RNG_HPP

#include <cstdint>
#include <array>

namespace ff1 {

class RNG {
public:
    RNG();

    // Initialize RNG state from file or table
    void load_table(const uint8_t* table_256_bytes);

    // Get current RNG pointer index (0-255)
    uint8_t get_index() const { return index_; }
    void set_index(uint8_t index) { index_ = index; }

    // Advance RNG and return byte value (0-255) matching NES BattleRNG
    uint8_t next_byte();

    // Get random integer in range [min_val, max_val] inclusive
    int next_range(int min_val, int max_val);

private:
    uint8_t index_ = 0;
    std::array<uint8_t, 256> table_;
};

} // namespace ff1

#endif // RNG_HPP
