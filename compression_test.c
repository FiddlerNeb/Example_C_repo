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
        prevToken.byte = token1.byte;
        continue;
      }
    }

    if ((prevToken.after & NIBBLE_NON_MATCH_BIT) != 0)
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
        prevToken.byte = token1.byte;
        #ifdef DEBUG
        print_array(data_ptr, data_size);
        #endif
        continue;
      }
    }
    else if ((token1.before & NIBBLE_NON_MATCH_BIT) != 0)
    

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
 * @brief calls compress and decompress on a test sample of data and times it
 *
 */
void main(void)
{
  clock_t start_time, end_time;
  uint64_t time_taken = 0.0f;
  /*
  uint8_t input_data_ptr[INPUT_SIZE] = {0x03, 0x74, 0x04, 0x04, 0x04, 0x35, 0x35, 0x64,
                                         0x64, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x56, 0x45, 0x56, 0x56, 0x56, 0x09, 0x09, 0x09};
  */
  /*
  uint8_t input_data_ptr[INPUT_SIZE] = {
    0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD,
    0xEE, 0xEE, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33,
    0x44, 0x44, 0x66, 0x77, 0x88, 0x99, 0x00, 0x00
};
//  correct compression

    0xAA, 0x22, 0xBB, 0xCC, 0x22, 0xDD, 0xEE, 0x22,
    0x11, 0x22, 0x22, 0x33, 0x44, 0x2C, 0x66, 0x77, 
    0x88, 0x99, 0x82, 0x00
};
*/
uint8_t input_data_ptr[INPUT_SIZE] = {
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0x1C, 0x2D, 0x3E, 0x4F,
    0x50, 0x61, 0x72, 0x83, 0x94, 0xA5, 0xB6, 0xC7
};
/*
//  correct compression

};
*/
  // TODO  instead of having the decompressed size known ahead of time, we should use malloc or similar since all we would know is the compressed size
  uint8_t decompressed_data_ptr[INPUT_SIZE] = {0};
  uint64_t main_data_size = INPUT_SIZE;
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
  print_array(input_data_ptr, main_data_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif


  start_time = clock();
  main_cmprss_size = byte_compress(input_data_ptr, main_data_size);
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
  print_array(input_data_ptr, main_cmprss_size);
  #if MARKDOWN_OUTPUT == 1
  printf("<br>");
  #endif

  start_time = clock();
  main_decmprss_size = byte_decompress(input_data_ptr, main_data_size, decompressed_data_ptr, main_cmprss_size);
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
  #if MARKDOWN_OUTPUT == 1
  printf("</code>");
  #endif
  #endif

  // TODO print compressed array

  return;
}