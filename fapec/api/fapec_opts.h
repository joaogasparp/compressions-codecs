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
 * @file fapec_opts.h:
 *
 * @brief Definition of the functions to setup the FAPEC compression and
 * run-time configuration options, either for compression or decompression.
 *
 * FAPEC requires 2 main configuration elements:
 * - The user (or runtime) options, which are encoded into a single 32-bit
 *   integer. We provide here the functions to encode and decode it from/to
 *   the individual options.
 * - The compression configuration in itself, stored in a fapecOpts struct
 *   referenced by a pointer. It indicates the pre-processing stage to be
 *   used and its options, general compression options such as the chunk
 *   size, etc. We provide the functions to create and configure this.
 ****************************************************************************/


#ifndef _FAPECOPTS_H_
#define _FAPECOPTS_H_

#include <stdbool.h>
#include <stdint.h>


/**
 * Some constants to help setting up FAPEC and its configuration options:
 */

/* Verbosity */
#define FAPEC_VERB_NONE  0  /* Completely silent operation (just error/warning messages to stderr) */
#define FAPEC_VERB_BASIC 1  /* Basic output */
#define FAPEC_VERB_FULL  2  /* Progress messages */
#define FAPEC_VERB_DEBUG 3  /* Further progress messages and compression options listed */

/* Error Detection And Correction (EDAC) */
#define FAPEC_EDAC_NONE  0       /* No EDAC option */
#define FAPEC_EDAC_CKSUM 1       /* Data checksums */
#define FAPEC_EDAC_BASIC 2       /* Basic EDAC (headers redundancy and checksums) */
#define FAPEC_EDAC_BASICCKSUM 3  /* Basic EDAC + data checksums */

/* Encryption */
#define FAPEC_CRYP_NONE  0   /* No encryption */
#define FAPEC_CRYP_XXTEA 1   /* XXTEA */
#define FAPEC_CRYP_AES256 2  /* AES-256b through OpenSSL (not available by default, to avoid dependencies on the OpenSSL runtime) */

/* (De)compression mode */
#define UNFAPEC_MODE_NORMAL 0	/* Normal comp/decomp mode */
#define UNFAPEC_MODE_THUMB  1   /* Image decomp: get low-res image */
#define UNFAPEC_MODE_LIST   2   /* Decomp: Just get cmp file info */
#define UNFAPEC_MODE_CHECK  3   /* Comp: Check correctness of compressed chunk */

/* Minimum size (in bytes) of a FAPEC chunk.
 * This is for the compression configuration. Of course, if FAPEC
 * finds a tiny piece of data (just a few bytes) it will be able to
 * handle it anyway; this minimum limit aims at the automatic chunking
 * that FAPEC does on larger files (to minimize the metadata overhead) */
#define FAPEC_MIN_CHUNK_SIZE 1024        /* 1 KiB */
/* Maximum allowed size (in bytes) of a FAPEC chunk */
#define FAPEC_MAX_CHUNK_SIZE 469762048   /* 448 MiB */
/* Default chunk size, in bytes */
#define FAPEC_DEFAULT_CHUNK_SIZE 1048576 /* 1 MiB */

/* Minimum symbol size supported (in bits per sample) */
#define FAPEC_MIN_SYMSIZE 4
/* Maximum symbol size supported (in bits per sample) */
#define FAPEC_MAX_SYMSIZE 28

/* Minimum allowed size of the adaptive coding buffer of the FAPEC
 * coding core (that is, the FAPEC Block Length), in samples */
#define MIN_FAPEC_BUFF_LEN 32
/* Maximum allowed size of the adaptive coding buffer of the FAPEC
 * coding core (that is, the FAPEC Block Length), in samples */
#define MAX_FAPEC_BUFF_LEN 1024
/* Default/recommended FAPEC block length, in samples */
#define DEFAULT_FAPEC_BUFF_LEN 128

/* Max header (bypassed data) size supported (for both file/part and chunk), in bytes */
#define FAPEC_MAX_HEADER_BYTES 524294

/* Maximum image dimensions (either width or height), in pixels */
#define FAPEC_MAX_IMG_DIM 8396800
/* Maximum loss level for images */
#define FAPEC_MAX_IMG_LOSSLEVEL 16
/* Maximum loss level for images with CILLIC fixed rate */
#define FAPEC_MAX_IMG_CILLICFIXEDRATE_LOSSLEVEL 254

/* Maximum value for the Wave re-calibration period or length */
#define FAPEC_MAX_WAVE_TRPERLEN 1048576
/* Default length of the Wave re-calibration period or length */
#define FAPEC_DEFAULT_WAVE_TRPERLEN 8192
/* Default number of samples used in the Wave re-calibration */
#define FAPEC_DEFAULT_WAVE_TRNSMP 1024

