#ifndef E2EEP_BYTE_ARRAY_HPP
#define E2EEP_BYTE_ARRAY_HPP

#include <string>
#include <cstring>
#include "log.hpp"



class ByteArray {
    public:

        ByteArray(){ 
            m_allocated = false;
            this->m_mem_size = 0;
            this->data = NULL;
            this->size = 0;
        }
        ~ByteArray(){ 
            this->free_data();
        }

        uint8_t* data;
        size_t   size;

        size_t mem_size() {
            return m_mem_size;
        }

        void allocate(size_t N) {
            if(!m_allocated && !data) {
                data = (uint8_t*)malloc(N);
                m_mem_size = N;
                m_allocated = true;
            }
        }

        void free_data() {
            if(m_allocated && data) {
                free(data);
                data = NULL;
            }
        }

    private:
        size_t  m_mem_size;
        bool    m_allocated;
};


static constexpr char HEX_SEPARATOR = ':';

std::string bytes_to_hexstr(uint8_t* bytes, size_t size);
ByteArray hexstr_to_bytes(const std::string& hexstr);


#endif
