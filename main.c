#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "compression_test.h"
#include "test_arrays.h"

// #define DEBUG 1

/**
 * @brief prints the input array to the console in a formatted fashion
 *
 * @param data_ptr
 * @param data_size
 */
void print_array(uint8_t *data_ptr, array_size_t data_size, char *array_name)
{
  printf("{ //%s", array_name);

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
  for (int i = 0; i < data_size; i++)
  {
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
array_size_t regression_test(buffer_element_t *input_data_ptr, array_size_t input_size)
{
  array_size_t main_cmprss_size = 0, main_decmprss_size = 0;
  uint8_t compressed_data_ptr[MAX_INPUT_SIZE] = {0};
  uint8_t decompressed_data_ptr[MAX_INPUT_SIZE] = {0};

  if (input_size > MAX_INPUT_SIZE)
  {
    printf("input size too large for regression test, skipping");
    return FAIL;
  }

  for (array_size_t i = 0; i < input_size; i++)
  {
    if (input_data_ptr[i] > MAX_NON_TOKEN_DATA)
    {
      print_array(input_data_ptr, input_size, GET_VAR_NAME(input_data_ptr));
      printf("\ninput array has a >127 value at index %d\n", i);
      return FAIL;
    }
  }

  if (input_size > 0)
  {
    memcpy(compressed_data_ptr, input_data_ptr, input_size);
    if (!ArraysAreEqual(input_data_ptr, compressed_data_ptr, input_size))
    {
      print_array(input_data_ptr, input_size, GET_VAR_NAME(input_data_ptr));
      print_array(compressed_data_ptr, input_size, GET_VAR_NAME(compressed_data_ptr));
      printf("copy error");
      return FAIL;
    }
  }
  main_cmprss_size = byte_compress(compressed_data_ptr, input_size);

  if (main_cmprss_size < input_size)
  {
    main_decmprss_size = byte_decompress(decompressed_data_ptr, MAX_INPUT_SIZE, compressed_data_ptr, main_cmprss_size);
  }
  else
  {
    memcpy(decompressed_data_ptr, input_data_ptr, input_size);
#ifdef DEBUG
    printf("size %d vs est cmpress %d\n", input_size, estimate_array_size(input_data_ptr, input_size));
    printf("uncompressible, skipping\n");
#endif
  }

  if (input_size > 0)
  {
    if (!ArraysAreEqual(input_data_ptr, decompressed_data_ptr, input_size))
    {
      print_array(input_data_ptr, input_size, GET_VAR_NAME(input_data_ptr));
      printf("input size: %d\n", input_size);
      print_array(compressed_data_ptr, main_cmprss_size, GET_VAR_NAME(compressed_data_ptr));
      printf("compressed size: %d\n", main_cmprss_size);
      print_array(decompressed_data_ptr, main_decmprss_size, GET_VAR_NAME(decompressed_data_ptr));
      printf("decompressed size: %d\n", main_decmprss_size);
      printf("test fail\n\n\n");
      return FAIL;
    }
#ifdef DEBUG
    printf("Regression Test Compression%%: %d%%\n", ((main_cmprss_size * 100) / input_size));
#endif
  }

  return (main_cmprss_size);
}

ret_code fill_array_randomly_and_randomize_size(buffer_element_t *data_ptr, array_size_t *data_size, array_size_t match_max_length, array_size_t max_length)
{
  buffer_element_t val = ERASED_BYTE;
  array_size_t length = 0;
  uint8_t matched;

  if (match_max_length > MAX_INPUT_SIZE)
    match_max_length = MAX_INPUT_SIZE;

  if (max_length > MAX_INPUT_SIZE)
    max_length = MAX_INPUT_SIZE;

  for (array_size_t i = 0; i < MAX_INPUT_SIZE; i += length)
  {
    // random length of matched/unmatched sequence
    length = rand() % match_max_length;

    if ((length + i) > MAX_INPUT_SIZE)
      length = MAX_INPUT_SIZE - i;

    // random choise of unmatched matched
    matched = rand() % 2;

    if (matched)
    {
      //  random int between 0 and MAX_NON_TOKEN_DATA
      val = rand() % MAX_NON_TOKEN_DATA;

      memset(&data_ptr[i], val, length);
    }
    else
    {
      for (array_size_t j = 0; j < length; j++)
      {
        //  random int between 0 and MAX_NON_TOKEN_DATA
        val = rand() % MAX_NON_TOKEN_DATA;
        memset(&data_ptr[i + j], val, 1);
      }
    }
  }

  // random length to trim array
  *data_size = rand() % max_length;
#ifdef DEBUG
  print_array(data_ptr, *data_size, GET_VAR_NAME(data_ptr));
#endif
  return PASS;
}

ret_code run_random_tests(array_size_t num_tests, array_size_t match_max_length, array_size_t max_length)
{
  uint8_t data_ptr[MAX_INPUT_SIZE] = {0};
  array_size_t data_size = 0;
  clock_t start_time, end_time;
  uint64_t time_taken = 0.0f;
  array_size_t cmpress_size = 0;
  uint16_t ave_cmpress_perc = 0, cmpress_perc = 0, count_less20 = 0, count_less40 = 0, count_less60 = 0, count_less70 = 0, count_less80 = 0, count_less90 = 0, count_more90 = 0;
  uint16_t time_countless1 = 0, time_countless3 = 0, time_countless6 = 0, time_countless10 = 0, time_countless20 = 0, time_countmore20 = 0;
  uint16_t ave_time = 0;

  for (array_size_t i = 0; i < num_tests; i++)
  {
    (void)fill_array_randomly_and_randomize_size(data_ptr, &data_size, match_max_length, max_length);

    start_time = clock();
    // cmpress_size = regression_test(test_arrays[i], array_sizes[i]);
    cmpress_size = regression_test(data_ptr, data_size);

    if ((cmpress_size > array_sizes[i]) && (cmpress_size <= 0))
      return FAIL;
    else
    {
#ifdef DEBUG
      printf("PASS\n\n");
#endif
    }
    end_time = clock();

    assert(CLOCKS_PER_SEC == 1000);
    // Calculate the time difference in milliseconds
    time_taken = (uint64_t)(end_time - start_time);
#ifdef DEBUG
    printf("Regression Test Time Taken: %dms\n", time_taken);
#endif // DEBUG
    if (cmpress_size < data_size)
    {
      if (time_taken < 1)
        time_countless1++;
      else if (time_taken < 3)
        time_countless3++;
      else if (time_taken < 6)
        time_countless6++;
      else if (time_taken < 10)
        time_countless10++;
      else if (time_taken < 20)
        time_countless20++;
      else
        time_countmore20++;

      ave_time = (uint64_t)(time_countless1 * 1 + time_countless3 * 3 + time_countless6 * 6 + time_countless10 * 10 + time_countless20 * 20 + time_countmore20 * 30) /
                 (time_countless1 + time_countless3 + time_countless6 + time_countless10 + time_countless20 + time_countmore20);
#ifdef DEBUG
      printf("<1x%d <3x%d <6x%d <10x%d <20x%d >20x%d \n", time_countless1, time_countless3, time_countless6, time_countless10, time_countless20, time_countmore20);
      printf("ave time: %dms\n", ave_time);
#endif // DEBUG
      cmpress_perc = (cmpress_size * 100 / data_size);

      if (cmpress_perc < 20)
        count_less20++;
      else if (cmpress_perc < 40)
        count_less40++;
      else if (cmpress_perc < 60)
        count_less60++;
      else if (cmpress_perc < 70)
        count_less70++;
      else if (cmpress_perc < 80)
        count_less80++;
      else if (cmpress_perc < 90)
        count_less90++;
      else
        count_more90++;

      ave_cmpress_perc = ((uint64_t)((count_less20 * 20) + (count_less40 * 40) + (count_less60 * 60) + (count_less70 * 70) + (count_less80 * 80) + (count_less90 * 90) + (count_more90 * 95)) /
                          (count_less20 + count_less40 + count_less60 + count_less70 + count_less80 + count_less90 + count_more90));
#ifdef DEBUG
      printf("<20x%d <40x%d <60x%d <70x%d <80x%d <90x%d <95x%d \n", count_less20, count_less40, count_less60, count_less70, count_less80, count_less90, count_more90);
      printf("ave cmpress %%: %d%%\n", ave_cmpress_perc);
#endif // DEBUG
    }
  }
  printf("ave cmpress %%: %d%%\n", ave_cmpress_perc);
  printf("ave time: %dms\n", ave_time);

  return PASS;
}

ret_code run_verbose_compression_test(buffer_element_t *data_ptr, array_size_t data_size)
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
  print_array(data_ptr, main_data_size, GET_VAR_NAME(data_ptr));
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
  print_array(data_ptr, main_cmprss_size, GET_VAR_NAME(data_ptr));
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
  print_array(decompressed_data_ptr, main_data_size, GET_VAR_NAME(decompressed_data_ptr));
#if MARKDOWN_OUTPUT == 1
  printf("<br>");
#endif
#if MARKDOWN_OUTPUT == 1
  printf("</code>");
#endif

  goto END;
ERROR:
  printf("ERROR HAS OCCURRED");

  return FAIL;
END:
  return PASS;
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
  array_size_t cmpress_size = 0;
  // debug
  //(void)regression_test(test_arrays[16], array_sizes[16]);
  // end debug

  // corner case tests
  for (uint8_t i = NUM_TESTS - 1; i > 0; i--)
  {
#ifdef DEBUG
    printf("\n\ntest: %d\n", i);
#endif
    cmpress_size = regression_test(test_arrays[i], array_sizes[i]);
    // if (run_verbose_compression_test(test_arrays[i], array_sizes[i]) != PASS)
    if ((cmpress_size > array_sizes[i]) || (cmpress_size <= 0))
    {
      goto ERROR;
    }
    else
    {
#ifdef DEBUG
      printf("PASS\n", i);
#endif
    }
  }
  printf("Max match size %d\n", 4);
  if (run_random_tests(1000, 4, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 8);
  if (run_random_tests(1000, 8, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 12);
  if (run_random_tests(1000, 12, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 16);
  if (run_random_tests(1000, 16, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 20);
  if (run_random_tests(1000, 20, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 24);
  if (run_random_tests(1000, 24, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 28);
  if (run_random_tests(1000, 28, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 32);
  if (run_random_tests(1000, 32, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 36);
  if (run_random_tests(1000, 36, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 40);
  if (run_random_tests(1000, 40, MAX_INPUT_SIZE) != PASS)
    goto ERROR;
  printf("Max match size %d\n", 44);
  if (run_random_tests(1000, 44, MAX_INPUT_SIZE) != PASS)
    goto ERROR;

  goto END;
ERROR:
  printf("ERROR HAS OCCURRED");
  return;
END:
  printf("All tests Passed\n");

  return;
}