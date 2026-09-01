/****************************************************************************
 *  FAPEC - Fully Adaptive Prediction Error Coder                           *
 *   (c) DAPCOM Data Services S.L. - http://www.dapcom.es                   *
 *   Entropy coding core patent: US20120166503 A1, 28-June-2012,            *
 *   "Method for fully adaptive calibration of a prediction error coder".   *
 *   Contact: jordi.portell@dapcom.es / fapec@dapcom.es                     *
 ****************************************************************************
 *   This software is property of DAPCOM Data Services S.L.                 *
 *   This C header file and the associated binary (compiled) libraries are  *
 *   subject to the license terms you should have received.                 *
 *   In particular, it is prohibited to install, use, integrate in another  *
 *   software or hardware system, or create derivative works, except as     *
 *   explicitly allowed by the license terms or by DAPCOM.                  *
 *   It is prohibited to copy, distribute, modify, sublicense, lease, rent, *
 *   loan, sell or reverse engineer without explicit permission of DAPCOM.  *
 ****************************************************************************/

/****************************************************************************
 * FAPEC - high-performance professional data compressor and archiver.
 *
 * @file fapec_comp.h:
 *
 * @brief Definition of the main compression functions.
 *
 * FAPEC can be invoked at different levels:
 * - File: similar to invoking the command-line compressor. It takes care of
 *   all the headers (metadata) generation, "multi-parts" if applicable
 *   (in FAPEC one "part" corresponds to one file and one given compression
 *   setup), chunking, multi-thread if requested, etc.
 *   It allows compressing directory trees or several files in a directory,
 *   if the input parameters request so. It can also be used for compressing
 *   just one single file, of course.
 *   This is the most "self-contained" case (all metadata automatically
 *   handled by FAPEC).
 * - Buffer: equivalent to the File case for just 1 single file.
 *   The buffer will be self-contained regarding compression options
 *   (unless explicitly deactivated).
 * - Chunk: This is the most basic invocation level for FAPEC; it is the
 *   core compression method. Please handle with care: here you need to take
 *   care of "chunking" and metadata yourself. That is: a compressed chunk
 *   won't be completely self-contained (you'll need to know and provide the
 *   compression options when decompressing it). And you need to take care
 *   of eventual "chunking" when compressing large buffers or files: you can
 *   handle up to 384MB at once, in a single chunk, but preferably you should
 *   split it in smaller chunks (typ. 1-16MB). Finally, in some cases
 *   (such as KWCD/KMALL, Tab/CSV or FastQ) you should make sure to provide
 *   "aligned" chunks (each starting at the actual start of a meaningful data
 *   block) to get better compression performance.
 * Here we provide functions to compress from disk to disk (file),
 * from (self-contained) buffer to buffer, from buffer to file, and
 * from raw chunk to compressed chunk.
 ****************************************************************************/


#ifndef _FAPEC_COMPRESSOR_H_
#define _FAPEC_COMPRESSOR_H_

#include <stdlib.h>
#include "fapec_opts.h"

/* Visibility options */
#ifndef _WIN32
	#define LINUX_VISIBILITY __attribute__ ((visibility("default")))
	#define WINDOWS_VISIBILITY
#else
	#define LINUX_VISIBILITY
	#ifdef F_LIB
		#define WINDOWS_VISIBILITY __declspec(dllexport)
	#else
		#define WINDOWS_VISIBILITY
	#endif
#endif



/**
 * Memory-based single-buffer (chunk) compression function
 *
 * It takes the buffer "buff" of size "buffSize" bytes,
 * compresses it with "cmpCfg" configuration (see e.g. fapec_cmpopts_new)
 * and "userOpts" options (see fapec_usropts_new_ck),
 * and returns the compressed buffer in the same "buff" parameter
 * (updating "buffSize" with the compressed buffer size in bytes).
 * It returns zero (or a positive value to indicate some
 * information from the compression) in case of success,
 * or -1 (or another negative value) to indicate a non-successful compression.
 * The user does not have to worry about the buffer allocated in "buff": the
 * routine frees the available pointer there and allocates a new one for
 * the compressed results. It means that such returned buffer must be freed by
 * the user.
 *
 * @param buff      Pointer to the buffer with the raw data to be compressed.
 *                  It will be updated to point to the new buffer with
 *                  compressed data.
 * @param buffSize  Original (raw, uncompressed) size of the buffer. It will
 *                  be updated to the compressed size.
 * @param userOpts  User/runtime options.
 * @param cmpCfg    FAPEC compression configuration.
 * @return Zero if successul, 2 if the buffer could not be compressed (in
 *         which case the original buffer and size will be returned),
 *         negative in case of errors.
 */
WINDOWS_VISIBILITY int fapec_compress_chunk(unsigned char **buff, size_t *buffSize,
		int userOpts, void *cmpCfg) LINUX_VISIBILITY;


