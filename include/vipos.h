#ifndef VIPOS_H
#define VIPOS_H

#include <stdint.h>

#define VIP_PACKAGE_ADDR 0x00020000u
#define VIP_MAGIC0 'V'
#define VIP_MAGIC1 'I'
#define VIP_MAGIC2 'P'
#define VIP_MAGIC3 '0'
#define VIP_PAYLOAD_BYTECODE 1u

struct vip_header {
    char magic[4];
    uint16_t version;
    uint16_t header_size;
    uint32_t manifest_size;
    uint32_t payload_size;
    uint32_t payload_type;
    uint32_t reserved;
    uint8_t payload_sha256[32];
} __attribute__((packed));

#endif
