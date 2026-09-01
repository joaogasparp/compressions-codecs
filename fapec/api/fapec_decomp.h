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


/****************************************************************************
 * FAPEC - high-performance professional data compressor and archiver.
 *
 * @file fapec_decomp.h:
 *
 * @brief Definition of the main decompression functions.
 *
 * FAPEC can be invoked at different levels:
 * - File: similar to invoking the command-line decompressor. It takes care of
 *   all "multi-parts" handling if applicable, identify decompression options
 *   from headers (metadata), rebuild from chunking, multi-thread if requested,
 *   recover from errors if applicable, etc.
 *   It allows decompressing directory trees or several files in a directory.
 *   It can also be used for decompressing just one single file, of course.
 *   This is the most "self-contained" use (all cases can be automatically
 *   handled by FAPEC).
 * - Buffer: quite equivalent to the File case for just 1 single file.
 * - Chunk: This is the most basic invocation level for FAPEC, it is the
 *   core decompression method. Please handle with care: here you need to take
 *   care of "de-chunking" and metadata yourself. That is: a compressed chunk
 *   won't be completely self-contained (you'll need to know and provide the
 *   decompression options). And you need to take care of eventual "de-chunking"
 *   when decompressing large buffers or files.
 * Here we provide functions to decompress from disk to disk (file), from
 * (self-contained) buffer to buffer, from a file to a buffer,
 * and from compressed chunk to decompressed chunk.
 * We also provide convenience functions to access a multi-part (multi-file,
 * folders, etc.) FAPEC archive, getting the number and names of parts
 * contained there, sizes, algorithms used, decompress a specific part
 * to a file or to a buffer, etc.
 ****************************************************************************/


#ifndef _FAPEC_DECOMPRESSOR_H_
#define _FAPEC_DECOMPRESSOR_H_

#include "fapec_opts.h"



/**
 * Public structure to hold the FAPEC compression configuration for a given part,
 * as returned by fapec_get_part_cmpopts, allowing to easily access the several options.
 * These are just the options relevant for an adequate decoding or understanding of
 * the part data (e.g. image, interleaved data, etc).
 */
typedef struct _fapecPartCmpOpts {
    char algorithm[10];           /* String with the type of pre-processing: Basic, TabTxt, Float, DWT, CILLIC, Wave, ... */

    unsigned char sampleBits;     /* Sample (or pixel) coding size in bits */
    unsigned char usefulPixBits;  /* Real (meaningful) bits per sample (or pixel) */
    unsigned char isBigEndian;    /* For 16-bit or 24-bit samples: 1 means Big Endian, 0 Little Endian */
    unsigned char hasSignedInts;  /* 0 means unsigned, 1 means signed */
    unsigned char lossy;          /* 0 means lossless, otherwise it indicates the lossy level. */

    /* Only applicable to algorithm = "DWT", "HPA" or "CILLIC": */
    unsigned imageWidth;          /* Width of the image, in pixels. */
    unsigned imageHeight;         /* Height of the image, in pixels. */
    unsigned short nBands;        /* Number of bands of the multi/hyperspectral image (also applicable to the "PBMB" algorithm). */
    unsigned char bandsCoding;    /* Multi-band coding: 0 = BIP, 1 = BIL, 2 = BSQ, 3 = Bayer RGGB. */

    unsigned char filterOrder;    /* Order of the filter, just for "Filter", "Wave" or "Wave2". */

    unsigned short waveChannels;  /* Only in "Wave" or "Wave2": Number of channels */

    unsigned short interleaving;  /* Interleaving used in "Basic" or "Filter", in samples. */
    unsigned char noPreProc;      /* Data format in "None": 0 for twos-complement, 1 for MSB as sign bit, 2 for unsigned values. */

    int chunkSize;                /* Size of the chunk in bytes */
    int headerOffset;             /* Offset (bytes) corresponding to the header of the file, handled separately in e.g. image compression. */
    int chunkOffset;              /* Offset (bytes) of the header of each chunk. */
} t_fapecPartCmpOpts;



