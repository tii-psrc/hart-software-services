#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

uint32_t snvm_crc32(uint32_t crc, const uint8_t *p, uint32_t len);


bool snvm_parse_u64(const char *str, uint64_t *value);
int snvm_strcmp(const char *a, const char *b);