/* Maximum interleaving (or shuffling) allowed, in samples */
#define FAPEC_MAX_INTERLEAVING 32775
/* Maximum interleaving capacity of FASEC (the simple and ultra-fast coding core alternative),
 * defining the size of an internal buffer and also how often FASEC coding is invoked.
 * It must be multiple of 4. */
#define FASEC_MAX_INTERLEAVING 256

/* Maximum number of colour/hyperspectral bands allowed */
#define FAPEC_MAX_NBANDS 32771


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
 * ***** Functions for the setup of general compression options: *****
 */


/**
 * Initialise the "FAPEC user configuration" code from the options provided.
 * The runtime options (threads, verbosity, etc.) and some compression options
 * (encryption, error-resiliency support, etc.) are encoded into a single 32-bit
 * integer. This function generates such code from the options provided.
 * Unless explicitly indicated, '1' means 'yes/true' and '0' means 'no/false'.
 * @param verbLevel Verbosity level (0-3, see FAPEC_VERB enums; 0 is silent except errors/warnings)
 * @param askOverwrite Ask before overwriting an existing output file
 * @param deleteInput Delete input when done
 * @param enforcePriv License-enforced privacy in compressed file
 * @param streamMode Streaming mode (compress/decompress from StdIn)
 * @param noAttr Do not (re)store file/folder attributes
 * @param noCompHead Avoid some headers in compressed output: 1 to avoid FCP header, 2 to even avoid FCS headers
 * @param edacOpt Error Detection And Correction option (0-3, see FAPEC_EDAC enums; recommended 3)
 * @param cryptOpt Encryption option (0-2, see FAPEC_CRYP enums; default 0)
 * @param threadPool Multi-thread pool (0-16)
 * @param cmpDecMode Compression or Decompression mode (0-3, see UNFAPEC_MODE enum; default 0)
 * @param noNames Do not store file/folder names in compressed output
 * @param noFoot Do not generate footers (incl. indexes) in compressed output
 * @param abortErr Abort in case of decompression errors (do not try to recover)
 * @param noRecurseDir Do not recurse subdirectories when compressing
 * @param keepLinks Do not follow symbolic links
 * @return A value with all options encoded in it.
 */
WINDOWS_VISIBILITY int fapec_usropts_new(int verbLevel, int askOverwrite, int deleteInput, int enforcePriv,
		int streamMode, int noAttr, int noCompHead,
		int edacOpt, int cryptOpt, int threadPool, int cmpDecMode,
		int noNames, int noFoot, int abortErr,
		int noRecurseDir, int keepLinks) LINUX_VISIBILITY;


/**
 * Initialise the "FAPEC user configuration" code from the options provided.
 * This is equivalent to fapec_usropts_new(), but only considering the options
 * applicable to chunk-level API compression.
 * @param verbLevel Verbosity level (0-3, see FAPEC_VERB enums; 0 is silent except errors/warnings)
 * @param edacOpt Error Detection And Correction option (0-3, see FAPEC_EDAC enums; recommended 3)
 * @param cryptOpt Encryption option (0-2, see FAPEC_CRYP enums; default 0)
 * @param cmpDecMode Compression or Decompression mode (0-3, see FAPEC_MODE and UNFAPEC_MODE enums; default 0)
 * @param abortErr Abort in case of decompression errors (do not try to recover)
 * @return A value with all options encoded in it.
 */
WINDOWS_VISIBILITY int fapec_usropts_new_ck(int verbLevel, int edacOpt, int cryptOpt, int cmpDecMode,
        int abortErr) LINUX_VISIBILITY;


/**
 * Get the "FAPEC user configuration" options from the code provided.
 * @param fapecUsrOpts The code with all user options.
 * @see fapec_usropts_new for the reset of parameters.
 */
WINDOWS_VISIBILITY void fapec_usropts_get(int fapecUsrOpts, int *verbLevel, int *askOverwrite, int *deleteInput,
		int *enforcePriv, int *streamMode, int *noAttr, int *noCompHead,
		int *edacOpt, int *cryptOpt, int *threadPool, int *cmpDecMode,
		int *noNames, int *noFoot, int *abortErr,
		int *noRecurseDir, int *keepLinks) LINUX_VISIBILITY;

/**
 * Update the log level in the user configuration variable.
 * @param fapecUsrOpts The variable with all user options, which will be updated with the new log level setting.
 * @param logLevel The log level to be set: 0 means completely silent (except warnings and errors),
 *                 1 means basic logging (e.g. at part level), 2 means progress logging (e.g. percentage),
 *                 3 means more exhaustive logging (e.g. part algorithm during decompression).
 */
WINDOWS_VISIBILITY void fapec_usropts_set_loglev(int *fapecUsrOpts, int logLev) LINUX_VISIBILITY;

/*
 * Determine and configure the right number of threads to be used:
 * Single-thread by default (and in case of single-core); if multi-core,
 * one thread per processor (either real core, or thread if hyperthreading);
 * if more than 8 cores available, just 8 computing threads (plus I/O).
 */
