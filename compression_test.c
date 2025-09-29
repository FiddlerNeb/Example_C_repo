/**
 * @file compression_test_lib.c
 * @author Benjamin Odegaard (ben.odegaard10@gmail.com)
 * @brief a test compression implementation for technical interview
 * @version 0.2
 * @date 2025-09-15
 *
 */
#include <string.h>

#include ".\compression_test.h"

#define DEBUG 1

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
      for (uint8_t j = 1; j < NIBBLE_NON_MATCH_BIT - 1; j++)
      {
        if ((i + j) > data_size)
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
      // find the number of consecutive non-matches after the currVal
      for (uint8_t j = 1; j < NIBBLE_NON_MATCH_BIT; j++)
      {
        if ((i + j) > data_size)
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
        if (data_ptr[i + j] == ERASED_BYTE)
          break;
      }
    }
  }

  // an unmatched 1 is the same as a matched 1, so use the matched syntax since that helps our compression
  if (token1.after == 0x9)
    token1.after = 0x1;

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
  array_size_t itterationCount = data_size * 2;
  array_size_t cmprss_size_est = 0;

  while ((i < data_size) && (data_ptr[i] != ERASED_BYTE))
  {
    // prefent infinite loop
    itterationCount--;
    if (itterationCount <= 1)
      break;

    // get 2x consecutive sets of match/non-match sequences and set them as the lengths in the token along with their match bits
    token1.before = getMatchLen(data_ptr, i, data_size).after;
    token1.after = getMatchLen(data_ptr, i + (token1.before & NIBBLE_VALUE_MASK), data_size).after;

    if ((token1.before & NIBBLE_NON_MATCH_BIT) != 0)
    {
      cmprss_size_est += (token1.before & NIBBLE_VALUE_MASK) + 1;
    }
    else
    {
      if ((token1.before & NIBBLE_VALUE_MASK) == 1)
        cmprss_size_est += 1;
      cmprss_size_est += 1;
    }
    i += (token1.before & NIBBLE_VALUE_MASK) + 1;

    if ((token1.after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      cmprss_size_est += (token1.after & NIBBLE_VALUE_MASK) + 1;
    }
    else
    {
      if ((token1.after & NIBBLE_VALUE_MASK) == 1)
        cmprss_size_est += 1;
      cmprss_size_est += 1;
    }
    i += (token1.after & NIBBLE_VALUE_MASK) + 1;

    // for the token
    cmprss_size_est += 1;
  }

  return cmprss_size_est;
}

void read_into_buffer(buffer_element_t *readBuff, array_size_t *buffReadIndex, buffer_element_t *data_ptr, array_size_t data_size, array_size_t *readIndex)
{
  array_size_t readBytes = 0;

  if (*buffReadIndex > 0)
  {
    memmove(&readBuff[0], &readBuff[*buffReadIndex], BUFFER_SIZE - *buffReadIndex);
    *buffReadIndex = BUFFER_SIZE - *buffReadIndex;
    readBytes = BUFFER_SIZE - *buffReadIndex;
  }
  else
  {
    readBytes = BUFFER_SIZE;
    *buffReadIndex = 0;
  }

  if ((*readIndex + readBytes) >= data_size)
  {
    memset(&readBuff[*buffReadIndex + (data_size - *readIndex)], ERASED_BYTE, BUFFER_SIZE - (*buffReadIndex + (data_size - *readIndex)));
    readBytes = data_size - *readIndex;
  }

  if (*readIndex < data_size)
  {
    // read data into next buffer so that we can always overwrite a few bytes in the main array if we need to
    memmove(&readBuff[*buffReadIndex], &data_ptr[*readIndex], readBytes);
    *readIndex += readBytes;
  }
  else
  {
    memset(&readBuff[*buffReadIndex], ERASED_BYTE, BUFFER_SIZE - *buffReadIndex);
  }
  *buffReadIndex = 0;
}

