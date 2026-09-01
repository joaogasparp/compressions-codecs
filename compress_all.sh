#!/bin/bash


set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ $# -lt 1 ]; then
    echo "Usage: $0 /path/to/images [dataset_name]"
    echo ""
    echo "Examples:"
    echo "  $0 ~/pictures"
    echo "  $0 ~/pictures INT"
    exit 1
fi

INPUT_DIR="$1"
if [ $# -ge 2 ]; then
    DATASET="$2"
elif [ -d "$INPUT_DIR/INT" ]; then
    DATASET="INT"
else
    DATASET="$(basename "$INPUT_DIR")"
fi

if [ -d "$INPUT_DIR/INT" ]; then
    INPUT_ROOT="$INPUT_DIR"
    INPUT_DIR="$INPUT_DIR/$DATASET"
elif [ -d "$INPUT_DIR/$DATASET" ]; then
    INPUT_ROOT="$INPUT_DIR"
    INPUT_DIR="$INPUT_DIR/$DATASET"
else
    INPUT_ROOT="$INPUT_DIR"
fi

if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Directory not found: $INPUT_DIR"
    exit 1
fi

mapfile -t INPUT_FILES < <(find "$INPUT_DIR" -maxdepth 1 -type f \( -name "*.raw" -o -name "*.fits" \) -print | sort)
TOTAL_FILES="${#INPUT_FILES[@]}"
if [ "$TOTAL_FILES" -eq 0 ]; then
    echo "Error: No .raw or .fits files found in $INPUT_DIR"
    exit 1
fi

echo "Compressing images from: $INPUT_DIR"
echo "Dataset: $DATASET"
echo "Files: $TOTAL_FILES"
echo ""

OUTPUT_DIR="compressed_images_bash/$DATASET"
mkdir -p "$OUTPUT_DIR"

FITS_DIR="$INPUT_ROOT/$DATASET"
if [ ! -d "$FITS_DIR" ]; then
    FITS_DIR="$(dirname "$INPUT_ROOT")/compressors_others/fits_preprocessed/$DATASET"
fi

if [ ! -d "$FITS_DIR" ] || [ "$(find "$FITS_DIR" -maxdepth 1 -type f -name '*.fits' 2>/dev/null | wc -l)" -lt "$TOTAL_FILES" ]; then
    FITS_DIR="$SCRIPT_DIR/fits_preprocessed/$DATASET"
    mkdir -p "$FITS_DIR"
    for input_file in "${INPUT_FILES[@]}"; do
        case "$input_file" in
            *.raw)
                filename=$(basename "$input_file")
                if [[ "$filename" =~ -1x([0-9]+)x([0-9]+)\.raw$ ]]; then
                    height="${BASH_REMATCH[1]}"
                    width="${BASH_REMATCH[2]}"
                    fits_file="$FITS_DIR/${filename%.raw}.fits"
                    if [ ! -f "$fits_file" ]; then
                        "$SCRIPT_DIR/raw_to_fits" "$input_file" "$fits_file" "$width" "$height" > /dev/null 2>&1
                    fi
                fi
                ;;
        esac
    done
fi

if [ -f "fapec/fapeclic.dat" ]; then
    mkdir -p ~/.fapec
    cp -f fapec/fapeclic.dat ~/.fapec/ 2>/dev/null || true
fi

export LD_LIBRARY_PATH="$PWD/jpegxl/build/lib:${LD_LIBRARY_PATH:-}"

CODECS=("bzip2" "gzip" "fapec" "ccsds123" "jpeg2000" "jpegls" "hcompress" "rice" "lzma" "zstd" "jpegxl")

COMPLETED=0
FAILED=0

for codec in "${CODECS[@]}"; do
    script="${codec}_cli.sh"
    FILES_FOR_CODEC=("${INPUT_FILES[@]}")

    case "$codec" in
        bzip2) description="bzip2 -9" ;;
        gzip) description="gzip -9" ;;
        lzma) description="lzma -9" ;;
        zstd) description="zstd -19" ;;
        fapec) description="FAPEC -bl 512" ;;
        ccsds123) description="CCSDS-123 default" ;;
        jpeg2000) description="JPEG 2000 lossless" ;;
        jpegls) description="JPEG-LS T123_high (T1=8 T2=16 T3=32)" ;;
        jpegxl) description="JPEG XL -e 10 lossless" ;;
        hcompress) description="Hcompress (fpack -h)" ;;
        rice) description="Rice (fpack -r)" ;;
    esac

    if [ "$codec" = "hcompress" ] || [ "$codec" = "rice" ]; then
        mapfile -t FILES_FOR_CODEC < <(find "$FITS_DIR" -maxdepth 1 -type f -name '*.fits' -print | sort)
    fi
    
    if [ ! -f "$script" ]; then
        echo "[SKIP] $codec - script not found"
        ((FAILED++))
        continue
    fi
    
    echo "[RUN] $description"
    if COMPRESS_DATASET="$DATASET" COMPRESS_INPUT_DIR="$INPUT_DIR" COMPRESS_FITS_DIR="$FITS_DIR" ./$script "${FILES_FOR_CODEC[@]}" 2>&1; then
        COMPLETED=$((COMPLETED + 1))
    else
        echo "[FAIL] $codec"
        FAILED=$((FAILED + 1))
    fi
    echo ""
done

echo ""
echo "Done. $COMPLETED/$((COMPLETED + FAILED)) codecs completed"
echo "Output: $OUTPUT_DIR"

if [ "$FAILED" -eq 0 ]; then
    exit 0
else
    exit 1
fi