WINDOWS_VISIBILITY void fapec_usropts_autocfg_nthreads(int *fapecUsrOpts) LINUX_VISIBILITY;

/**
 * Update the number of threads in the user configuration variable.
 * @param fapecUsrOpts The variable with all user options, which will be updated with the new threadPool setting.
 * @param threadPool The threads to be used: 0 means single-thread, 1 means one thread for
 *        compression/decompression plus read/write threads, 2...62 means multi-thread for comp/decomp,
 *        and any negative value means automatic configuration (use one thread per processor seen, up to max. 8 threads).
 */
WINDOWS_VISIBILITY void fapec_usropts_set_nthreads(int *fapecUsrOpts, int threadPool) LINUX_VISIBILITY;

/**
 * Update the "delete input after successfully finishing" in the user configuration variable.
 * @param fapecUsrOpts The variable with all user options, which will be updated.
 * @param delInput True to delete the input file(s) when done, False to keep them.
 */
WINDOWS_VISIBILITY void fapec_usropts_set_delInput(int *fapecUsrOpts, bool delInput) LINUX_VISIBILITY;

/**
 * Update the "ask before overwriting" option in the user configuration variable.
 * @param fapecUsrOpts The variable with all user options, which will be updated.
 * @param askOverwrite True to interactively ask (through the console I/O) before overwriting an existing output file.
 */
WINDOWS_VISIBILITY void fapec_usropts_set_askOverwrite(int *fapecUsrOpts, bool askOverwrite) LINUX_VISIBILITY;

/**
 * Update the "do not recurse subdirectories" option (in compression) or "extract all files in
 *        the same working directory" option (in decompression) in the user configuration variable.
 * @param fapecUsrOpts The variable with all user options, which will be updated.
 * @param noDirTree True to AVOID recursing subdirectories / reconstructing original tree; False to keep the original path.
 */
WINDOWS_VISIBILITY void fapec_usropts_set_noDirTree(int *fapecUsrOpts, bool noDirTree) LINUX_VISIBILITY;

/**
 * Update the "license-enforced privacy" option in the user configuration variable.
 * @param fapecUsrOpts The variable with all user options, which will be updated.
 * @param enforcePriv True to set the license-privacy bit, False to generate a shareable archive.
 */
WINDOWS_VISIBILITY void fapec_usropts_set_enforcePriv(int *fapecUsrOpts, bool enforcePriv) LINUX_VISIBILITY;

/**
 * Update the "encrypt file when compressing" option in the user configuration variable.
 * Note that it requires to *interactively* provide the encryption key!
 * @param fapecUsrOpts The variable with all user options, which will be updated.
 * @param cryptOpt 0 to generate a non-encrypted archive; 1 to use XXTEA; 2 to use OpenSSL (if supported).
 */
WINDOWS_VISIBILITY void fapec_usropts_set_cryptOpt(int *fapecUsrOpts, int cryptOpt) LINUX_VISIBILITY;

/**
 * Update the "abort decompression in case of errors" option in the user configuration variable.
 * @param fapecUsrOpts The variable with all user options, which will be updated.
 * @param abortErr True to abort decompression at the very first error, False to
 *        try to recover a possibly corrupted archive (if supported by the license).
 */
WINDOWS_VISIBILITY void fapec_usropts_set_abortErr(int *fapecUsrOpts, bool abortErr) LINUX_VISIBILITY;


/**
 * Create a new FAPEC compression configuration structure (the so-called fapecOpts)
 * with a basic set of parameters:
 * Chunksize set to a default of 1MB, and auto-config activated.
 * Beware: HistogramBalancing is set to true by default; it is only needed in decompression;
 * if you need to force its deactivation (due to e.g. direct chunk-based decompression
 * function call on a compressed stream that was generated without HB), please call
 * fapec_cmpopts_sethb(fapecopts, false) after this; generally, it's never needed.
 * @return Pointer to the new FAPEC compression configuration options.
 * The user is responsible of freeing this pointer when done,
 * except when invoking the file-level API which already frees the options pointer.
 */

WINDOWS_VISIBILITY void* fapec_cmpopts_new() LINUX_VISIBILITY;


/**
 * Same as fapec_cmpopts_new(), but not allocating the fapecOpts pointer. Instead,
 * it takes a pointer (which may be a static variable) and just sets up its default contents.
 * @param fapecOpts         Pointer to the FAPEC options
 */
WINDOWS_VISIBILITY void fapec_cmpopts_setup(void *fapecOpts) LINUX_VISIBILITY;


