#ifndef _GVSP_H_
#define _GVSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define GV_STREAM_INCOMING_BUFFER_SIZE  65536

//stream parameters
#define GVSP_PACKET_SIZE_DEFAULT            1500
#define GVSP_PACKET_EXTENDED_ID_MODE_MASK   0x80
#define GVSP_PACKET_ID_MASK                 0x00ffffff
#define GVSP_PACKET_INFO_CONTENT_TYPE_MASK  0x7f000000
#define GVSP_PACKET_INFO_CONTENT_TYPE_POS   24

#define IP_HEADER_SIZE      20
#define UDP_HEADER_SIZE     8

#define MAX_FRAME_SIZE (2048 * 2448 * 3 * 2)

enum gvsp_content_type {
    GVSP_CONTENT_TYPE_DATA_LEADER = 0x01,
    GVSP_CONTENT_TYPE_DATA_TRAILER = 0x02,
    GVSP_CONTENT_TYPE_DATA_BLOCK = 0x03,
    GVSP_CONTENT_TYPE_ALL_IN = 0x04
};

enum gvsp_payload_type {
    GVSP_PAYLOAD_TYPE_UNKNOWN = -1,
    GVSP_PAYLOAD_TYPE_IMAGE = 0x0001,
    GVSP_PAYLOAD_TYPE_RAWDATA = 0x0002,
    GVSP_PAYLOAD_TYPE_FILE = 0x0003,
    GVSP_PAYLOAD_TYPE_CHUNK_DATA = 0x0004,
    GVSP_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA = 0x0005, //Deprecated by AIA GigE Vision Specification v2.0
    GVSP_PAYLOAD_TYPE_JPEG = 0x0006,
    GVSP_PAYLOAD_TYPE_JPEG2000 = 0x0007,
    GVSP_PAYLOAD_TYPE_H264 = 0x0008,
    GVSP_PAYLOAD_TYPE_MULTIZONE_IMAGE = 0x0009,
    GVSP_PAYLOAD_TYPE_IMAGE_EXTENDED_CHUNK = 0x4001
};

enum gvsp_packet_type {
    GVSP_PACKET_TYPE_OK = 0x0000,
    GVSP_PACKET_TYPE_RESEND = 0x0100,
    GVSP_PACKET_TYPE_PACKET_UNAVAILABLE = 0x800c
};

struct gvsp_data_leader {
    uint16_t flags;
    uint16_t payload_type;
    uint32_t timestamp_high;
    uint32_t timestamp_low;
    uint32_t pixel_format;
    uint32_t width;
    uint32_t height;
    uint32_t x_offset;
    uint32_t y_offset;
};

//__attribute__((packed)) used for alignement as is (any way wrong offset of packet_id)

struct gvsp_header {
    uint16_t frame_id;      //= block_id
    uint32_t packet_info;   //= payload_type (1 byte) + packet_id (3 bytes)
    uint8_t data[];
} __attribute__((packed));

struct gvsp_extended_header {
    uint16_t flags;
    uint32_t packet_info;
    uint64_t frame_id;
    uint32_t packet_id;
    uint8_t data[];
} __attribute__((packed));

struct gvsp_packet {
    uint16_t packet_type;   //= packet_status
    uint8_t header[];
} __attribute__((packed));


static inline int gvsp_packet_type_is_error(const enum gvcp_packet_type packet_type) {
    return (packet_type == GVCP_PACKET_TYPE_ERROR);
}

static inline enum gvsp_packet_type gvsp_packet_get_packet_type(const struct gvsp_packet *packet) {
    return (const enum gvsp_packet_type)ntohs(packet->packet_type);
}

static inline int gvsp_packet_has_extended_ids(const struct gvsp_packet *packet) {
    return ((packet->header[2] & GVSP_PACKET_EXTENDED_ID_MODE_MASK) != 0);
}

static inline enum gvsp_content_type gvsp_packet_get_content_type(const struct gvsp_packet *packet) {
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        return (enum gvsp_content_type)((ntohl(header->packet_info) & GVSP_PACKET_INFO_CONTENT_TYPE_MASK) >> GVSP_PACKET_INFO_CONTENT_TYPE_POS);
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return (enum gvsp_content_type)((ntohl(header->packet_info) & GVSP_PACKET_INFO_CONTENT_TYPE_MASK) >> GVSP_PACKET_INFO_CONTENT_TYPE_POS);
    }
}

static inline uint32_t gvsp_packet_get_packet_id(const struct gvsp_packet *packet) {
    //dump_data("packet", packet, 40);
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        return ntohl(header->packet_id);
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return (ntohl(header->packet_info) & GVSP_PACKET_ID_MASK);
    }
}

static inline uint64_t gvsp_packet_get_frame_id(const struct gvsp_packet *packet) {
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        //return GUINT64_FROM_BE(header->frame_id); -- check this place: convert from big endian
        return header->frame_id;
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return ntohs(header->frame_id);
    }
}

static inline char *gvsp_packet_get_data(const struct gvsp_packet *packet) {
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        return (char*)&(header->data);
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return (char*)&(header->data);
    }
}


static inline size_t gvsp_packet_get_data_size(const struct gvsp_packet *packet, size_t packet_size) {
    if (gvsp_packet_has_extended_ids(packet)) {
        return packet_size - sizeof(struct gvsp_packet) - sizeof(struct gvsp_extended_header);
    }
    else {
        return packet_size - sizeof(struct gvsp_packet) - sizeof(struct gvsp_header);
    }
}

static inline enum gvsp_payload_type gvsp_packet_get_payload_type(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    enum gvsp_payload_type  payload_type = ntohs(leader->payload_type);
    switch (payload_type) {
        case GVSP_PAYLOAD_TYPE_IMAGE:
        case GVSP_PAYLOAD_TYPE_FILE:
            return GVSP_PAYLOAD_TYPE_IMAGE;
        default:
            return GVSP_PAYLOAD_TYPE_UNKNOWN;
    }
    return GVSP_PAYLOAD_TYPE_UNKNOWN;
}

static inline uint32_t gvsp_packet_get_x_offset(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->x_offset);
}

static inline uint32_t gvsp_packet_get_y_offset(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->y_offset);
}

static inline uint32_t gvsp_packet_get_width(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->width);
}

static inline uint32_t gvsp_packet_get_height(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->height);
}

static inline uint32_t gvsp_packet_get_pixel_format(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->pixel_format);
}

static inline uint64_t gvsp_packet_get_timestamp(const struct gvsp_packet *packet, uint64_t timestamp_tick_frequency) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    uint64_t timestamp_s;
    uint64_t timestamp_ns;
    uint64_t timestamp;

    if (timestamp_tick_frequency < 1) {
        return 0;
    }

    timestamp = ((uint64_t)ntohl(leader->timestamp_high) << 32) | ntohl(leader->timestamp_low);
    timestamp_s = timestamp / timestamp_tick_frequency;
    timestamp_ns = ((timestamp % timestamp_tick_frequency) * 1000000000) / timestamp_tick_frequency;

    timestamp_ns += timestamp_s * 1000000000;

    return timestamp_ns;
}


#ifdef __cplusplus
}
#endif

#endif //_GVSP_H_

