#pragma once

#include "CodesTable.h"

namespace phuffman {
    namespace CPU {
        void Decode(unsigned char** data,
                    size_t* data_length,
                    CodesTable codes_table,
                    unsigned int* encoded_data,
                    size_t encoded_data_length,
                    unsigned char encoded_data_trail_zeroes,
                    size_t block_int_size,
                    unsigned char* block_bit_offsets,
                    unsigned int* block_sym_sizes,
                    size_t block_count);
    }
}