/**
 * Set (activate) the "Sockets-based logging and control" option in the fapecOpts struct.
 * With this call, we tell FAPEC to initialize the sockets subsystem for progress monitoring and control.
 * Beware: as of FAPEC 23.0.1, the sockets subsystem is just available for Windows.
 * @param fapecOpts The FAPEC Options that we've created and set up with fapec_cmpopts_new and fapec_cmpopts_setup.
 * @param socketsPort The port to be used (1-65535). We can pass 0 to explicitly tell that
 *        we don't want sockets (that's the default behaviour).
 */
WINDOWS_VISIBILITY void fapec_cmpopts_set_sockets(void *fapecOpts, int socketsPort) LINUX_VISIBILITY;


/**
 * Set the common compression parameters in a fapecOpts configuration structure:
 * @param fapecOpts         Pointer to the FAPEC options
 * @param chunkSize         Size of the compression chunk, in bytes (max. 384MB)
 * @param headerOffset      File header to be bypassed, in bytes (max. 512K)
 * @param chunkOffset       Ditto for each chunk (max. 512K)
 * @param fapecBlockLength  Length of the FAPEC adaptiveness block, in samples (32-1024)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts, etc.), zero if successful.
 */
WINDOWS_VISIBILITY int fapec_cmpopts_setcommon(void *fapecOpts, int32_t chunkSize,
		uint32_t headerOffset, uint32_t chunkOffset, uint16_t fapecBlockLength) LINUX_VISIBILITY;

/**
 * For file decompression (and for compressed archives holding just 1 single file),
 * this function allows specifying a given output filename.
 * @param fapecOpts    Pointer to the FAPEC options
 * @param outFileName  String with the output file name to be used
 * @return Negative in case of problems (e.g. null fapecOpts or string), zero if successful.
 */
WINDOWS_VISIBILITY int fapec_cmpopts_setdecompfname(void *fapecOpts, char *outFileName) LINUX_VISIBILITY;

/**
 * Set the HistogramBalancing in the fapecOpts configuration.
 * This is only needed for decompression (in compression it's set at compilation time,
 * and typically it's always activated - except in very stringent and specific cases
 * such as satellite payloads).
 * When calling fapec_cmpopts_new you'll get HB activated by default; if you're sure that
 * you have to decompress a non-HB stream (and you only have a Headless FCC-level ICD compliance,
 * i.e. direct call to fapec_decomp_chunk), you must call this with 'false'.
 * Typically you don't need to call this.
 * @param fapecOpts  Pointer to the FAPEC options
 * @param hbSetting  False to decompress a non-HB stream, True otherwise.
 */
WINDOWS_VISIBILITY void fapec_cmpopts_sethb(void *fapecOpts, bool hbSetting) LINUX_VISIBILITY;

/**
 * Set the "Signed Integers" attribute in the fapecOpts configuration.
 * This is done separately (not in each of the "activate" functions below, as e.g. the
 * bigEndian option) partly for backwards compatibility, but specially because the Unsigned
 * (default) case is much more typical and thus it's the default.
 * @param fapecOpts  Pointer to the FAPEC options
 * @param useSigned  True for signed integers, false for unsigned.
 */
WINDOWS_VISIBILITY void fapec_cmpopts_setsignints(void *fapecOpts, bool useSigned) LINUX_VISIBILITY;

/**
 * Set the "Lossy" option in the fapecOpts configuration.
 * It allows to set a general lossy configuration, in case we wish to run in AutoConfig mode
 * but still activate losses (when possible, e.g. KMWCD, WCD, basic N-bit, etc).
 * For specific pre-proc activations, better to set the lossy level there.
 * @param fapecOpts  Pointer to the FAPEC options
 * @param lossy      Loss level (0-16).
 */
WINDOWS_VISIBILITY void fapec_cmpopts_setlossy(void *fapecOpts, int lossy) LINUX_VISIBILITY;

/**
 * Activate the generation of "basic analytics" or statistics from the compression process.
 * It will lead to a text file, with the same name than the .fapec archive but with a .fstats extension.
 * @param fapecOpts    Pointer to the FAPEC options
 * @param fapecUsrOpts The variable with all user options, which will be updated
 * @param statsfname   The file name where analytics will be stored. If NULL, it will
 *                     try to determine it from fapecOpts->fcpName (by changing .fapec by .fstats),
 *                     but if fapecOpts->fcpName is also NULL, it will return an error.
 * @param doStats      True to activate analytics.
 * @return Negative in case of problems, zero otherwise.
 */
WINDOWS_VISIBILITY int fapec_cmpopts_setstats(void *fapecOpts, int *fapecUsrOpts, char *statsfname, bool doStats) LINUX_VISIBILITY;

/**
 * Activate the "encrypt file when compressing" option,
 * and pass the encryption key as parameter.
 * If you want to just activate the encryption option without passing the key here (forcing to
 * do it interactively), you must use fapec_usropts_set_cryptOpt() instead.
 * @param fapecOpts    Pointer to the FAPEC options
 * @param fapecUsrOpts The variable with all user options, which will be updated
 * @param cryptOpt     0 to generate a non-encrypted archive; 1 to use XXTEA; 2 to use OpenSSL (if supported)
 * @param key          The string with the encryption key (which must be between 8 and 32 chars)
 * @return Negative in case of problems, zero otherwise.
 */
