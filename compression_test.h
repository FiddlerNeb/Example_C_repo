
#ifndef COMPRESS_TEST_H
#define COMPRESS_TEST_H
#include <stdint.h>

#define GET_VAR_NAME(x) #x

#define MARKDOWN_OUTPUT 0

typedef enum
{
  FAIL = 0,
  PASS = 1,
  INVALID = -1,
} ret_code;

#define MIN_COMPRESSIBLE_FILE_PERC 97
#define MAX_INPUT_SIZE 512
#define BUFFER_SIZE 24
#define TOKEN_INIT 0
#define NIBBLE_MAX 0xF
#define NIBBLE_NON_MATCH_BIT 0x8
#define NIBBLE_VALUE_MASK 0x7
#define PRINT_ROW_SIZE 8
#define ERASED_BYTE 0xFF
#define MAX_NON_TOKEN_DATA 0x7F

typedef uint8_t buffer_element_t;
typedef uint64_t array_size_t;

typedef union
{
  uint8_t byte;
  struct
  {
    uint8_t after : 4;
    uint8_t before : 4;
  };
} cmprss_token_t;

void print_array(uint8_t *data_ptr, array_size_t data_size, char *array_name);
// cmprss_token_t getMatchLen(buffer_element_t *data_ptr, array_size_t i, array_size_t data_size);
int estimate_array_size(buffer_element_t *data_ptr, array_size_t data_size);
int byte_compress(buffer_element_t *data_ptr, array_size_t data_size);
int run_verbose_compression_test(buffer_element_t *data_ptr, array_size_t data_size);
uint8_t ArraysAreEqual(buffer_element_t *data_ptr1, buffer_element_t *data_ptr2, array_size_t data_size);

int byte_decompress(buffer_element_t *uncmprss_data_ptr, array_size_t uncmprss_data_size, buffer_element_t *cmprss_data_ptr, array_size_t cmpress_data_size);

#endif // COMPRESS_TEST_H