/**
 * Main memory-based single-buffer decompression function
 *
 * It takes the buffer "buff" of size "buffSize" bytes,
 * decompresses it with "cmpCfg" configuration,
 * and returns the decompressed buffer in the same "buff" parameter
 * (updating "buffSize" with the decompressed buffer size in bytes).
 * It returns zero or 1 in case of success, or a negative value to
 * indicate a non-successful decompression.
 * The user does not have to worry about the buffer allocated in "buff": the
 * routine frees the available pointer there and allocates a new one for
 * the decompressed results. It means that such returned buffer must be freed by
 * the user.
 *
 * @param buff      Pointer to the buffer with compressed data to be restored.
 *                  It will be updated to point to the new buffer with
 *                  decompressed data.
 * @param buffSize  Compressed size of the buffer. It will
 *                  be updated to the restored (raw) size.
 * @param userOpts  User/runtime options.
 * @param cmpCfg    FAPEC decompression configuration.
 * @return Zero if successul, 1 to indicate that it's the last chunk for the
 *         part being decompressed, negative in case of errors.
 */
WINDOWS_VISIBILITY int fapec_decomp_chunk(unsigned char **buff, size_t *buffSize,
		int userOpts, void *cmpCfg) LINUX_VISIBILITY;


/**
 * Main memory-based input+output buffers decompression function
 *
 * It takes the buffer "inBuff" of size "inBuffSize" bytes,
 * decompresses it with "cmpCfg" configuration,
 * ("userOpts" indicate some file-wide options),
 * and returns the decompressed buffer in "outBuff" parameter
 * (updating "outBuffSize" with the decompressed buffer size in bytes).
 * IMPORTANT: outBuff must be preallocated with the adequate ChunkSize
 * (you may get it e.g. with fapec_get_rawchunksize); the
 * actual size returned will be updated in outBuffSize.
 * It returns zero or a positive value in case of success,
 * or a negative value to indicate a non-successful decompression.
 * Compared to fapec_decomp_chunk(), this function is a bit more efficient
 * when we can reuse memory buffers (we avoid the need of allocating+freeing
 * them for every chunk). That is specially interesting when dealing with
 * tiny chunks.
 * Actually, fapec_decomp_chunk() calls the present function.
 *
 * @param inBuff      Input buffer with the data to be decompressed.
 * @param inBuffSize  Size of the input buffer.
 * @param outBuff     Pre-allocated output buffer.
 * @param outBuffSize Output buffer size is stored here.
 * @param userOpts    User/runtime options.
 * @param cmpCfg      FAPEC compression configuration.
 * @return Zero if successul, 1 to indicate that it's the last chunk for the
 *         part being decompressed, negative in case of errors.
 */
WINDOWS_VISIBILITY int fapec_decomp_chunk_reusebuff(unsigned char *inBuff, size_t inBuffSize,
		unsigned char *outBuff, size_t *outBuffSize,
		int userOpts, void *cmpCfg) LINUX_VISIBILITY;


/**
 * Get the compressed chunk size
 *
 * It takes a buffer with a FAPEC compressed chunk
 * and returns its original (raw, uncompressed) size in bytes
 * (from the chunk internal header).
 *
 * @param inBuff      Input buffer with the data to be decompressed.
 * @param inBuffSize  Size of the input buffer.
 * return Number of bytes that will be generated when decompressing
 *        this chunk, or negative in case of problems. Specifically,
 *        it may return the raw bytes size with negative sign if the
 *        format of the chunk is different to that of the present code.
 */
WINDOWS_VISIBILITY int fapec_get_rawchunksize(unsigned char *inBuff,
		size_t inBuffSize) LINUX_VISIBILITY;


/**
 * Decode an FCC External Header (as generated by fapec_gen_fcceh)
 * which, if present, must be read before invoking fapec_get_rawchunksize
 * or a chunk decompression function.
 * @param inPtr  Input data pointer
 * @param edac   EDAC option
 * @param isComp Set to true if the chunk was compressed; if false, no need to call fapec_get_rawchunksize or the chunk decompression
 * @param isLast Set to true if it's the last chunk in the sequence
 * @return Compressed dataSize (total compressed chunk size including internal header), or negative if errors
 */
WINDOWS_VISIBILITY int fapec_decode_fcceh(uint8_t *inPtr,
        uint8_t edac, bool *isComp, bool *isLast) LINUX_VISIBILITY;