/**
 * Memory-based input+output buffer (chunk) compression function
 *
 * It takes the buffer "inBuff" of size "inBuffSize" bytes,
 * compresses it with "cmpCfg" configuration (see e.g. fapec_cmpopts_new)
 * and "userOpts" options (see fapec_usropts_new_ck),
 * and returns the compressed buffer into the PRE-ALLOCATED "outBuff" parameter
 * (setting "outBuffSize" to the compressed buffer size in bytes).
 * IMPORTANT: outBuff must be preallocated with at least 1.2 x inBuffSize.
 * The value of *outBuffSize does not mind at all.
 * It returns zero (or a positive value to indicate some
 * information from the compression) in case of success,
 * or -1 (or another negative value) to indicate a non-successful compression.
 * Compared to fapec_compress_chunk(), this function is a bit more efficient
 * when we can reuse memory buffers (we avoid the need of allocating+freeing
 * them for every chunk). That is specially interesting when dealing with
 * tiny chunks.
 * Actually, fapec_compress_chunk() calls this function.
 *
 * @param inBuff      Input buffer with the raw data to be compressed.
 * @param inBuffSize  Size of the input buffer.
 * @param outBuff     Pre-allocated output buffer.
 * @param outBuffSize Output buffer size is stored here.
 * @param userOpts    User/runtime options.
 * @param cmpCfg      FAPEC compression configuration.
 * @return Zero if successul, 2 if the buffer could not be compressed (in
 *         which case the original buffer and size will be returned),
 *         negative in case of errors.
 *
 */
WINDOWS_VISIBILITY int fapec_compress_chunk_reusebuff(unsigned char *inBuff, size_t inBuffSize,
		unsigned char *outBuff, size_t *outBuffSize,
		int userOpts, void *cmpCfg) LINUX_VISIBILITY;


/**
 * Generate the FCC External Header.
 * This is useful for users that wish to do their own chunking on a data stream,
 * and later on, decompress the multi-chunk output (compressed buffer or file).
 * These external headers allow to properly identify and handle the chunks,
 * e.g. through fapec_decomp_file or the CLI.
 * @param isComp   True if the chunk was actually compressed (see fapec_compress_chunk_reusebuff return value).
 * @param isLast   True if it's the last chunk in the stream.
 * @param dataSize Compressed data size.
 * @param edac     EDAC option (see e.g. fapec_usropts_new_ck).
 * @param outPtr   Pre-allocated output buffer (16 bytes are enough).
 * @return Size of the FCC EH.
 */
WINDOWS_VISIBILITY int fapec_gen_fcceh(bool isComp, bool isLast, int dataSize, uint8_t edac,
        uint8_t *outPtr) LINUX_VISIBILITY;


/**
 * Main file-based compression function
 *
 * It takes the input file(s) "inFiles",
 * applies FAPEC on it using the configuration indicated in "cmpCfg",
 * and writes the compressed output on "outFile".
 * Either a fixed or automatic compression configuration will be used depending on "cmpCfg".
 * If autoconfigured, "cmpCfg" is updated to the compression options automatically selected for this file.
 * Compression information (progress, result, etc.) is printed to stdout depending on the 'verbosity' level.
 *
 * @param inFiles   Array of null-terminated strings indicating the input file and folder names
 *                  (including relative or absolute paths if needed) to be compressed.
 * @param numFiles  Number of files or folders provided through the inFiles array.
 * @param outFile   Output (.fapec) file where all input file(s) will be compressed.
 * @param userOpts  User/runtime options.
 * @param cmpCfg    FAPEC compression configuration.
 * @return          Zero if successful, negative if errors, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_compress_file(char **inFiles, int numFiles, char *outFile,
        int userOpts, void *cmpCfg) LINUX_VISIBILITY;


/**
 * Buffer-to-buffer compression function
 *
 * It takes an input buffer *inBuff of inSize bytes,
 * allocates the necessary output buffer *outBuff
 * (which will be at most inSize bytes plus a small overhead of some ~1K),
 * compresses inBuff into outBuff using FAPEC
 * (with the configuration given in "cmpCfg"),
 * resizes the resulting buffer to just the actual size generated,
 * and returns such size through outSize.
 * The user is responsible of freeing *inBuff and *outBuff when done!
 *
 * @param inBuff    Input buffer with the data to be compressed.
 * @param outBuff   Pointer to the (internally allocated) output, compressed buffer.
 * @param inSize    Input buffer size.
 * @param outSize   Output (compressed) size will be stored here.
 * @param userOpts  User/runtime options.
 * @param cmpCfg    FAPEC compression configuration.
 * @return          Zero if successful, negative if errors, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_compress_buff2buff(unsigned char *inBuff, unsigned char **outBuff,
		int64_t inSize, int64_t *outSize,
		int userOpts, void *cmpCfg) LINUX_VISIBILITY;


/**
 * Buffer-to-file compression function
 *
 * It takes an input buffer *inBuff of inSize bytes,
 * compresses it with FAPEC using the configuration indicated in "cmpCfg",
 * and writes the compressed output on "outFile".
 * The user is responsible of freeing *inBuff when done!
 * @param inBuff    Input buffer with the data to be compressed.
 * @param inSize    Input buffer size.
 * @param outFile   Output (.fapec) file where input buffer will be compressed.
 * @param userOpts  User/runtime options.
 * @param cmpCfg    FAPEC compression configuration.
 * @return          Zero if successful, negative if errors, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_compress_buff2file(unsigned char *inBuff, int64_t inSize,
        char *outFile, int userOpts, void *cmpCfg) LINUX_VISIBILITY;


#endif