WINDOWS_VISIBILITY int fapec_cmpopts_setcrypt(void *fapecOpts, int *fapecUsrOpts, int cryptOpt, const char *key) LINUX_VISIBILITY;


/**
 * ***** Functions for the activation of one or another compression option: *****
 * (if you wish to use the file-based or buffer-based API in AutoConfig mode, then you don't need these;
 * if you wish to use the chunk-based API, then you need these for sure)
 */


/**
 * Select and configure the 'basic' (or 'delta') compression options, with/without interleaving,
 * with/without losses.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (4-28)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param interleaving  Interleaving, in samples (1-32K)
 * @param lossy         Lossy level (0 for lossless, up to symSize-1 for lossy)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_basic(void *fapecOpts, int8_t symSize, bool bigEndian,
		uint16_t interleaving, uint8_t lossy) LINUX_VISIBILITY;

/*
 * Select and configure the 'FASEC' compression options, with/without interleaving.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (8 or 16)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param interleaving  Interleaving, in samples (1-256)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_fasec(void *fapecOpts, int8_t symSize, bool bigEndian,
        uint16_t interleaving) LINUX_VISIBILITY;

/**
 * Select and configure the 'filter' compression options, with/without interleaving.
 * Lossy compression is not supported here.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (4-28)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param interleaving  Interleaving, in samples (1-32K)
 * @param filterOrder   Filter order (2-4)
 * @return negative in case of problems (e.g. invalid options, null fapecOpts, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_filter(void *fapecOpts, int8_t symSize, bool bigEndian,
		uint16_t interleaving, uint8_t filterOrder) LINUX_VISIBILITY;

/**
 * Select and configure the 'wave' compression options.
 * @deprecated PLEASE USE fapec_cmpopts_activate_wave2() INSTEAD.
 *
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (4-28)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param interleaving  Interleaving, in samples (1-32K)
 * @param lossy         Lossy level (0 for lossless, up to 16 for lossy)
 * @param filterOrder   Filter order (4-10)
 * @param trperiod      Training period (16-8M)
 * @param trlen         Training length (16-8M)
 * @return negative in case of problems (e.g. invalid options, null fapecOpts, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_wave(void *fapecOpts, int8_t symSize, bool bigEndian,
        uint16_t interleaving, uint8_t lossy, uint8_t filterOrder,
        uint32_t trperiod, uint32_t trlen) LINUX_VISIBILITY;

/**
 * Select and configure the 'wave2' compression options.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (4-28)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param interleaving  Interleaving, in samples (1-32K)
 * @param lossy         Lossy level (0 for lossless, up to 16 for lossy)
 * @param filterOrder   Filter order (2-16)
 * @param coupleChann   True to couple channels (and thus reuse LPCs for all channels), false for separate LPCs per channel
 * @param rectWave      True to rectify the wave in the prediction
 * @param trperiod      Training period (16-8M)
 * @param trlen         Training length (16-8M)
 * @param snr           SNR threshold for Smart Lossy (x100, so 1 is 0.01 and 50000 is 500.00; 0 deactivates Smart Lossy)
 * @param sigBits       Target bits for signal periods in Smart Lossy (-1 for none, up to 17 for lossless)
 * @param noiBits       Target bits for noise periods in Smart Lossy (-1 for none, up to 17 for lossless)
 * @param minBits       Minimum bits regardless of SNR in Smart Lossy (0 for none, up to 16 for lossless)
 * @return negative in case of problems (e.g. invalid options, null fapecOpts, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_wave2(void *fapecOpts, int8_t symSize, bool bigEndian,
        uint16_t interleaving, uint8_t lossy, uint8_t filterOrder,
        bool coupleChann, bool rectWave,
        uint32_t trperiod, uint32_t trlen,
		uint16_t snr, int8_t sigBits, int8_t noiBits, int8_t minBits) LINUX_VISIBILITY;

/**
 * Select and configure the 'float' compression options.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param interleaving  Interleaving, in samples (1-32K)
 * @param lossy         Lossy level (0 for lossless, up to 23 for lossy)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_floats(void *fapecOpts, bool bigEndian,
		uint16_t interleaving, uint8_t lossy) LINUX_VISIBILITY;

/**
 * Select and configure the 'double' compression options.
 * NOTE that lossy is not implemented yet for doubles, so the 'lossy' parameter has no effect for now.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param interleaving  Interleaving, in samples (1-32K)
 * @param lossy         Lossy level (0 for lossless, up to 30 for lossy)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_doubles(void *fapecOpts, bool bigEndian,
		uint16_t interleaving, uint8_t lossy) LINUX_VISIBILITY;

/**
 * Select and configure the 'text' (or 'FAPECLZ') compression options.
 * @param fapecOpts  Pointer to the FAPEC options
 * @param cmpLevel   Compression level (0=fast, 9=best)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_text(void *fapecOpts, int8_t cmpLevel) LINUX_VISIBILITY;

/**
 * Select and configure the LZW compression options.
 * @param fapecOpts  Pointer to the FAPEC options
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_lzw(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Select and configure the 'tabulated text' compression options.
 * The field separator(s) must be indicated with fieldSep1 and 2
 * (if there's just 1 separator then it must be passed to both parameters,
 * that is, fieldSep1 = fieldSep2 = our separator).
 * @param fapecOpts  Pointer to the FAPEC options
 * @param fieldSep1  Field separator
 * @param fieldSep2  Second field separator (it must be equal to fieldSep1 if there's only one)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_tabtxt(void *fapecOpts, char fieldSep1,
		char fieldSep2) LINUX_VISIBILITY;

/**
 * Select and configure the 'FastQ' (genomics) compression options.
 * @param fapecOpts  Pointer to the FAPEC options
 * @return Negative in case of problems (null fapecOpts or
 * unsupported by license)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_fastq(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Select and configure the 'KWCD' (watercolumn) compression options.
 * @param fapecOpts  Pointer to the FAPEC options
 * @param lossy      Lossy level (0 for lossless, up to 7 for lossy)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_kwcd(void *fapecOpts, uint8_t lossy) LINUX_VISIBILITY;

/**
 * Select and configure the 'Kall' (bathymetry, ".all" files) compression options.
 * @param fapecOpts  Pointer to the FAPEC options
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_kall(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Select and configure the 'kmall' (new Kongsberg Maritime files, incl. ".kmwcd" files)
 * compression options. Additional options allow to select the lossless mode (value 0),
 * or if nonzero, different lossy levels, for the different elements of MRZ (bathymetry)
 * and MWC (watercolumn) datagrams.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param mrzSndLossy   [0-7] Loss level for MRZ Soundings (max. 4 recommended)
 * @param mrzSILossy    [0-16] Loss level for MRZ Seabed Image
 * @param mwcAmpLossy   [0-16] Loss level for MWC (watercolumn) amplitude samples
 * @param mwcPhLossy    [0-16] Loss level for MWC (watercolumn) phase samples
 * @param mwcSmartLossy [0-16] Aggressiveness in removing 'useless' MWC samples
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_kmall(void *fapecOpts,
        uint8_t mrzSndLossy, uint8_t mrzSILossy,
        uint8_t mwcAmpLossy, uint8_t mwcPhLossy, uint8_t mwcSmartLossy) LINUX_VISIBILITY;

/**
 * Select and configure the 'SRODOC' compression options (reserved).
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_srodoc(void *fapecOpts,
		uint8_t iqloss, uint8_t iqOccExtraLoss,
		uint8_t rstTimeLoss, uint8_t rstDoppLoss) LINUX_VISIBILITY;

/**
 * Select and configure the 'KIQDOC' compression options (reserved).
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_kiqdoc(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Select and configure the 'multi-band prediction-based image' compression options.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (4-28)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param nBands        Number of bands (1 for monochrome/greyscale, up to 32K bands)
 * @param lossy         Lossy level (0 for lossless, up to symSize-1 for lossy)
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_multibandpredimg(void *fapecOpts, int8_t symSize,
		bool bigEndian, uint16_t nBands, uint8_t lossy) LINUX_VISIBILITY;

/**
 * Select and configure the 'no pre-proc' compression options.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (4-28)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param dataFormat    -1 for twos-complement, 1 for sign bit in MSB, 2 for unsigned values
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_nopreproc(void *fapecOpts, int8_t symSize,
		bool bigEndian, int8_t dataFormat) LINUX_VISIBILITY;

/**
 * Select and configure the 'DWT image' compression options.
 * @param fapecOpts     Pointer to the FAPEC options
 * @param symSize       Symbol size, in bits (4-28)
 * @param bigEndian     True for Big Endian data, False for Little Endian data
 * @param imageWidth    Image width (pixels), up to 8.3M
 * @param imageHeight   Image height (pixels), 1 to force 1D-DWT, or up to 8.3M
 * @param nBands        Number of bands (1 for monochrome/greyscale, up to 32K bands)
 * @param lossy         Lossy level (0 for lossless, up to symSize-1 for lossy)
 * @param realBpp       Actual number of bits per pixel
 * @param bandsCoding   Multi-band coding: 0=BIP, 1=BIL (unsupported), 2=BSQ, 3=Bayer
 * @return Negative in case of problems (e.g. invalid options, null fapecOpts,
 * unsupported by license, etc.)
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_dwtimg(void *fapecOpts, int8_t symSize, bool bigEndian,
		uint32_t imageWidth, uint32_t imageHeight, uint16_t nBands, uint8_t lossy,
		int8_t realBpp, int8_t bandsCoding) LINUX_VISIBILITY;

/**
 * Select and configure the 'HPA image' compression options.
 * @see fapec_cmpopts_activate_dwtimg for parameters.
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_hpaimg(void *fapecOpts, int8_t symSize, bool bigEndian,
		uint32_t imageWidth, uint32_t imageHeight, uint16_t nBands, uint8_t lossy,
		int8_t realBpp, int8_t bandsCoding) LINUX_VISIBILITY;

/**
 * Select and configure the 'CILLIC image' compression options.
 * @see fapec_cmpopts_activate_dwtimg for parameters.
 * Besides, here we have:
 * @param lev   Level of multi-band adaptiveness: 0 to force spatial-only decorrelator
 *              for band 2 onwards, 1 to force spectral-only, 2 to force the mixed one,
 *              and 3-7 to indicate different sizes of the testing sub-block (from
 *              4x4 to 12x12 pixels, thus meaning a slower operation for high levels
 *              but leading to better estimates and thus better ratios).
 * This function activates the lossless or near-lossless (fixed-quality) algorithm.
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_cillicimg(void *fapecOpts, int8_t symSize, bool bigEndian,
		uint32_t imageWidth, uint32_t imageHeight, uint16_t nBands, uint8_t lossy,
		int8_t realBpp, int8_t bandsCoding, int8_t lev) LINUX_VISIBILITY;

/**
 * Select and configure the 'CILLIC image' compression options for fixed rate.
 * @see fapec_cmpopts_activate_dwtimg for parameters.
 */
