#include <cstdint>
#include "byte_array.hpp"


ByteArray hexstr_to_bytes(const std::string& hexstr) {
    ByteArray array;
    array.allocate(hexstr.size());

    constexpr size_t HEXBUF_SIZE = 8;
    char   hexbuf[HEXBUF_SIZE+1] = { 0 };
    size_t  hexbuf_idx = 0;

    for(size_t i = 0; i < hexstr.size(); i++) {
        char c = hexstr[i];
        if(c == HEX_SEPARATOR) {
            if(hexbuf_idx == 0) {
                continue; // Skip until there is no 'HEX_SEPARATOR'
            }

            array.data[array.size] = (uint8_t)strtol(hexbuf, NULL, 16);
            memset(hexbuf, 0, HEXBUF_SIZE);
            hexbuf_idx = 0;
            array.size++;
        }
        else {
            hexbuf[hexbuf_idx++] = c;
            if(hexbuf_idx > HEXBUF_SIZE) {
                log_print(ERROR, "Invalid data. Corrupted?");
                return array;
            }
        }
    }
    return array;
}

std::string bytes_to_hexstr(uint8_t* bytes, size_t size) {
    if(!bytes || (size == 0)) {
        return "";
    }
    std::string str = "";
    str.reserve(256);

    const char* HEX = "0123456789abcdef";

    for(size_t i = 0; i < size; i++) {
        str.push_back(HEX[ (abs(bytes[i]) >> 4) % 0xf ]);
        str.push_back(HEX[ (abs(bytes[i])) & 0xf ]);
        str.push_back(HEX_SEPARATOR);
    }

    return str;
}


