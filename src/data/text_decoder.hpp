#ifndef TEXT_DECODER_HPP
#define TEXT_DECODER_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace ff1 {

class TextDecoder {
public:
    // Decode a single NES byte into its string representation
    static std::string decode_char(uint8_t code);

    // Decode a byte array into a std::string using DTE and standard mapping
    static std::string decode_string(const uint8_t* data, size_t length);

    // Decode a null-terminated byte sequence
    static std::string decode_null_terminated(const uint8_t* data, size_t max_length = 256);
};

} // namespace ff1

#endif // TEXT_DECODER_HPP
