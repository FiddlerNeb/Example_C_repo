@mainpage Ben O Compression algorithm for interview 2025-09-19
@tableofcontents
@section Assignment Code Design Test: Data Compression Design
<p>
Design an algorithm that will compress a given data buffer of bytes. 
<br>Please describe your design and submit an implementation in a language of your choice.
</p>
 *  The algorithm will live within a function.
 *  This function will be called with two arguments;
    *   A pointer to the data buffer (data_ptr)
    *   The number of bytes to compress (data_size).
 * After the function executes the data in the buffer will be modified and the size of the modified buffer will be returned.
 * Assumptions:
    *  The data_ptr will point to an array of bytes.
    *   Each byte will contain a number from 0 to 127 (0x00 to 0x7F).
    *   It is common for the data in the buffer to have the same value repeated in the series.
    *   The compressed data will need to be decompressable.
    *   Please ensure that your algorithm allows for a decompression algorithm to return the buffer to it’s previous form.
> **Example data and function call:**<br>
<code>
   // Data before the call<br>
    data_ptr[] = { 0x03, 0x74, 0x04, 0x04, 0x04, 0x35, 0x35, 0x64,<br>
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0x64, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,<br>
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0x56, 0x45, 0x56, 0x56, 0x56, 0x09, 0x09, 0x09 };<br>
    data_size = 24;<br>
    new_size = byte_compress( data_ptr, data_size );<br>
</code>
@section plan Planning
<p>
* use doxygen to document code
* approach code for unit testing as well as easy reading where possible
* emphesize embedded context, perhaps balance throughput and compression as if they were requirements?
* schedule at least 50% of development time for testing & itteration
</p>

 @section CompressionAnswer Compression Algorithm pseudocode
 @subsection General General description
 <p>
Inspired by the <a href="https://lz4.org/" target="_blank">L4Z algorithm</a> I found during my research, I chose this since it balances compression speed with compression size in order to ensure throughput. Ensuring throughput would be essential for applications like BLE data transmission on an embedded device.<br>
 </p>
 <p>
 I decided to focus on creating pairs of byte counts of sequences of duplicated bytes with a sample of the byte. These pairs would be inserted periodically within the data so that the data could be parsed in sequence or in portions if needeed. Parsing the data in portions would be practical in a transmission application where the receiving device could start decompressing the data without waiting for the transmission to complete.<br>
 </p>
@subsection Matched Compression of matched sequences
<p>
I formatted the sequence byte counts into a half-byte (nibble) in order to minimize the size of the byte counts. I then put 2x nibbles together to form a "byte count token" which indicates the byte count of the preceding and following byte sequences. This helps parsing by byte-by-byte for simplicity. Finally only one example byte of each of the preceding and following byte sequences needs to be kept after compression, resulting in a reduction in overall byte count.<br>
<br>
I decided to limit to using only the first 3x bits of the nibble, limiting the match recognition lengh to 7. This simplification allowed for an unused bit which I used in the un-matched sequence enhancement described below. This also reduced the complexity of test cases for this implementation.
</p>
> **Example data and compression result:**<br>
<code>
original bytes:<br>
0x04, 0x04, 0x04, 0x35, 0x35<br>
preceding byte count: 3<br>
following byte count: 2<br>
token (Hex): 0x32<br>
after insertion into sequence:<br>
0x04, 0x04, 0x04, 0x32, 0x35, 0x35<br>
after removing un-necesarry sequence bytes:<br>
0x04, 0x32, 0x35<br>
</code>
@subsection Un-matched Compression of un-matched sequences

