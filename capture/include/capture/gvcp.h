#ifndef _GVCP_H_
#define _GVCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//used only for recv discovery package -- now as magic value
#define DEVICE_BUFFER_SIZE              512

//GigE Vision Specification
#define GVCP_DEVICE_PORT                3956 //GVCP
#define GVCP_DISCOVERY_DATA_SIZE        0xf8

#define GVCP_MANUFACTURER_NAME_OFFSET   0x00000048
#define GVCP_MANUFACTURER_NAME_SIZE     32

#define GVCP_MODEL_NAME_OFFSET          0x00000068
#define GVCP_MODEL_NAME_SIZE            32

#define GVCP_DEVICE_VERSION_OFFSET      0x00000088
#define GVCP_DEVICE_VERSION_SIZE        32

#define GVCP_MANUFACTURER_INFORMATIONS_OFFSET   0x000000a8
#define GVCP_MANUFACTURER_INFORMATIONS_SIZE     48

#define GVCP_SERIAL_NUMBER_OFFSET       0x000000d8
#define GVCP_SERIAL_NUMBER_SIZE	        16

#define GVCP_USER_DEFINED_NAME_OFFSET   0x000000e8
#define GVCP_USER_DEFINED_NAME_SIZE     16

#define GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET     0x00000008

//register addresses
#define GV_GVCP_CAPABILITY_OFFSET               0x00000934


#define GV_REGISTER_N_STREAM_CHANNELS_OFFSET    0x00000904

#define GV_HEARTBEAT_TIMEOUT_OFFSET             0x00000938
#define GV_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET   0x0000093c
#define GV_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET    0x00000940
#define GV_TIMESTAMP_CONTROL_OFFSET             0x00000944

#define GV_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET   0x00000d18
#define GV_STREAM_CHANNEL_0_PORT_OFFSET         0x00000d00
#define GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET  0x00000d04
#define GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET    0x00000d1c

#define GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET     0x00000a00

#define GV_XML_URL_0_OFFSET                     0x00000200
#define GV_XML_URL_1_OFFSET                     0x00000400
#define GV_XML_URL_SIZE                         512

#define next_packet_id(id)   (0xf000 | ((++(id)) & 0xfff))

enum gvcp_command {
    GVCP_COMMAND_DISCOVERY_CMD = 0x0002,
    GVCP_COMMAND_DISCOVERY_ACK = 0x0003,
    GVCP_COMMAND_PACKET_RESEND_CMD = 0x0040,
    GVCP_COMMAND_PACKET_RESEND_ACK = 0x0041,
    GVCP_COMMAND_READ_REG_CMD = 0x0080,
    GVCP_COMMAND_READ_REG_ACK = 0x0081,
    GVCP_COMMAND_WRITE_REG_CMD = 0x0082,
    GVCP_COMMAND_WRITE_REG_ACK = 0x0083,
    GVCP_COMMAND_READ_MEM_CMD = 0x0084,
    GVCP_COMMAND_READ_MEM_ACK = 0x0085,
    GVCP_COMMAND_WRITE_MEM_CMD = 0x0086,
    GVCP_COMMAND_WRITE_MEM_ACK = 0x0087
};

enum gvcp_packet_type {
    GVCP_PACKET_TYPE_ACK = 0x00,
    GVCP_PACKET_TYPE_CMD = 0x42,
    GVCP_PACKET_TYPE_ERROR = 0x80,
    GVCP_PACKET_TYPE_UNKNOWN_ERROR = 0x8f
};

enum gvcp_cmd_packet_flags {
    GVCP_CMD_PACKET_FLAGS_NONE = 0x00,
    GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED = 0x01,
    GVCP_CMD_PACKET_FLAGS_EXTENDED_IDS = 0x10
};

//__attribute__((packed)) used for alignement as is (any way wrong offset of packet_id)

struct gvcp_header{
    uint8_t packet_type;
    uint8_t packet_flags;
    uint16_t command;
    uint16_t size;
    uint16_t id;
}; __attribute__((packed)) 

struct gvcp_packet {
    struct gvcp_header header;
    char data[];
};

static inline void dump_data(const char *prefix, char *bytes, size_t size) {
    uint32_t i;
    fprintf(stdout, "%s: ", prefix);
    fflush(stdout);
    if (!bytes) {
        fprintf(stdout, "<null>\n");
        fflush(stdout);
        return;
    }
    for (i = 0; i < size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

#if 0 //moved to macros with the same name
static inline uint16_t next_packet_id(uint16_t packet_id) { //bug: not updated packet_id
    return 0xf000 | ((++packet_id) & 0xfff); 
}
#endif

void print_packet_full(struct gvcp_packet *packet);
void print_packet_header(struct gvcp_packet *packet);
void print_packet_data(struct gvcp_packet *packet);

struct gvcp_packet *gvcp_discovery_packet(size_t *packet_size);
int gvcp_discovery_ack(char *vendor, char *model, char *serial, char *uder_id, char *mac, struct gvcp_packet *packet);

struct gvcp_packet *gvcp_read_register_packet(uint32_t reg_addr, uint32_t packet_id, uint32_t *packet_size);
struct gvcp_packet *gvcp_write_register_packet(uint32_t reg_addr, uint32_t reg_value, uint32_t packet_id, uint32_t *packet_size);
int gvcp_register_ack(uint32_t *value, struct gvcp_packet *packet);

//important: read/write commands are possible only after connect to command socket
int read_register(int fd, uint32_t reg_addr, uint32_t *value, uint32_t packet_id);
//important: read/write commands are possible only after connect to command socket
int write_register(int fd, uint32_t reg_addr, uint32_t value, uint32_t packet_id);


//with timeout 500000us //TODO: make constant
struct gvcp_packet *listen_packet_ack(int fd);

//commands in registers
uint32_t gvcp_init(int fd, uint32_t packet_id, int data_fd);
int gvcp_watchdog(int fd, uint32_t packet_id);

#ifdef __cplusplus
}
#endif

#endif //_GVCP_H_

