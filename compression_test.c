/**
 * @file compression_test_lib.c
 * @author Benjamin Odegaard (ben.odegaard10@gmail.com)
 * @brief a test compression implementation for technical interview
 * @version 0.1
 * @date 2025-09-15
 * Code Design Test: Data Compression Design
 *   Design an algorithm that will compress a given data buffer of bytes.
 *
 *   Please describe your design and submit an implementation in a language of your choice.
 *   The algorithm will live within a function.
 *   This function will be called with two arguments;
 *      a pointer to the data buffer (data_ptr) and
 *       the number of bytes to compress (data_size).
 *   After the function executes the data in the buffer will be modified and the size of the modified buffer will be returned.
 * Assumptions:
 *   The data_ptr will point to an array of bytes.
 *   Each byte will contain a number from 0 to 127 (0x00 to 0x7F).
 *   It is common for the data in the buffer to have the same value repeated in the series.
 *   The compressed data will need to be decompressable.
 *   Please ensure that your algorithm allows for a decompression algorithm to return the buffer to it’s previous form.
 * Example data and function call:
 *   // Data before the call
 *   // data_ptr[] = { 0x03, 0x74, 0x04, 0x04, 0x04, 0x35, 0x35, 0x64,
 *   //                0x64, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
 *   //                0x56, 0x45, 0x56, 0x56, 0x56, 0x09, 0x09, 0x09 };
 *   // data_size = 24;
 *
 *   // new_size = byte_compress( data_ptr, data_size );
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <assert.h>

#define BUFFER_SIZE 24
#define TOKEN_INIT 0
#define NIBBLE_MAX 0xF

typedef union
{
  uint8_t byte;
  struct
  {
    uint8_t before : 4;
    uint8_t after : 4;
  };
} cmprss_token_t;

struct match_result_struct
{
  uint8_t matched_len;
  uint8_t non_matched_len;
};

uint8_t getMatchLen(uint8_t *data_ptr, uint8_t i)
{
  match_result_struct result;
  result.matched_len = 0;
  result.non_matched_len = 0;

  if (data_ptr[i] == data_ptr[i + 1])
  {
    // find the number of consecutive matches after the currVal
    for (j = 2; j < NIBBLE_MAX; j++)
    {
      if (data_ptr[i] == data_ptr[i + j])
        result.matched_len++;
      else
      {
        break;
      }
    }
  }
  else
  {
    //find the number of consecutive non-matches after the currVal
    for (j = 2; j < NIBBLE_MAX; j++)
    {
      if (data_ptr[i+(j-1)] == data_ptr[i + j])
        break;
      else
      {
        result.non_matched_len++;
      }
    }
  }


  

  return result;
}

/**
 * @brief compresses a byte array of data using a custom algorithm
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
int byte_compress(uint8_t *data_ptr, uint64_t data_size)
{
  uint64_t size_after_compression = 0;
  uint64_t i = 0, j = 0;
  uint8_t prev_i = 0, currVal = 0, nextVal = 0, non_match = 0;
  uint8_t readIndex = 0, writeIndex = 0, buffReadIndex = 0, buffWritePtr = 0;
  cmprss_token_t token1;
  token1.before = NIBBLE_MAX;
  token1.after = NIBBLE_MAX;
  uint8_t buffer[16] = {0xFF};

  {
    uint8_t match_len = 0;
    do
    {
      currVal = data_ptr[i];
      // find the number of consecutive matches after the currVal
      for (j = 0; j < NIBBLE_MAX; j++)
      {
        if (currVal == data_ptr[i + j])
          match_len++;
        else
        {
          non_match++;
        }
      }
      token1.before = match_len;

      // grab the next value, and increment i
      currVal = data_ptr[++i];

      // find the number of consecutive matches after the currVal
      for (j = 0; j < NIBBLE_MAX; j++)
      {
        if (data_ptr[i] == data_ptr[i + j])
          after_match_len++;
      }
      token1.after = after_match_len;

      // insert the token
      data_ptr[i] = token1.byte;

      // grab the next value, and increment i
      nextVal = data_ptr[++i];

      if (currVal != nextVal)

        // save and go to the next unmatched index
        prev_i = i;
      i = i + after_match_len;
    } while (i < data_size);
  }

  return size_after_compression;
}

/**
 * @brief compresses a byte array of data using a custom algorithm
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
int byte_decompress(uint8_t *data_ptr, uint64_t data_size)
{
  uint64_t size_after_decompression = 0;

  return size_after_decompression;
}

/**
 * @brief calls compress and decompress on a test sample of data and times it
 *
 */
void main(void)
{
  clock_t start_time, end_time;
  uint64_t time_taken = 0.0f;
  uint8_t data_ptr[BUFFER_SIZE] = {0x03, 0x74, 0x04, 0x04, 0x04, 0x35, 0x35, 0x64,
                                   0x64, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x56, 0x45, 0x56, 0x56, 0x56, 0x09, 0x09, 0x09};

  uint64_t main_data_size = BUFFER_SIZE;
  uint64_t main_cmprss_size = main_data_size;
  uint64_t main_decmprss_size = main_data_size;

  printf("\n\nInitialization complete\n");
  printf("Size before: %d\n", main_data_size);

  start_time = clock();
  main_cmprss_size = byte_compress(data_ptr, main_data_size);
  end_time = clock();

  // check that clock ticks are indeed seconds
  assert(CLOCKS_PER_SEC == 1000);
  // Calculate the time difference in milliseconds
  time_taken = (uint64_t)(end_time - start_time);

  printf("Size Compressed: %d\n", main_cmprss_size);
  printf("Compress Time Taken: %dms\n", time_taken);

  start_time = clock();
  main_decmprss_size = byte_decompress(data_ptr, main_data_size);
  end_time = clock();

  // Calculate the time difference in milliseconds
  time_taken = (uint64_t)(end_time - start_time);

  printf("Size decompressed: %d\n", main_cmprss_size);
  printf("Decompress Time Taken: %dms\n", time_taken);

  return;
}