WINDOWS_VISIBILITY int fapec_cmpopts_activate_cillicimgfixedrate(void *fapecOpts, int8_t symSize, bool bigEndian,
		uint32_t imageWidth, uint32_t imageHeight, uint16_t nBands, float lossy, 
		int8_t realBpp, int8_t bandsCoding, int8_t lev) LINUX_VISIBILITY;


/**
 * ***** Functions related to logging and errors: *****
 */


/**
 * Set the log level.
 * @param fapecOpts    Pointer to the FAPEC options
 * @param logLevel     The log level: 0=silent, 1=warn+error, 2=info, 3=debug
 */
WINDOWS_VISIBILITY void fapec_set_loglevel(void *fapecOpts, int logLevel) LINUX_VISIBILITY;

/**
 * Set the log level from a FAPEC user options code.
 * @param fapecOpts    Pointer to the FAPEC options
 * @param usrOpts      The FAPEC user options code generated by e.g. fapec_usropts_new()
 */
WINDOWS_VISIBILITY void fapec_set_loglevel_uoc(void *fapecOpts, int usrOpts) LINUX_VISIBILITY;

/**
 * Allocate and activate the internal logger.
 * Then, on each logging call (info, errors, etc), FAPEC will internally
 * check if it's ready; if so, logs will be accumulated there instead of
 * being output to the console. It will then be responsibility of the
 * user to log those accumulated logs (accessible through the pointer returned
 * by this function) after the FAPEC main API invocation, passing those to your
 * appropriate logging method, and calling fapec_free_loggers() when done.
 * Note that the socket-based messaging will continue working regardless
 * of this logger configuration.
 * fapec_set_loglevel() or fapec_set_loglevel_uoc() should have been called before.
 * @return Pointer to the internal logging buffer (or NULL in case of problems)
 */