During testing I found poor performance when dealing with multiple un-duplicated bytes in a row. A refinement I made is that, un-duplicated bytes would get treated as a single sequence of un-duplicated bytes putting a token at the start and end of the sequence. This reduced the amount of tokens inserted when there was no compression via byte sequence duplication.<br>
If the input array starts with an unmatched sequence, we must input a token (0x8#) at the beginning to allow for our decompression strategy, see @ref DecompressionAnswer
<br>I utilized the unused 0x8 nibble bit to indicate an unmatched sequence, the start nibble may optionally include the count of unmatched bytes but is not necesarry by design since this allows for fewer tokens for unmatched sequences longer than 7.<br>
**Improvement:** for sequences longer than 7, add a length byte after the token<br>
> **Basic Example data and un-duplication enhanced compression result:**<br>
<code>
pre compression size: 8
original bytes:<br>
0x02, 0x02, 0x04, 0x05, 0x07, 0x03, 0x03, 0x03<br>
preceding matched byte count: 2<br>
following unmatched byte count: 3<br>
following matched byte count: 3<br>
before unmatched token (Hex): 0x2B<br>
after unmatched token (Hex):  0x83<br>
after insertion into sequence:<br>
0x02, 0x02, 0x2B, 0x04, 0x05, 0x07, 0x83, 0x03, 0x03, 0x03<br>
after removing un-necesarry sequence bytes:<br>
0x02, 0x2B, 0x04, 0x05, 0x07, 0x83, 0x03 <br>
post compression size: 7
</code>
@subsection OverflowHandling Overflow Handling
<p>
Since adding tokens to unmatched byte sequences results in a net gain in bytes, we need to add overlow capability to our algorithm to preserve later data in our input array. This overflow capability will unfortunately be cyclically costly as we have to create a gap in the data to insert the tokens.<br>
**Improvement:** If I had more time, I would pull a buffer out of the beginning of the file first to make room and then proceed with the algorithm as-is.<br>
**Improvement:** utilize an output memory space, rather than overwritting the input buffer, this could be an input to the function<br>
**Improvement:** dynamically allocate more memory to the array, but this is typically disabled in embedded applications.<br>
</p>
> **Overflow Example data and un-duplication enhanced compression result:**<br>
<code>
pre compression size: 3
original bytes:<br>
0x04, 0x05, 0x07<br>
preceding unmatched byte count: 3<br>
following byte count: 3<br>
before unmatched token (Hex): 0x8B<br>
after unmatched token (Hex):  0x80<br>
after insertion into sequence:<br>
0x8B, 0x04, 0x05, 0x07, 0x80<br>
after removing un-necesarry sequence bytes:<br>
0x8B, 0x04, 0x05, 0x07, 0x83, 0x35<br>
post compression size: 6 **Results in more bytes than we started with!!!**
</code>
@section DecompressionAnswer Decompression Algorithm pseudocode
<p>
Similar to compression, decompression can be done in sequence and in segments.
  * The first token will always be either in the first or second byte of the compressed array. 
        * If the first byte contains a value larger than 0x7F then the file starts with an unmatched string. 
        * If not, then the second byte is your first token. 
After Identifying the first token, you follow the compression rules in reverse to decompress the bytes. <br>
  * For a token nibble N in the compressed array, indicating a matched string, duplicate the next byte N times into the decompressed array. Then skip the following byte in the compressed array to find your next token.<br>
  * For a token nibble N in the compressed array, indicating an unmatched string, scan the following bytes for a value greater than 0x7F to identify the next tokena and the end of the unmatched length.<br>
**Improvement:** recommend that the output not be the same array as the compressed version for cyclic efficiency.<br>
**Improvement:** recommend that there be a header/footer showing how large the decompressed size is for usage for memory allocation<br>
</p>
> **Example matched token data and decompression result:**<br>
<code>
original bytes:<br>
0x04, 0x32, 0x35, 0x04, 0x32, 0x35<br>
token1 (Hex): 0x32<br>
preceding byte count: 3<br>
following byte count: 2<br>
token1 bytes decompressed:
0x04,0x04,0x04, 0x35, 0x35,
token2 (Hex): 0x32<br>
token2+Token1 bytes decompressed:
0x04,0x04,0x04, 0x35, 0x35, 0x04,0x04,0x04, 0x35, 0x35,
</code>
<br>
<br>
> **Example unmatched token data and decompression result:**<br>
<code>
pre compression size: 3
original bytes:<br>
0x8B, 0x04, 0x05, 0x07, 0x83, 0x35<br>
token1 (Hex): 0x8B<br>
token2 (Hex): 0x83<br>
preceding byte count: 0
unmatched sequence byte count: 3<br>
following matched sequence byte count: 3<br>
bytes decompressed:
0x04, 0x05, 0x07, 0x35, 0x35, 0x35<br>
post compression size: 6
</code>
@section improvements Improvements
<p>
If I had more time, 
1. I'd get clearer requirements on the application context, as this would help decide whether throughput, compression, or maintainability should be prioritized.
2. I'd re-architect the solution to be more modular. The trouble with prioritizing memory and cyclic efficiency is that you sacrifice on maintainability and stability of the code. Planning and testing are the key to maintainable firmware.
3. I'd use a unit test framework to set up the unit tests so that they're more maintainable and repeatable.

</p>