/**
 * Main file-based decompression function
 *
 * It takes the input file "inFile", checks its consistency and loads the necessary
 * configuration from it, decompresses it, and writes the compressed output on "outFile".
 * "cmpCfg" can be NULL, except in these cases:
 * - Decompression of "raw-only" compression cases (i.e. when we don't store the
 *   compression metadata in the .fapec file, mainly due to "-rawout" or "-nofcsh"
 *   options in compression). Then you need to specify the (de)compression configuration here.
 * - Decompression of encrypted files (so that we can provide the decryption key
 *   through these options).
 * - Provision of a specific output filename through these options (only applicable
 *   to single-part archives, stdout, or /dev/null). Note that this can also be
 *   achieved by providing a null cmpCfg and a non-null outFile.
 * Decompression information (progress, result, etc.) is printed to stdout depending
 * on the 'verbosity' level.
 * "outFile" can be null, in which case it will just take the (original) output
 * filename(s) embedded in the metadata of the compressed file (or the specific output
 * filename configured in cmpCfg).
 * If outFile is not null, it will always take precedence (except in multi-part archives).
 *
 * @param inFile   Input (.fapec) file to be decompressed.
 * @param outFile  For single-file .fapec archives, output filename to use.
 * @param userOpts User/runtime options.
 * @param cmpCfg   FAPEC compression configuration. Freed here before returning.
 * @return Zero if successul, negative in case of errors, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_decomp_file(char *inFile, char *outFile, int userOpts, void *cmpCfg)
	LINUX_VISIBILITY;


/**
 * File-to-buffer decompression function
 *
 * It takes the input file "inFile", checks its consistency and loads the necessary
 * configuration from it, decompresses it, and writes the compressed output into
 * a new buffer (internally allocated with the necessary size) into "outBuff".
 * The size is returned through outSize.
 * When returning, the user does not need to worry about freeing cmpCfg (in case
 * it was allocated at all; you can safely pass null here).
 * Note that in case of multi-part files it only decompresses the first part (for now).
 * The user is responsible of freeing *outBuff when done.
 * The function returns 0 or a positive value if successful,
 * or a negative value in case of problems.
 * @param inFile   Input (.fapec) file to be decompressed.
 * @param outBuff  Output buffer allocated here, where we'll store the decompressed file
 * @param outSize  We return here the decompressed size (outBuff size)
 * @param userOpts User/runtime options
 * @param cmpCfg   FAPEC compression configuration. Freed here before returning.
 * @return Zero if successul, negative in case of errors, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_decomp_file2buff(char *inFile, unsigned char **outBuff, int64_t *outSize,
		int userOpts, void *cmpCfg) LINUX_VISIBILITY;


/**
 * Function for buffer-to-buffer decompression:
 * It takes an input buffer *inBuff of inSize bytes,
 * allocates the necessary output buffer *outBuff
 * (we know beforehand the decompressed size thanks to the FAPEC compressed headers),
 * decompresses inBuff into outBuff
 * (with the configuration contained in the compressed buffer,
 * or if "raw-only", with the one given in "cmpCfg"),
 * and returns such size through outSize.
 * Note that in case of multi-part files it only decompresses the first part (for now).
 * The user is responsible of freeing *outBuff when done.
 * The cmpCfg is already freed before returning.
 * It returns 0 or positive if OK, or negative in case of problems.
 * @param inBuff  Input (compressed) buffer
 * @param outBuff Output (decompressed) buffer, allocated here
 * @param inSize  Input (compressed size)
 * @param outSize Output size
 * @param userOpts User/runtime options
 * @param cmpCfg   FAPEC compression configuration. Freed here before returning.
 * @return Zero if successul, negative in case of errors, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_decomp_buff2buff(unsigned char *inBuff, unsigned char **outBuff,
		int64_t inSize, int64_t *outSize,
		int userOpts, void *cmpCfg) LINUX_VISIBILITY;



/**
 * Functions to create a self-contained structure with all the information of a
 * given ".fapec" archive: number of parts, list of parts (with their names,
 * sizes, etc), ...
 * These will allow obtaining information (e.g. a listing of the files and
 * folders contained in an archive), and also to e.g. decompress just some
 * given part.
 */