WINDOWS_VISIBILITY char *fapec_setup_logger() LINUX_VISIBILITY;

/**
 * Free the internal logger (string buffer).
 * @param loggerPtr Pointer to the internal logging buffer (returned by fapec_setup_logger)
 */
WINDOWS_VISIBILITY void fapec_free_logger(char *loggerPtr) LINUX_VISIBILITY;

/**
 * Check if some error occurred during the FAPEC execution.
 * @return True/nonzero if errors happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_errs() LINUX_VISIBILITY;

/**
 * Check if some warning occurred during the FAPEC execution.
 * @return True/nonzero if warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_warns() LINUX_VISIBILITY;

/**
 * Get all the error and warning codes accumulated during the execution of FAPEC
 * (at any level: library or CLI).
 * It returns zero if no error or warning occurred. Otherwise, it returns a
 * 32-bit bitwise code (a signed integer), with the MSB or sign bit (mask 0x80000000)
 * always set (indicating that an error or warning happened), and the rest of bits
 * set depending on the exact causes.
 * If the code returned is exactly 0x80000000, it means a 'generic' or undefined error.
 * Fatal errors lead to the 2nd MSB to be set (0x40000000).
 * This is the full list of possible errors and bit masks for the rest:
 * - 0x000001: command-line
 * - 0x000002: I/O read
 * - 0x000004: I/O write
 * - 0x000008: I/O seek
 * - 0x000010: File attribute get/set
 * - 0x000020: Memory allocation
 * - 0x000040: File format
 * - 0x000080: Checksum
 * - 0x000100: Consistency check
 * - 0x000200: Sync
 * - 0x000400: PLID
 * - 0x000800: License
 * - 0x001000: Too large chunk
 * - 0x002000: Image/chunk consistency
 * - 0x004000: Chunk (de)compression
 * - 0x008000: Buffer (de)compression
 * - 0x010000: File (de)compression
 * - 0x020000: Thread creation
 * - 0x040000: Thread joining
 * - 0x080000: Encryption
 * @param fapecOpts     Pointer to the FAPEC options
 * @return The bitwise code indicating all errors or warnings that have happened.
 */