ret_code copy_writeBuffer_to_output(buffer_element_t *writeBuff, array_size_t *buffWriteIndex, buffer_element_t *readBuff, array_size_t *buffReadIndex, buffer_element_t *data_ptr, array_size_t data_size, array_size_t *writeIndex, array_size_t *readIndex)
{
  if (*buffWriteIndex >= (*writeIndex - *readIndex))
  {
    read_into_buffer(readBuff, buffReadIndex, data_ptr, data_size, readIndex);
  }

  // make sure we have space to write the buffer contents
  if ((*buffWriteIndex < (data_size - *writeIndex)) &&
      ((*buffWriteIndex < (*writeIndex - *readIndex))))
  {
    memmove(&data_ptr[*writeIndex], &writeBuff[0], *buffWriteIndex);
    *writeIndex += *buffWriteIndex;
    *buffWriteIndex = 0;
  }
  else
  {
    // abort, we've already tried to read more bytes into the read_buffer
    // abort, there's not enough space left in the array to fit the compressed contents
    // we should never end up here
    return FAIL;
  }

  return PASS;
}

void fill_write_buffer_from_read_buffer(buffer_element_t *writeBuff, array_size_t *buffWriteIndex, buffer_element_t *readBuff, array_size_t *buffReadIndex, cmprss_token_t *prevToken)
{
  cmprss_token_t token1;

  // get 2x consecutive sets of match / non - match sequences and set them as the lengths in the token along with their match bits
  token1.before = getMatchLen(readBuff, *buffReadIndex, BUFFER_SIZE).after;
  token1.after = getMatchLen(readBuff, *buffReadIndex + (token1.before & NIBBLE_VALUE_MASK), BUFFER_SIZE).after;

  if ((token1.before == 0) && (token1.after == 0))
  {
    if (readBuff[*buffReadIndex] == ERASED_BYTE)
    {
      // add end of unmatched token at the end of the file
      token1.after = 0;
      token1.before = NIBBLE_NON_MATCH_BIT;
      writeBuff[(*buffWriteIndex)++] = (buffer_element_t)token1.byte;
    }
    return;
  }

  if ((token1.before & NIBBLE_NON_MATCH_BIT) != 0)
  {
    if (((*prevToken).after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      // we have non-matching characters which need to be continued
      memmove(&writeBuff[*buffWriteIndex], &readBuff[*buffReadIndex], (token1.before & NIBBLE_VALUE_MASK));
      *buffWriteIndex += (token1.before & NIBBLE_VALUE_MASK);
      *buffReadIndex += (token1.before & NIBBLE_VALUE_MASK);

      if ((token1.after & NIBBLE_NON_MATCH_BIT) != 0)
      {
        memmove(&writeBuff[*buffWriteIndex], &readBuff[*buffReadIndex], (token1.after & NIBBLE_VALUE_MASK));
        *buffWriteIndex += (token1.after & NIBBLE_VALUE_MASK);
        *buffReadIndex += (token1.after & NIBBLE_VALUE_MASK);
      }

      if (readBuff[*buffReadIndex] == ERASED_BYTE)
      {
        // add end of unmatched token at the end of the file
        token1.after = 0;
        token1.before = NIBBLE_NON_MATCH_BIT;
        writeBuff[(*buffWriteIndex)++] = (buffer_element_t)token1.byte;
      }
// prevToken.byte = token1.byte;
#ifdef DEBUG
      print_array(writeBuff, *buffWriteIndex, GET_VAR_NAME(writeBuff));
#endif
      return;
    }
    else
    {
      if (((*prevToken).after & NIBBLE_NON_MATCH_BIT) == 0)
      {
        // to go from matched to unmatched on a non-token boundry, we need to fake a match to move the boundry to the correct spot for decoding
        writeBuff[(*buffWriteIndex)++] = readBuff[(*buffReadIndex)++];
        token1.before -= 1;
        writeBuff[(*buffWriteIndex)++] = 0x10 + token1.before;
      }
      memmove(&writeBuff[*buffWriteIndex], &readBuff[*buffReadIndex], (token1.before & NIBBLE_VALUE_MASK));
      *buffWriteIndex += (token1.before & NIBBLE_VALUE_MASK);
#ifdef DEBUG
      print_array(writeBuff, *buffWriteIndex, GET_VAR_NAME(writeBuff));
#endif
      if (((*prevToken).after & NIBBLE_NON_MATCH_BIT) == 0)
      {
        *buffReadIndex += (token1.before & NIBBLE_VALUE_MASK);
        // TODO copy buffer to memory
        (*prevToken).byte = 0x10 + token1.before;
        return;
      }
    }
  }
  else
  {
    if (((*prevToken).after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      // add end of unmatched token at the end of the file
      token1.after = token1.before;
      token1.before = NIBBLE_NON_MATCH_BIT;
      writeBuff[(*buffWriteIndex)++] = (buffer_element_t)token1.byte;
    }

    // insert the before match value
    writeBuff[(*buffWriteIndex)++] = readBuff[*buffReadIndex];

    if (((*prevToken).after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      (*prevToken).byte = token1.byte;
      *buffReadIndex += (token1.after & NIBBLE_VALUE_MASK);
      return;
    }
  }
  *buffReadIndex += (token1.before & NIBBLE_VALUE_MASK);

#ifdef DEBUG
  print_array(writeBuff, *buffWriteIndex, GET_VAR_NAME(writeBuff));
#endif

  // insert the token
  writeBuff[(*buffWriteIndex)++] = (buffer_element_t)token1.byte;

  // TODO for a smaller buffer, should we copy buffer over here?

#ifdef DEBUG
  print_array(writeBuff, *buffWriteIndex, GET_VAR_NAME(writeBuff));
#endif

  // TODO still needed?
  // if ((token1.after == 0) || (readIndex>(data_size-1)))
  //   break;

  if ((token1.after & NIBBLE_NON_MATCH_BIT) != 0)
  {
    memmove(&writeBuff[*buffWriteIndex], &readBuff[*buffReadIndex], (token1.after & NIBBLE_VALUE_MASK));
    *buffWriteIndex += (token1.after & NIBBLE_VALUE_MASK);
    *buffReadIndex += (token1.after & NIBBLE_VALUE_MASK);
    if (readBuff[*buffReadIndex] == ERASED_BYTE)
    {
      // add end of unmatched token at the end of the file
      token1.after = 0;
      token1.before = NIBBLE_NON_MATCH_BIT;
      writeBuff[(*buffWriteIndex)++] = (buffer_element_t)token1.byte;
    }
  }
  else
  {
    // insert the after match value
    writeBuff[(*buffWriteIndex)++] = readBuff[*buffReadIndex];
    *buffReadIndex += (token1.after & NIBBLE_VALUE_MASK);
  }
#ifdef DEBUG
  print_array(writeBuff, *buffWriteIndex, GET_VAR_NAME(writeBuff));
#endif

  (*prevToken).byte = token1.byte;
}

/**
 * @brief compresses a byte array of data using a custom algorithm
 *
 * @param data_ptr
 * @param data_size
 * @return int
 */
int byte_compress(buffer_element_t *data_ptr, array_size_t data_size)
{
  array_size_t size_after_compression = 0, est_size = 0;
  array_size_t readIndex = 0;
  array_size_t writeIndex = 0, buffWriteIndex = 0, buffReadIndex = 0;
  cmprss_token_t token1, prevToken;
  token1.before = NIBBLE_MAX;
  token1.after = NIBBLE_MAX;
  prevToken.before = NIBBLE_MAX;
  prevToken.after = NIBBLE_MAX;
  // buffer_element_t buffer = ERASED_BYTE, buffer2 = ERASED_BYTE;
  buffer_element_t writeBuff[BUFFER_SIZE], readBuff[BUFFER_SIZE];
  memset(writeBuff, ERASED_BYTE, BUFFER_SIZE);
  memset(readBuff, ERASED_BYTE, BUFFER_SIZE);
  uint64_t itterationCount = data_size * 2;

  est_size = estimate_array_size(data_ptr, data_size);
  if (est_size >= (data_size - 1))
  {
    // likely uncompressible via this method, abort
    return data_size;
  }

#if DEBUG == 1
  print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif // DEBUG

  token1.before = getMatchLen(data_ptr, 0, data_size).after;

  // if we start off with a non matched streak, we need to put in a token at the beginning of the file
  if ((token1.before & NIBBLE_NON_MATCH_BIT) != 0)
  {
    // add a token in to indicate the end of the non-matching characters
    token1.after = token1.before;
    token1.before = NIBBLE_NON_MATCH_BIT;
    writeBuff[buffWriteIndex++] = (buffer_element_t)token1.byte;
    (prevToken).byte = token1.byte;
  }
  else
  {
    (prevToken).byte = TOKEN_INIT;
  }

  do
  {
    // prevent infinite loop
    itterationCount--;
    if (itterationCount <= 1)
      break;

    read_into_buffer(readBuff, &buffReadIndex, data_ptr, data_size, &readIndex);
#if DEBUG == 1
    print_array(readBuff, BUFFER_SIZE, GET_VAR_NAME(readBuff));
#endif // DEBUG
    fill_write_buffer_from_read_buffer(writeBuff, &buffWriteIndex, readBuff, &buffReadIndex, &prevToken);
#if DEBUG == 1
    print_array(writeBuff, BUFFER_SIZE, GET_VAR_NAME(writeBuff));
#endif // DEBUG
    if (PASS != copy_writeBuffer_to_output(writeBuff, &buffWriteIndex, readBuff, &buffReadIndex, data_ptr, data_size, &writeIndex, &readIndex))
    {
      // ERROR
      writeIndex = 0;
      break;
    }
#if DEBUG == 1
    print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif // DEBUG
#if DEBUG == 1
    print_array(readBuff, BUFFER_SIZE, GET_VAR_NAME(readBuff));
#endif // DEBUG
  } while (readBuff[0] != ERASED_BYTE);

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
int byte_compress_old(buffer_element_t *data_ptr, array_size_t data_size)
{
  array_size_t size_after_compression = 0;
  array_size_t i = 0;
  uint8_t writeIndex = 0, eofWriteIndex = BUFFER_SIZE - 1;
  cmprss_token_t token1, prevToken;
  token1.before = NIBBLE_MAX;
  token1.after = NIBBLE_MAX;
  prevToken.before = NIBBLE_MAX;
  prevToken.after = NIBBLE_MAX;
  buffer_element_t buffer = ERASED_BYTE, buffer2 = ERASED_BYTE;
  buffer_element_t eofBuffer[BUFFER_SIZE];
  memset(eofBuffer, ERASED_BYTE, BUFFER_SIZE);
  uint64_t itterationCount = data_size * 2;

  if (estimate_array_size(data_ptr, data_size) >= data_size)
  {
    // likely uncompressible via this method, abort
    return data_size;
  }

  while ((i < data_size) && (data_ptr[i] != ERASED_BYTE))
  {
    // prefent infinite loop
    itterationCount--;
    if (itterationCount <= 1)
      break;

    // get 2x consecutive sets of match/non-match sequences and set them as the lengths in the token along with their match bits
    token1.before = getMatchLen(data_ptr, i, data_size).after;
    token1.after = getMatchLen(data_ptr, i + (token1.before & NIBBLE_VALUE_MASK), data_size).after;

    if ((token1.before == 0) && (token1.after == 0))
      break;

    // check if we're about to reach the end of the buffer
    if (data_size < (i + (token1.before & NIBBLE_VALUE_MASK) + 1 + (token1.after & NIBBLE_VALUE_MASK)))
    {
      if ((writeIndex + ((data_size) - (i))) < data_size)
      {
        if (data_ptr[i] == ERASED_BYTE)
          break;
        else if (eofWriteIndex + 1 < BUFFER_SIZE)
        {
          // eofBuffer[eofWriteIndex--] = data_ptr[data_size-1];
          memmove(&data_ptr[writeIndex], &data_ptr[i], ((data_size) - (i)));

#ifdef DEBUG
          print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif
          memmove(&data_ptr[writeIndex + ((data_size) - (i))], &eofBuffer[eofWriteIndex + 1], BUFFER_SIZE - (eofWriteIndex + 1));

#ifdef DEBUG
          print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif
          memset(&data_ptr[writeIndex + (BUFFER_SIZE - (eofWriteIndex + 1)) + ((data_size) - (i))], 0xFF, ((data_size) - (writeIndex + (BUFFER_SIZE - (eofWriteIndex + 1)) + ((data_size) - (i)))));
          eofWriteIndex = BUFFER_SIZE;
          i = writeIndex;
#ifdef DEBUG
          print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif
          buffer = ERASED_BYTE;
          continue;
        }
      }
      else if (((token1.before & NIBBLE_NON_MATCH_BIT) == 0) &&
               (i + (token1.before) < (data_size - 1)))
      {
        // let the algorithm continue at least once to try and compress some more space last minute
      }
      else
      {
        break;
      }
    }

    if ((prevToken.after & NIBBLE_NON_MATCH_BIT) != 0) // && ((i == 0) || (token1.after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      // if we start off with a non matched streak, we need to put in a token at the beginning of the file
      if ((i != 0) && ((token1.before & NIBBLE_NON_MATCH_BIT) == 0) ||
          (i == 0) && ((token1.before & NIBBLE_NON_MATCH_BIT) != 0))
      {
        // add a token in to indicate the end of the non-matching characters
        token1.after = token1.before;
        token1.before = NIBBLE_NON_MATCH_BIT;
        // save the value from the space to be used by the token
        buffer = data_ptr[writeIndex];
      }
      else if (i == 0)
      {
        // matching bit at the beginning of the array, move on
      }
      else if (token1.after == 0)
      {
        // the unmatching before bytes go to the end of the file, we should add a token and allow the compression to terminate
      }
      else
      {
        // we have non-matching characters which need to be continued
        memmove(&data_ptr[writeIndex], &data_ptr[i], (token1.before & NIBBLE_VALUE_MASK));
        writeIndex = writeIndex + (token1.before & NIBBLE_VALUE_MASK);
        i = i + (token1.before & NIBBLE_VALUE_MASK);
        buffer = ERASED_BYTE;
// prevToken.byte = token1.byte;
#ifdef DEBUG
        print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif

        if (i >= data_size)
        {
          // add a token in to indicate the end of the non-matching characters
          token1.after = 0;
          token1.before = NIBBLE_NON_MATCH_BIT;
          data_ptr[writeIndex++] = (buffer_element_t)token1.byte;
        }
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
        // to go from matched to unmatched on a non-token boundry, we need to fake a match to move the boundry to the correct spot for decoding
        data_ptr[writeIndex++] = data_ptr[i++];
        token1.before = token1.before - 1;
        data_ptr[writeIndex++] = 0x10 + token1.before;
      }
      memmove(&data_ptr[writeIndex], &data_ptr[i], (token1.before & NIBBLE_VALUE_MASK));
      writeIndex = writeIndex + (token1.before & NIBBLE_VALUE_MASK);
#ifdef DEBUG
      print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
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
    print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif
    i = i + (token1.before & NIBBLE_VALUE_MASK);

    // insert the token
    data_ptr[writeIndex++] = (buffer_element_t)token1.byte;

#ifdef DEBUG
    print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif

    if ((token1.after == 0) || (i > (data_size - 1)))
      break;

    if ((token1.after & NIBBLE_NON_MATCH_BIT) != 0)
    {
      if (buffer != ERASED_BYTE)
      {
        if ((writeIndex + (token1.after & NIBBLE_VALUE_MASK)) < i)
        {
          memmove(&data_ptr[writeIndex + 1], &data_ptr[writeIndex], (token1.after & NIBBLE_VALUE_MASK));
          data_ptr[writeIndex] = buffer;
#ifdef DEBUG
          print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif
        }
        else
        {
          // problem case where we've not got enough space to insert the token
          eofBuffer[eofWriteIndex--] = data_ptr[data_size - 1];
          memmove(&data_ptr[writeIndex + 1], &data_ptr[writeIndex], ((data_size - 1) - (writeIndex)));
          data_ptr[writeIndex] = buffer;
#ifdef DEBUG
          print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
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
      data_ptr[writeIndex] = data_ptr[i + 1];
      writeIndex = writeIndex + 1;
    }
#ifdef DEBUG
    print_array(data_ptr, data_size, GET_VAR_NAME(data_ptr));
#endif

    i = i + (token1.after & NIBBLE_VALUE_MASK);

    // in case the unmatch token1.after goes to the end of the array, add one last token to delimit the file.
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
