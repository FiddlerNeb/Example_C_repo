#include <string.h>

#include "compression_test.h"

#define DEBUG 1
/**
 * @brief
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
int byte_decompress(buffer_element_t *uncmprss_data_ptr, array_size_t uncmprss_data_size, buffer_element_t *cmprss_data_ptr, array_size_t cmpress_data_size)
{
  array_size_t sizeAfterDecompression = 0;
  array_size_t i = 0, afterCopyIndex = 0, beforeCopyIndex = 0, readTokenIndex = 0, writeIndex = 0;
  cmprss_token_t curToken, nextToken;
  curToken.byte = ERASED_BYTE;
  nextToken.byte = ERASED_BYTE;
  uint16_t count = 0;

  //* The first token will always be either in the first or second byte of the compressed array.
  //      * If the first byte contains a value larger than 0x7F then the file starts with an unmatched string.
  //      * If not, then the second byte is your first token.
  if (cmprss_data_ptr[0] > MAX_NON_TOKEN_DATA)
  {
    curToken.byte = cmprss_data_ptr[0];
    readTokenIndex = 0;
  }
  else
  {
    curToken.byte = cmprss_data_ptr[1];
    readTokenIndex = 1;
  }

  while ((readTokenIndex < cmpress_data_size) && (cmprss_data_ptr[readTokenIndex] != ERASED_BYTE))
  {
    if (writeIndex >= uncmprss_data_size)
    {
      // error, uncmprss_array not large enough to hold uncmprss data
      writeIndex = 0;
      break;
    }

    if (count > (uncmprss_data_size + 1))
    {
      break;
    }
    count++;
    beforeCopyIndex = readTokenIndex - 1;

    if ((curToken.before & NIBBLE_NON_MATCH_BIT) != 0)
    {
      // For an unmatched string,
      // identify the next token and the end of the unmatched length and copy the unmatched length to the output without modification
      // readTokenIndex should be at a token

      memmove(&uncmprss_data_ptr[writeIndex], &cmprss_data_ptr[afterCopyIndex], ((readTokenIndex) - (afterCopyIndex)));
      writeIndex += ((readTokenIndex) - (afterCopyIndex));
    }
    else
    {
      // For a matched string,
      // duplicate the next byte N times into the decompressed array.
      memset(&uncmprss_data_ptr[writeIndex], cmprss_data_ptr[beforeCopyIndex], (curToken.before & NIBBLE_VALUE_MASK));
      writeIndex += (curToken.before & NIBBLE_VALUE_MASK);
    }
#ifdef DEBUG
    print_array(uncmprss_data_ptr, writeIndex, GET_VAR_NAME(uncmprss_data_ptr));
#endif

    afterCopyIndex = readTokenIndex + 1;
    if ((curToken.after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      // For an unmatched string,
      // Identify the start of the non-matching set
      readTokenIndex += curToken.after & NIBBLE_VALUE_MASK;

      // scan the following bytes for a value greater than 0x7F
      while ((readTokenIndex < cmpress_data_size) && (cmprss_data_ptr[readTokenIndex] != ERASED_BYTE))
      {
        nextToken.byte = cmprss_data_ptr[readTokenIndex];
        if ((nextToken.before & NIBBLE_NON_MATCH_BIT) != 0)
          break;
        else
          readTokenIndex++;
      }
    }
    else
    {
      // For a matched string,
      // duplicate the next byte N times into the decompressed array.
      memset(&uncmprss_data_ptr[writeIndex], cmprss_data_ptr[afterCopyIndex], (curToken.after & NIBBLE_VALUE_MASK));
      writeIndex += (curToken.after & NIBBLE_VALUE_MASK);

      // Then skip the following byte in the compressed array to find your next token.
      readTokenIndex += 3;
    }
#ifdef DEBUG
    print_array(uncmprss_data_ptr, writeIndex, GET_VAR_NAME(uncmprss_data_ptr));
#endif
    curToken.byte = cmprss_data_ptr[readTokenIndex];
  }

  sizeAfterDecompression = writeIndex;

  return sizeAfterDecompression;
}