WINDOWS_VISIBILITY int fapec_get_accum_errcode(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Get the basic error text (or type of error) associated to a specific error code
 * (as provided by functions returning error codes, NOT as provided by fapec_get_accum_errcode).
 * @param Error code returned by some of the FAPEC API functions
 * @return Error text
 */
WINDOWS_VISIBILITY const char * fapec_get_errtext(int errCode) LINUX_VISIBILITY;

/**
 * Check if an error or warning related to I/O has happened.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if I/O errors/warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_io(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Check if an error or warning related to memory handling has happened.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if mem errors/warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_mem(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Check if an error or warning related to data corruption has happened,
 * such as unexpected/invalid data format, checksum error, consistency check
 * failure, data sync failure, etc.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if errors/warnings related to data corruption happened,
 *         False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_corrup(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Check if an error or warning related to licensing has happened.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if licensing errors/warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_lic(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Check if an error or warning related to multi-threading has happened.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if multi-threading errors/warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_thread(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Check if an error or warning related to data encryption/decryption has happened.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if en/decryption errors/warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_crypt(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Check if an error or warning related to configuration has happened,
 * such as command-line, too large chunk, image chunking, etc.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if I/O errors/warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_cfg(void *fapecOpts) LINUX_VISIBILITY;

/**
 * Check if a fatal error or warning has happened.
 * @param fapecOpts     Pointer to the FAPEC options
 * @return True/nonzero if fatal errors/warnings happened, False/zero otherwise.
 */
WINDOWS_VISIBILITY bool fapec_has_err_fatal(void *fapecOpts) LINUX_VISIBILITY;


/**
 * ***** Functions to obtain information on the FAPEC library, options supported, license...: *****
 */


/**
 * Obtain the license type:
 * -1 = Invalid or none, 0 = Evaluation (typ. 30 days), 1 = Full (typ. 1 year),
 *  2 = Unlimited, 3 = (reserved), 4 = Restricted (some limited functionalities),
 *  5 = Full Decompression-Only
 * @return The license type code (-1 to 5, see above).
 */
WINDOWS_VISIBILITY int fapec_get_lic_type() LINUX_VISIBILITY;

/**
 * In case of evaluation or full (time-limited) license, get the remaining days.
 * @return Remaining license days.
 */
WINDOWS_VISIBILITY int fapec_get_eval_lic_rem_days() LINUX_VISIBILITY;

/**
 * Get the license owner name.
 * @return License owner text.
 */
WINDOWS_VISIBILITY const char *fapec_get_lic_owner() LINUX_VISIBILITY;

/**
 * Get the FAPEC version string (e.g., "23.0.0", "23.0 Beta", etc).
 * @return FAPEC version text.
 */
WINDOWS_VISIBILITY const char *fapec_get_version() LINUX_VISIBILITY;

/**
 * Get the FAPEC release date as an ISO date string ("YYYY-MM-DD").
 * @return FAPEC release date text.
 */
WINDOWS_VISIBILITY const char *fapec_get_rel_date() LINUX_VISIBILITY;

/**
 * Function to "test" (activate=false) or actually use (activate=true) a
 * license file given by the user (licfname).
 * It returns the same code than fapec_license_init (i.e. negative if problems),
 * and stores the info in licType, remDays and owner.
 * The string in 'owner' must be preallocated or static with at least 64 bytes.
 * If activate=false, we take care of NOT overwriting the "active" license.
 * Return value:
 *   0: all OK
 *  -1: no license was found.
 *  -2: license is invalid/malformed/corrupted.
 *  -3: license is for evaluation and has expired.
 *  -4: license is limited/full and has expired.
 * @param licfname The path+name of the license file to test or use
 * @param activate True to actually use the license file, false to just test it
 * @param licType  It will be updated with the license type code
 * @param remDays  It will be updated with the remaining license days
 * @param owner    It will be updated by copying here the license owner text
 * @return 0 if all OK, negative otherwise (see above)
 */
WINDOWS_VISIBILITY int fapec_test_or_use_lic_file(char *licfname, bool activate,
        int *licType, int *remDays, char *owner) LINUX_VISIBILITY;


#endif