/**
 * Create an "info structure" from a given ".fapec" archive.
 * PLEASE NOTE: it allocates memory to store all this info, so it must be freed
 * when we are done (by calling fapec_free_archive_info_struct).
 * It returns a pointer to the "farchInfo" struct (FAPEC Archive Info, which is
 * actually called "fcsOpts" within the FAPEC code). In case of problems it
 * returns NULL.
 * userOpts allows to pass the user options (threads, overwrite output, etc) which
 * will then be used in case of actual operations (e.g. decompression).
 * In *ret we'll leave the error code in case of problems (i.e. NULL return),
 * or otherwise the size of the archive file.
 * Some useful "*ret" values in case of problems:
 * - License-protected file: -61
 * - Encrypted file (we should ask the user to enter a password): -100
 * - File given by *inFile not found / doesn't exist: -20
 * - Consistency error (perhaps you indicated a non-FAPEC file): -51
 * @param inFile   File (.fapec) to be examined or decompressed
 * @param userOpts User options
 * @param ret      Error code in case we return NULL
 * @return Pointer to the FAPEC Archive Info struct, or NULL in case of problems.
 */
WINDOWS_VISIBILITY void * fapec_get_archive_info_struct(char *inFile, int userOpts, int64_t *ret) LINUX_VISIBILITY;

/**
 * Same as fapec_get_archive_info_struct, but allowing to provide a decryption key.
 * @param inFile   File (.fapec) to be examined or decompressed
 * @param userOpts User options
 * @param ret      Error code in case we return NULL
 * @param key      The password to be used (to be provided here in plain text, as given by the user)
 * @return Pointer to the FAPEC Archive Info struct, or NULL in case of problems
 */
WINDOWS_VISIBILITY void * fapec_get_crypt_archive_info_struct(char *inFile, int userOpts, int64_t *ret, char *key) LINUX_VISIBILITY;

/**
 * Delete (free) the info struct from a given .fapec archive.
 * @param farchInfo  The pointer to the FAPEC Archive Info struct to be freed
 */
WINDOWS_VISIBILITY void fapec_free_archive_info_struct(void *farchInfo) LINUX_VISIBILITY;

/**
 * Activate the sockets-based logging and control subsystem.
 * @param farchInfo The FAPEC Archive options pointer.
 * @param sockPort  The port to be used (1-65535). We can pass 0 to explicitly tell that
 *                  we don't want sockets (that's the default behaviour).
 */
WINDOWS_VISIBILITY void fapec_set_sockport_archstruct(void *farchInfo, uint16_t sockPort) LINUX_VISIBILITY;

/**
 * Get the number of parts (files or folders) contained in a given ".fapec" archive,
 * from its info struct.
 * @param farchInfo The FAPEC Archive Info pointer.
 */
WINDOWS_VISIBILITY int fapec_get_farch_num_parts(void *farchInfo) LINUX_VISIBILITY;

/**
 * Get the name of a given 'part' contained in a FAPEC archive.
 * @param farchInfo The FAPEC Archive options pointer.
 * @param partNum   The part number (contained in the FAPEC Archive) for which we want to
 *                  obtain the name. It must obviously be between 0 and fapec_get_farch_num_parts()-1.
 * @return The part name string, or NULL in case of problems.
 */
WINDOWS_VISIBILITY char * fapec_get_part_name(void *farchInfo, int partNum) LINUX_VISIBILITY;

/**
 * Get the compressed size of a given 'part' contained in a FAPEC archive.
 * @param farchInfo The FAPEC Archive Info pointer.
 * @param partNum   The part number (contained in the FAPEC Archive) for which we want to
 *                  obtain the size. It must obviously be between 0 and fapec_get_farch_num_parts()-1.
 * @return The compressed part size, or -1 in case of problems.
 */
WINDOWS_VISIBILITY int64_t fapec_get_part_compsize(void *farchInfo, int partNum) LINUX_VISIBILITY;

/**
 * Get the original size of a given 'part' contained in a FAPEC archive.
 * @param farchInfo The FAPEC Archive Info pointer.
 * @param partNum   The part number (contained in the FAPEC Archive) for which we want to
 *                  obtain the size. It must obviously be between 0 and fapec_get_farch_num_parts()-1.
 * @return The compressed part size, or -1 in case of problems.
 */
WINDOWS_VISIBILITY int64_t fapec_get_part_origsize(void *farchInfo, int partNum) LINUX_VISIBILITY;

