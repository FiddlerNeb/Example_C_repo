#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "compression_test.h"
#include "test_arrays.h"

/**
 * @brief prints the input array to the console in a formatted fashion
 *
 * @param data_ptr
 * @param data_size
 */
void print_array(uint8_t *data_ptr, array_size_t data_size)
{
  printf("{");

  if (data_size > 256)
    data_size = 256;
  for (array_size_t k = 0; k < data_size; k++)
  {
    // start a new row for every PRINT_ROW_SIZE bytes
    if ((k % PRINT_ROW_SIZE) == 0)
    {
      #if MARKDOWN_OUTPUT == 1
      printf("<br>");
      #endif
      printf("\n 0x%X, ", data_ptr[k]);
    }
    else
    {
      printf("0x%X, ", data_ptr[k]);
    }
  }
  if (data_size >= 256)
    printf("\nreached 256 byte print limit. more not printed...");

  printf("\n}\n");
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
}

/**
 * @brief determine if param arrays are equal
 *
 * @param data_ptr
 * @param data_size
 * @return bool
 */
uint8_t ArraysAreEqual(buffer_element_t *data_ptr1, buffer_element_t *data_ptr2, array_size_t data_size)
{
  uint8_t result = 0;
    // Compare elements one by one
    for (int i = 0; i < data_size; i++) {
        if (data_ptr1[i] != data_ptr2[i]) 
        {
            result = 0;
            break;
        }
        else
        {
          result = 1;
        }
    }

    return result;
} 

/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
uint8_t regression_test(buffer_element_t *input_data_ptr, array_size_t input_size)
{
  uint64_t main_cmprss_size = 0, main_decmprss_size = 0;
  uint8_t compressed_data_ptr[MAX_INPUT_SIZE] = {0};
  uint8_t decompressed_data_ptr[MAX_INPUT_SIZE] = {0};

  if (input_size > MAX_INPUT_SIZE)
  {
    printf("input size too large for regression test, skipping");
    return 0;
  }

  memcpy(compressed_data_ptr, input_data_ptr, input_size);
  if (!ArraysAreEqual(input_data_ptr, compressed_data_ptr, input_size))
  {
    print_array(input_data_ptr, input_size);
    print_array(compressed_data_ptr, input_size);
    printf("copy error");
    return 0;
  }
  main_cmprss_size = byte_compress(compressed_data_ptr, input_size);

  if (main_cmprss_size < input_size)
  {
    main_decmprss_size = byte_decompress(decompressed_data_ptr, MAX_INPUT_SIZE, compressed_data_ptr, main_cmprss_size);
  }  
  else
  {
    memcpy(decompressed_data_ptr, input_data_ptr, input_size);
    printf("uncompressible, skipping\n");
  }

  if (!ArraysAreEqual(input_data_ptr, decompressed_data_ptr, input_size))
  {
    print_array(input_data_ptr, input_size);
    printf("input size: %d\n", input_size);
    print_array(compressed_data_ptr, main_cmprss_size);
    printf("compressed size: %d\n", main_cmprss_size);
    print_array(decompressed_data_ptr, main_decmprss_size);
    printf("decompressed size: %d\n", main_decmprss_size);
    printf("test fail");
    return 0;
  }

  return 1;
}

int run_verbose_compression_test(buffer_element_t *data_ptr, array_size_t data_size)
{
  clock_t start_time, end_time;
  uint64_t time_taken = 0.0f;

  uint8_t decompressed_data_ptr[MAX_INPUT_SIZE] = {0};
  uint64_t main_data_size = data_size;
  uint64_t main_cmprss_size = main_data_size;
  uint64_t main_decmprss_size = main_data_size;
  #if MARKDOWN_OUTPUT == 1
  printf("<code>");
  #endif
  printf("\n\nInitialization complete\n");
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
  printf("Size before: %d\n", main_data_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
  print_array(data_ptr, main_data_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif


  start_time = clock();
  main_cmprss_size = byte_compress(data_ptr, main_data_size);
  end_time = clock();

  // TODO write 0xFF to all bytes larger than the post-compression size?

  // check that clock ticks are indeed seconds
  assert(CLOCKS_PER_SEC == 1000);
  // Calculate the time difference in milliseconds
  time_taken = (uint64_t)(end_time - start_time);

  printf("\nSize Compressed: %d\n", main_cmprss_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
  printf("Compress Time Taken: %dms\n", time_taken);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
  print_array(data_ptr, main_cmprss_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif

  start_time = clock();
  if (data_size > MAX_INPUT_SIZE)
  {
    printf("decompression buffer not large enough to accept expected decompressed size");
    goto ERROR;
  }
  main_decmprss_size = byte_decompress(decompressed_data_ptr, main_data_size, data_ptr, main_cmprss_size);
  end_time = clock();

  // Calculate the time difference in milliseconds
  time_taken = (uint64_t)(end_time - start_time);

  printf("\nSize decompressed: %d\n", main_decmprss_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
  printf("Decompress Time Taken: %dms\n", time_taken);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
  print_array(decompressed_data_ptr, main_data_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
  #if MARKDOWN_OUTPUT == 1
  printf("</code>");
  #endif

  goto END;
  ERROR:
    printf("ERROR HAS OCCURRED");
  END:
}

/*
uint8_t input_data_ptr[INPUT_SIZE] = 

}

/**
 * @brief calls compress and decompress on a test sample of data and times it
 *
 */
void main(void)
{
  //(void)run_verbose_compression_test(input_data_ptr, 63);
  for (uint8_t i = 0;i< NUM_TESTS; i++)
  {
    printf("test: %d\n", i);
    if (!regression_test(test_arrays[i], array_sizes[i]))
        return;
  }

  printf("All tests Passed\n");

  return;
}