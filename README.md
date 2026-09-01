# 11 Compression Codecs

Collection of 11 lossless compression codecs for image compression.

## Codecs Included

1. **bzip2**
2. **gzip**
3. **lzma**
4. **zstd**
5. **fapec**
6. **ccsds123**
7. **jpeg2000**
8. **jpegls**
9. **jpegxl**
10. **hcompress**
11. **rice**

## Installation Requirements

### Debian/Ubuntu
```bash
sudo apt-get install zstd hcompress
```

### All Binaries
Pre-compiled binaries are included:
- FAPEC, CCSDS-123, JPEG 2000, JPEG-LS, JPEG XL
- Utility tools (raw_to_pgm, raw_to_fits)

## Usage

### Run All Codecs
```bash
./compress_all.sh /path/to/images [dataset_name]
```

### Run Single Codec
```bash
./bzip2_cli.sh /path/to/images/*.raw
./jpegls_cli.sh /path/to/images/*.raw
./jpegxl_cli.sh /path/to/images/*.raw
```

## Output
Compressed files are saved to:
```
compressed_images_bash/{dataset_name}/{codec_name}/
```

## Input Formats
- `.raw` - Raw binary pixel data (most codecs)
- `.fits` - FITS astronomical files (Rice codec)
- Any binary file (generic compressors)