/**
 * Get the date (last modified time) of a given 'part' contained in a FAPEC archive.
 * It is returned in the form of a string with this format (in local time): "YYYY-MM-DD HH:MM"
 * The string is a static char, so please do not reuse (directly assign) the pointer; instead,
 * always copy its contents to your target string.
 * @param farchInfo The FAPEC Archive Info pointer.
 * @param partNum   The part number (contained in the FAPEC Archive) for which we want to
 *                  obtain the size. It must obviously be between 0 and fapec_get_farch_num_parts()-1.
 * @return The part date, or NULL in case of problems.
 */
WINDOWS_VISIBILITY char * fapec_get_part_date(void *farchInfo, int partNum) LINUX_VISIBILITY;

/**
 * Get a brief string describing the compression algorithm used for a given part.
 * @param farchInfo The FAPEC Archive Info pointer.
 * @param partNum   The part number (contained in the FAPEC Archive) for which we want to obtain the
 *                  compression algorithm info. It must obviously be between 0 and fapec_get_farch_num_parts()-1.
 * @return Static string with the compression algorithm (not to be freed), or NULL in case of problems.
 */
WINDOWS_VISIBILITY char * fapec_get_part_cmpalgo(void *farchInfo, int partNum) LINUX_VISIBILITY;

/**
 * Get a t_fapecPartCmpOpts struct describing the compression options used for a given part.
 * @param farchInfo   The FAPEC Archive Info pointer.
 * @param partNum     The part number (contained in the FAPEC Archive) for which we want to obtain the
 *                    compression algorithm info. It must obviously be between 0 and fapec_get_farch_num_parts()-1.
 * @param partCmpOpts The t_fapecPartCmpOpts structure where we'll leave all the information.
 * @return Negative in case of problems, or zero otherwise.
 */
WINDOWS_VISIBILITY int fapec_get_part_cmpopts(void *farchInfo, int partNum, t_fapecPartCmpOpts *partCmpOpts) LINUX_VISIBILITY;

/**
 * Set the output filename when extracting a given part from a FAPEC archive.
 * @param farchInfo   The FAPEC Archive Info pointer.
 * @param outFileName The file name (incl. path if needed) where the decompressed part will be stored.
 * @return Negative in case of problems, or zero otherwise.
 */
WINDOWS_VISIBILITY int fapec_set_part_outfname(void *farchInfo, char *outFileName) LINUX_VISIBILITY;

/**
 * Decompress just one specific part from a given FAPEC archive.
 * If both partName and partNum have valid values, we'll prioritize partName.
 * If none have valid values (i.e., NULL and -1), it will return with an error code.
 * The output file name will be the original part name (unless fapec_set_part_outfname() was called before).
 * @param farchInfo The FAPEC Archive options pointer.
 * @param partName  The name (as returned by fapec_get_part_name) of the part to be decompressed,
 *                  or NULL if we want to indicate it through partNum.
 * @param partNum   The part number to be decompressed (from 0 to fapec_get_farch_num_parts-1,
 *                  following the same order as returned by these functions), or -1 if we want
 *                  to indicate it through partName.
 * @return Zero if successful, error code (a negative value) in case of problems,
 *         1 when it's the last part of the archive, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_decomp_part_from_file(void *farchInfo, char *partName, int partNum) LINUX_VISIBILITY;

/**
 * Decompress just one specific part from a given FAPEC archive into a buffer.
 * It's equivalent to fapec_decomp_part_from_file().
 * @param farchInfo The FAPEC Archive options pointer.
 * @param partName  The name (as returned by fapec_get_part_name) of the part to be decompressed,
 *                  or NULL if we want to indicate it through partNum.
 * @param partNum   The part number to be decompressed (from 0 to fapec_get_farch_num_parts-1,
 *                  following the same order as returned by these functions), or -1 if we want
 *                  to indicate it through partName.
 * @param outBuff   The output buffer, internally allocated here (so it must be freed when done).
 * @param outSize   The size of the output buffer.
 * @return Zero if successful, error code (a negative value) in case of problems,
 *         1 when it's the last part of the archive, >=10 if warnings.
 */
WINDOWS_VISIBILITY int fapec_decomp_part_from_file_to_buff(void *farchInfo, char *partName, int partNum,
        unsigned char **outBuff, int64_t *outSize) LINUX_VISIBILITY;


#endif
