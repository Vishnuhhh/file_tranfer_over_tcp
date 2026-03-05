#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define CHUNK_SIZE 65536
#define FILENAME_SIZE 256

typedef struct {
    uint64_t file_size;
    uint32_t chunk_size;
    char filename[FILENAME_SIZE];
} file_header;

typedef struct {
    uint32_t seq;
    uint32_t size;
    uint32_t checksum;
} chunk_header;


uint32_t crc32(const unsigned char *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for(size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for(int j = 0; j < 8; j++)
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

#endif
