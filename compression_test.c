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
#define NIBBLE_NON_MATCH_BIT 0x8
#define NIBBLE_VALUE_MASK 0x7

typedef uint8_t buffer_element_t;

typedef union
{
  uint8_t byte;
  struct
  {
    uint8_t before : 4;
    uint8_t after : 4;
  };
} cmprss_token_t;

cmprss_token_t getMatchLen(buffer_element_t *data_ptr, uint8_t i)
{
  cmprss_token_t token1;
  token1.before = 0;
  token1.after = 0;

  if (data_ptr[i] == data_ptr[i + 1])
  {
    // find the number of consecutive matches after the currVal
    for (uint8_t j = 1; j < NIBBLE_NON_MATCH_BIT; j++)
    {
      if (data_ptr[i] == data_ptr[i + j])
        token1.after++;
      else
      {
        break;
      }
    }
  }
  else
  {
    // we should skip the first match, so we start at 2, since we don't what to include the final value in the case of a non-match length
    // find the number of consecutive non-matches after the currVal
    for (uint8_t j = 2; j < NIBBLE_NON_MATCH_BIT; j++)
    {
      if (data_ptr[i + (j - 1)] == data_ptr[i + j])
        break;
      else
      {
        token1.after++;
        token1.after = token1.after | NIBBLE_NON_MATCH_BIT;
      }
    }
  }

  return token1;
}

/**
 * @brief compresses a byte array of data using a custom algorithm
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
int byte_compress(buffer_element_t *data_ptr, uint64_t data_size)
{
  uint64_t size_after_compression = 0;
  uint64_t i = 0, j = 0;
  uint8_t prev_i = 0, currVal = 0, nextVal = 0, non_match = 0;
  uint8_t readIndex = 0, writeIndex = 0, buffReadIndex = 0, buffWritePtr = 0;
  cmprss_token_t token1;
  token1.before = NIBBLE_MAX;
  token1.after = NIBBLE_MAX;
  buffer_element_t buffer = 0xFF;

  {
    do
    {

      if (i >= data_size)
        break;

      // get 2x consecutive sets of match/non-match sequences and set them as the lengths in the token along with their match bits
      token1.before = getMatchLen(data_ptr, i).after;
      token1.after = getMatchLen(data_ptr, i + (token1.before & NIBBLE_VALUE_MASK)).after;

      // WARNING inserting a token will increase the size not reduce it
      if (((token1.before & NIBBLE_NON_MATCH_BIT) != 0) &&
          ((token1.after & NIBBLE_NON_MATCH_BIT) != 0) &&
          (writeIndex > i - 1))
      {
        // this failure mode may have corrupted the data due to the incomplete conversion
        // TODO create recovery method? or way to continue compressing? Perhaps make a buffer and shuffle all remaining bits outwards...
        size_after_compression = writeIndex;
        break;
      }

      if ((token1.before & NIBBLE_NON_MATCH_BIT) != 0)
      {
        writeIndex = writeIndex + (token1.before & NIBBLE_VALUE_MASK);
        // save the value from the space to be used by the token
        buffer = data_ptr[writeIndex];
      }
      else
      {
        writeIndex = i + 1;

        // no save required as value is matched to the previous one and can be ignored
      }

      // insert the token
      data_ptr[i + (token1.before & NIBBLE_VALUE_MASK)] = (buffer_element_t)token1.byte;

      if ((token1.after & NIBBLE_NON_MATCH_BIT) != 0)
      {
        writeIndex = writeIndex + (token1.after & NIBBLE_VALUE_MASK);
      }
      else
      {
        writeIndex = i + 1;
      }

      i = i + (token1.before & NIBBLE_NON_MATCH_BIT) + 1 + (token1.after & NIBBLE_NON_MATCH_BIT);
      buffer = 0xFF;
    } while (i < data_size);
  }
  size_after_compression = writeIndex;

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

  // TODO write 0xFF to all bytes larger than the post-compression size?

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

  // TODO print compressed array

  return;
}