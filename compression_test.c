/**
 * @file compression_test_lib.c
 * @author Benjamin Odegaard (ben.odegaard10@gmail.com)
 * @brief a test compression implementation for technical interview
 * @version 0.2
 * @date 2025-09-15
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <assert.h>

#define MARKDOWN_OUTPUT 1

#define INPUT_SIZE 24
#define BUFFER_SIZE 64
#define TOKEN_INIT 0
#define NIBBLE_MAX 0xF
#define NIBBLE_NON_MATCH_BIT 0x8
#define NIBBLE_VALUE_MASK 0x7
#define PRINT_ROW_SIZE 8
#define ERASED_BYTE 0xFF

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

/**
 * @brief prints the input array to the console in a formatted fashion
 *
 * @param data_ptr
 * @param data_size
 */
void print_array(uint8_t *data_ptr, array_size_t data_size)
{
  printf("{");
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

  printf("\n}\n");
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif
}

cmprss_token_t getMatchLen(buffer_element_t *data_ptr, array_size_t i, array_size_t data_size)
{
  cmprss_token_t token1;
  token1.before = 0;
  token1.after = 0;

  if (data_ptr[i] != ERASED_BYTE)
  {
    if (data_ptr[i] == data_ptr[i + 1])
    {
      
      // original match
      token1.after++;
      // find the number of consecutive matches after the currVal
      for (uint8_t j = 1; j < NIBBLE_NON_MATCH_BIT-1; j++)
      {
        if ((i+j)> data_size)
          break;
        
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
      for (uint8_t j = 1; j < NIBBLE_NON_MATCH_BIT; j++)
      {
        if ((i+j)> data_size)
          break;
        if (data_ptr[i] == ERASED_BYTE)
          break;
        if (data_ptr[i + (j - 1)] == data_ptr[i + j])
          break;
        else
        {
          token1.after++;
          token1.after = token1.after | NIBBLE_NON_MATCH_BIT;
        }
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
int estimate_array_size(buffer_element_t *data_ptr, array_size_t data_size)
{
  array_size_t i = 0;
  cmprss_token_t token1;
  token1.before = NIBBLE_MAX;
  token1.after = NIBBLE_MAX;
  array_size_t itterationCount = data_size*2;
  array_size_t cmprss_size_est = 0;

  while ((i < data_size) && (data_ptr[i] != ERASED_BYTE))
  {
    //prefent infinite loop
    itterationCount--;
    if (itterationCount <=1)
      break;

    // get 2x consecutive sets of match/non-match sequences and set them as the lengths in the token along with their match bits
    token1.before = getMatchLen(data_ptr, i, data_size).after;
    token1.after = getMatchLen(data_ptr, i + (token1.before & NIBBLE_VALUE_MASK), data_size).after;

    if ((token1.before & NIBBLE_NON_MATCH_BIT) != 0)
    {
      cmprss_size_est = cmprss_size_est + (token1.before & NIBBLE_VALUE_MASK) + 1;
      
    }
    else
    {
      cmprss_size_est = cmprss_size_est + 1;
    }
    i = i + (token1.before & NIBBLE_VALUE_MASK) + 1;

    if ((token1.after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      cmprss_size_est = cmprss_size_est + (token1.after & NIBBLE_VALUE_MASK) + 1;
    }
    else
    {
      cmprss_size_est = cmprss_size_est + 1;
    }
    i = i + (token1.after & NIBBLE_VALUE_MASK) + 1;

    //for the token
    cmprss_size_est = cmprss_size_est + 1;
  }

  return cmprss_size_est;
}

//#define DEBUG 1
/**
 * @brief compresses a byte array of data using a custom algorithm
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
int byte_compress(buffer_element_t *data_ptr, array_size_t data_size)
{
  array_size_t size_after_compression = 0;
  array_size_t i = 0;
  uint8_t writeIndex = 0, eofWriteIndex = BUFFER_SIZE-1;
  cmprss_token_t token1, prevToken;
  token1.before = NIBBLE_MAX;
  token1.after = NIBBLE_MAX;
  prevToken.before = NIBBLE_MAX;
  prevToken.after = NIBBLE_MAX;
  buffer_element_t buffer = ERASED_BYTE, buffer2 = ERASED_BYTE;
  buffer_element_t eofBuffer[BUFFER_SIZE];
  memset(eofBuffer, ERASED_BYTE, BUFFER_SIZE);
  uint64_t itterationCount = data_size*2;

  if (estimate_array_size(data_ptr, data_size) >= data_size)
  {
    //likely uncompressible via this method, abort
    return data_size;
  }



  while ((i < data_size) && (data_ptr[i] != ERASED_BYTE))
  {
    //prefent infinite loop
    itterationCount--;
    if (itterationCount <=1)
      break;

    // get 2x consecutive sets of match/non-match sequences and set them as the lengths in the token along with their match bits
    token1.before = getMatchLen(data_ptr, i, data_size).after;
    token1.after = getMatchLen(data_ptr, i + (token1.before & NIBBLE_VALUE_MASK), data_size).after;

    if ((token1.before == 0) && (token1.after == 0))
      break;
  
    //check if we're about to reach the end of the buffer
    if (data_size < (i+(token1.before & NIBBLE_VALUE_MASK)+1+(token1.after & NIBBLE_VALUE_MASK)))
    {
      if ((writeIndex+((data_size)-(i))) < data_size)
      {
        if (data_ptr[i] == ERASED_BYTE)
          break;
        else if (eofWriteIndex+1 < BUFFER_SIZE)
        {
          //eofBuffer[eofWriteIndex--] = data_ptr[data_size-1];
          memmove(&data_ptr[writeIndex], &data_ptr[i], ((data_size)-(i)));
          
          #ifdef DEBUG
          print_array(data_ptr, data_size);
          #endif
          memmove(&data_ptr[writeIndex+((data_size)-(i))], &eofBuffer[eofWriteIndex+1], BUFFER_SIZE - (eofWriteIndex+1));
          
          #ifdef DEBUG
          print_array(data_ptr, data_size);
          #endif
          memset(&data_ptr[writeIndex+(BUFFER_SIZE - (eofWriteIndex+1))+((data_size)-(i))], 0xFF, ((data_size)-(writeIndex+(BUFFER_SIZE - (eofWriteIndex+1))+((data_size)-(i)))));
          eofWriteIndex = BUFFER_SIZE;
          i = writeIndex; 
          #ifdef DEBUG
          print_array(data_ptr, data_size);
          #endif
          buffer = ERASED_BYTE;
          continue;
        }
      }
      else if (((token1.before & NIBBLE_NON_MATCH_BIT) == 0) && 
               (i + (token1.before) < (data_size-1)))
      {
        //let the algorithm continue at least once to try and compress some more space last minute
      }
      else
      {
        break;
      }
    }

    if ((prevToken.after & NIBBLE_NON_MATCH_BIT) != 0)// && ((i == 0) || (token1.after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      //if we start off with a non matched streak, we need to put in a token at the beginning of the file
      if ((i != 0) && ((token1.before & NIBBLE_NON_MATCH_BIT) == 0) || 
          (i == 0) && ((token1.before & NIBBLE_NON_MATCH_BIT) != 0))
      {
        //add a token in to indicate the end of the non-matching characters
        token1.after = token1.before;
        token1.before = NIBBLE_NON_MATCH_BIT;
        // save the value from the space to be used by the token
        buffer = data_ptr[writeIndex];
      }
      else if (i==0)
      {
        //matching bit at the beginning of the array, move on
      }
      else if (token1.after == 0)
      {
        //the unmatching before bytes go to the end of the file, we should add a token and allow the compression to terminate
      }
      else
      {
        //we have non-matching characters which need to be continued
        memmove(&data_ptr[writeIndex], &data_ptr[i], (token1.before & NIBBLE_VALUE_MASK));
        writeIndex = writeIndex + (token1.before & NIBBLE_VALUE_MASK);
        i = i + (token1.before & NIBBLE_VALUE_MASK);
        buffer = ERASED_BYTE;
        //prevToken.byte = token1.byte;
        #ifdef DEBUG
        print_array(data_ptr, data_size);
        #endif
        continue;
      }
    }
    

    // WARNING inserting a token will increase the size not reduce it
    if (((token1.before & NIBBLE_NON_MATCH_BIT) != 0) &&
        ((token1.after & NIBBLE_NON_MATCH_BIT) != 0) &&
        (writeIndex > (i - 1)))
    {
      // If we quit here, this failure mode will have corrupted the data due to the incomplete conversion
      // TODO create recovery method? or way to continue compressing? Perhaps make a buffer and shuffle all remaining bits outwards...
      size_after_compression = (array_size_t)-1;
      break;
    }

    if ((token1.before & NIBBLE_NON_MATCH_BIT) != 0)
    {
      if ((prevToken.after & NIBBLE_NON_MATCH_BIT) == 0)
      {
        //to go from matched to unmatched on a non-token boundry, we need to fake a match to move the boundry to the correct spot for decoding
        data_ptr[writeIndex++] = data_ptr[i++];
        token1.before = token1.before -1;
        data_ptr[writeIndex++] = 0x10 + token1.before;
        
      }
      memmove(&data_ptr[writeIndex], &data_ptr[i], (token1.before & NIBBLE_VALUE_MASK));
      writeIndex = writeIndex + (token1.before & NIBBLE_VALUE_MASK);
      #ifdef DEBUG
      print_array(data_ptr, data_size);
      #endif
      if ((prevToken.after & NIBBLE_NON_MATCH_BIT) == 0)
      {
        i = i + (token1.before & NIBBLE_VALUE_MASK);
        buffer = ERASED_BYTE;
        prevToken.byte = 0x10 + token1.before;
        continue;
      }
      // save the value from the space to be used by the token
      buffer = data_ptr[writeIndex];

      
    }
    else
    {
      // insert the before match value
      data_ptr[writeIndex] = data_ptr[i];
      writeIndex = writeIndex + 1;

      // save the value from the space to be used by the token
      buffer = data_ptr[writeIndex];
    }
    #ifdef DEBUG
    print_array(data_ptr, data_size);
    #endif
    i = i + (token1.before & NIBBLE_VALUE_MASK);

    // insert the token
    data_ptr[writeIndex++] = (buffer_element_t)token1.byte;

    #ifdef DEBUG
    print_array(data_ptr, data_size);
    #endif

    if (token1.after == 0)
      break;

    if ((token1.after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      if (buffer != ERASED_BYTE)
      {
        if ((writeIndex + (token1.after & NIBBLE_VALUE_MASK)) < i)
        {
          memmove(&data_ptr[writeIndex+1], &data_ptr[writeIndex], (token1.after & NIBBLE_VALUE_MASK));
          data_ptr[writeIndex] = buffer;
          #ifdef DEBUG
          print_array(data_ptr, data_size);
          #endif
        }
        else
        {
          //problem case where we've not got enough space to insert the token
          eofBuffer[eofWriteIndex--] = data_ptr[data_size-1];
          memmove(&data_ptr[writeIndex+1], &data_ptr[writeIndex], ((data_size-1)-(writeIndex)));
          data_ptr[writeIndex] = buffer;
          #ifdef DEBUG
          print_array(data_ptr, data_size);
          #endif
          i = i + 1;
        }
      }

      memmove(&data_ptr[writeIndex], &data_ptr[i], (token1.after & NIBBLE_VALUE_MASK));
      writeIndex = writeIndex + (token1.after & NIBBLE_VALUE_MASK);
    }
    else
    {
      // insert the before match value
      data_ptr[writeIndex] = data_ptr[i+1];
      writeIndex = writeIndex + 1;
    }
    #ifdef DEBUG
    print_array(data_ptr, data_size);
    #endif

    i = i + (token1.after & NIBBLE_VALUE_MASK);

    //in case the unmatch token1.after goes to the end of the array, add one last token to delimit the file.
    if ((i > data_size) && (token1.after & NIBBLE_NON_MATCH_BIT) > 0)
    {
      token1.before = NIBBLE_NON_MATCH_BIT;
      token1.after = 0;
      data_ptr[writeIndex++] = token1.byte;
    }

    buffer = ERASED_BYTE;
    prevToken.byte = token1.byte;
  } 

  size_after_compression = writeIndex;

  return size_after_compression;
}

/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
int byte_decompress(buffer_element_t *uncmprss_data_ptr, array_size_t uncmprss_data_size, buffer_element_t *cmprss_data_ptr, array_size_t cmpress_data_size)
{
  array_size_t size_after_compression = 0;
  array_size_t i = 0;



  return size_after_compression;
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
uint8_t regression_test_basic(void)
{
  uint64_t main_cmprss_size = 0;
  uint8_t input_data_ptr[INPUT_SIZE] = {0x03, 0x74, 0x04, 0x04, 0x04, 0x35, 0x35, 0x64,
                                         0x64, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x56, 0x45, 0x56, 0x56, 0x56, 0x09, 0x09, 0x09};
  uint8_t answer_data_ptr[INPUT_SIZE] = { 0x8A, 0x3, 0x74, 0x83, 0x4, 0x35, 0x24, 0x64,
                                          0x0, 0x5A, 0x56, 0x45, 0x83, 0x56, 0x9, 0x30};

  main_cmprss_size = byte_compress(input_data_ptr, INPUT_SIZE);

  if (!ArraysAreEqual(input_data_ptr, answer_data_ptr, main_cmprss_size))
  {
    print_array(input_data_ptr, INPUT_SIZE);
    print_array(answer_data_ptr, main_cmprss_size);
    printf("compressed size: %d\n", main_cmprss_size);
    printf("test_basic fail");
    return 0;
  }

  return 1;
}

/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
uint8_t regression_test_pairs(void)
{
  uint64_t main_cmprss_size = 0;
  uint8_t input_data_ptr[INPUT_SIZE] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD,
                                         0xEE, 0xEE, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33,
                                         0x44, 0x44, 0x66, 0x77, 0x88, 0x99, 0x00, 0x00};
  uint8_t answer_data_ptr[INPUT_SIZE] = { 0xAA, 0x22, 0xBB, 0xCC, 0x22, 0xDD, 0xEE, 0x22,
                                          0x11, 0x22, 0x22, 0x33, 0x44, 0x2C, 0x66, 0x77,
                                          0x88, 0x99, 0x82, 0x0,};

  main_cmprss_size = byte_compress(input_data_ptr, INPUT_SIZE);

  if (!ArraysAreEqual(input_data_ptr, answer_data_ptr, main_cmprss_size))
  {
    print_array(input_data_ptr, INPUT_SIZE);
    print_array(answer_data_ptr, main_cmprss_size);
    printf("compressed size: %d\n", main_cmprss_size);
    printf("test_pairs fail");
    return 0;
  }

  return 1;
}

/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
uint8_t regression_test_matched_unmatched(void)
{
  uint64_t main_cmprss_size = 0;
  uint8_t input_data_ptr[INPUT_SIZE] = {
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0x1C, 0x2D, 0x3E, 0x4F,
    0x50, 0x61, 0x72, 0x83, 0x94, 0xA5, 0xB6, 0xC7
};
  uint8_t answer_data_ptr[INPUT_SIZE] = {
 0xAA, 0x75, 0xAA, 0x1C, 0x1E, 0x2D, 0x3E, 0x4F, 
 0x50, 0x61, 0x72, 0x83, 0x94, 0xA5, 0xB6, 0xC7,
 0xD0,
};

  main_cmprss_size = byte_compress(input_data_ptr, INPUT_SIZE);

  if (!ArraysAreEqual(input_data_ptr, answer_data_ptr, main_cmprss_size))
  {
    print_array(input_data_ptr, INPUT_SIZE);
    print_array(answer_data_ptr, main_cmprss_size);
    printf("compressed size: %d\n", main_cmprss_size);
    printf("matched_unmatched fail");
    return 0;
  }

  return 1;
}

/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
uint8_t regression_test_unmatched_matched(void)
{
  uint64_t main_cmprss_size = 0;
  uint8_t input_data_ptr[INPUT_SIZE] = {
    0x1C, 0x2D, 0x3E, 0x4F, 0x50, 0x61, 0x72, 0x83, 
    0x94, 0xA5, 0xB6, 0xC7, 0xAA, 0xAA, 0xAA, 0xAA, 
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA
};
  uint8_t answer_data_ptr[INPUT_SIZE] = {
 0x8F, 0x1C, 0x2D, 0x3E, 0x4F, 0x50, 0x61, 0x72,
 0x83, 0x94, 0xA5, 0xB6, 0xC7, 0x87, 0xAA, 0xAA,
 0x50,
};

  main_cmprss_size = byte_compress(input_data_ptr, INPUT_SIZE);

  if (!ArraysAreEqual(input_data_ptr, answer_data_ptr, main_cmprss_size))
  {
    
    print_array(input_data_ptr, INPUT_SIZE);
    print_array(answer_data_ptr, main_cmprss_size);
    printf("compressed size: %d\n", main_cmprss_size);
    printf("unmatched_matched fail");
    return 0;
  }

  return 1;
}

/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
uint8_t regression_test_all_unmatched(void)
{
  uint64_t main_cmprss_size = 0;
  uint8_t input_data_ptr[INPUT_SIZE] =  {
    0x3A, 0x7F, 0xC2, 0x1D, 0xE4, 0x56,
    0xA9, 0x08, 0xBE, 0x42, 0x99, 0x6C,
    0xF1, 0x2E, 0x75, 0xD3, 0x11, 0x84,
    0x5B, 0x67, 0x90, 0xAD, 0x38, 0xCB
};
  uint8_t answer_data_ptr[INPUT_SIZE] =  {
    0x3A, 0x7F, 0xC2, 0x1D, 0xE4, 0x56,
    0xA9, 0x08, 0xBE, 0x42, 0x99, 0x6C,
    0xF1, 0x2E, 0x75, 0xD3, 0x11, 0x84,
    0x5B, 0x67, 0x90, 0xAD, 0x38, 0xCB
};

  main_cmprss_size = byte_compress(input_data_ptr, INPUT_SIZE);

  if (!ArraysAreEqual(input_data_ptr, answer_data_ptr, main_cmprss_size))
  {
    print_array(input_data_ptr, INPUT_SIZE);
    print_array(answer_data_ptr, main_cmprss_size);
    printf("compressed size: %d\n", main_cmprss_size);
    printf("all unmatched fail");
    return 0;
  }

  return 1;
}

/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
uint8_t regression_test_len25(void)
{
  uint64_t main_cmprss_size = 0;
  uint8_t input_data_ptr[INPUT_SIZE] =  {
    0x1A, 0x1A, 0x2B, 0x2B, 0x2B,
    0x3C, 0x3C, 0x4D, 0x4D, 0x4D,
    0x5E, 0x5E, 0x6F, 0x6F, 0x70,
    0x11, 0x22, 0x33, 0x44, 0x55,
    0x66, 0x12, 0x24, 0x36, 0x48
};
  uint8_t answer_data_ptr[INPUT_SIZE] =  {
 0x1A, 0x23, 0x2B, 0x3C, 0x23, 0x4D, 0x5E, 0x22,
 0x6F, 0x70, 0x1E, 0x11, 0x22, 0x33, 0x44, 0x55,
 0x66, 0x12, 0x24, 0x36, 0x48, 0xC0,
};

  main_cmprss_size = byte_compress(input_data_ptr, INPUT_SIZE);

  if (!ArraysAreEqual(input_data_ptr, answer_data_ptr, main_cmprss_size))
  {
    print_array(input_data_ptr, INPUT_SIZE);
    print_array(answer_data_ptr, main_cmprss_size);
    printf("compressed size: %d\n", main_cmprss_size);
    printf("len 25 fail");
    return 0;
  }

  return 1;
}

int run_verbose_compression_test(buffer_element_t *data_ptr, array_size_t data_size)
{
  clock_t start_time, end_time;
  uint64_t time_taken = 0.0f;

  uint8_t decompressed_data_ptr[INPUT_SIZE] = {0};
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
  main_decmprss_size = byte_decompress(data_ptr, main_data_size, decompressed_data_ptr, main_cmprss_size);
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

  uint8_t input_data_ptr[63] =  {
    0x1A, 0x1A, 0x2B, 0x2B, 0x2B,
    0x3C, 0x3C, 0x4D, 0x4D, 0x4D,
    0x5E, 0x5E, 0x6F, 0x6F, 0x6F,
    0x11, 0x11, 0x22, 0x22, 0x22,
    0x33, 0x33, 0x44, 0x44, 0x44,
    0x55, 0x55, 0x66, 0x66, 0x66,
    0x10, 0x20, 0x30, 0x40, 0x50,
    0x60, 0x70, 0x71, 0x72, 0x73,
    0x13, 0x23, 0x34, 0x45, 0x57,
    0x14, 0x25, 0x36, 0x47, 0x58,
    0x15, 0x26, 0x37, 0x48, 0x59,
    0x16, 0x27, 0x38, 0x49, 0x5A,
    0x17, 0x28, 0x39
};

  (void)run_verbose_compression_test(input_data_ptr, 63);

  if (!regression_test_basic())
    return;
  if (!regression_test_pairs())
    return;
  if (!regression_test_matched_unmatched())
    return;
  if (!regression_test_unmatched_matched())
    return;
  if (!regression_test_all_unmatched())
    return;
  if (!regression_test_len25())
    return;

  printf("All tests Passed\n");

  return;
}