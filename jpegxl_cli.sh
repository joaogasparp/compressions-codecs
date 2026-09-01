#!/bin/bash


if [ $# -eq 0 ]; then
    DATASET="INT"
    INPUT_DIR="../astronomical_images/$DATASET"
    OUTPUT_DIR="compressed_images_bash/$DATASET/jpegxl"
    mkdir -p "$OUTPUT_DIR"
    INPUT_FILES=("$INPUT_DIR"/*.raw)
elif [ $# -eq 1 ] && [ ! -f "$1" ]; then
    DATASET="$1"
    INPUT_DIR="../astronomical_images/$DATASET"
    OUTPUT_DIR="compressed_images_bash/$DATASET/jpegxl"
    mkdir -p "$OUTPUT_DIR"
    INPUT_FILES=("$INPUT_DIR"/*.raw)
else
    INPUT_FILES=("$@")
    OUTPUT_DIR="compressed_images_bash/${COMPRESS_DATASET:-INT}/jpegxl"
    mkdir -p "$OUTPUT_DIR"
fi

TEMP_DIR="temp_pgm"
JPEGXL_BIN="jpegxl/build/tools/cjxl"
JPEGXL_DECOMP="jpegxl/build/tools/djxl"
RAW_TO_PGM="./raw_to_pgm"

mkdir -p "$TEMP_DIR"

if [ ! -x "$JPEGXL_BIN" ]; then
    echo "Error: $JPEGXL_BIN not found. Build with:"
    echo "  cd jpegxl && mkdir -p build && cd build"
    echo "  cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF .."
    echo "  cmake --build . -- -j\$(nproc)"
    exit 1
fi

if [ ! -x "$JPEGXL_DECOMP" ]; then
    echo "Error: $JPEGXL_DECOMP not found."
    exit 1
fi

if [ ! -x "$RAW_TO_PGM" ]; then
    echo "Error: $RAW_TO_PGM not found. Build with: gcc -Wall -O3 -o raw_to_pgm raw_to_pgm.c"
    exit 1
fi

total_compressed_bytes=0
total_pixels=0
total_comp_time=0
total_decomp_time=0
total_memory=0
num_files=0
num_validation_errors=0

echo -e "Bytes\tBPS\tComp_Time\tDecomp_Time\tMemory\tValid\tFilename"

for input_file in "${INPUT_FILES[@]}"; do
    filename=$(basename "$input_file")
    base="${filename%.raw}"
    temp_pgm="$TEMP_DIR/${base}.pgm"
    output_file="$OUTPUT_DIR/${base}.jxl"
    
    if [[ "$filename" =~ -1x([0-9]+)x([0-9]+)\.raw$ ]]; then
        height="${BASH_REMATCH[1]}"
        width="${BASH_REMATCH[2]}"
        pixels=$((width * height))
    else
        continue
    fi
    
    "$RAW_TO_PGM" "$input_file" "$temp_pgm" "$width" "$height" > /dev/null 2>&1
    
    if [ ! -f "$temp_pgm" ]; then
        rm -f "$temp_pgm"
        continue
    fi
    
    time_output=$( (/usr/bin/time -f "%e %M" "$JPEGXL_BIN" \
        "$temp_pgm" "$output_file" \
        -d 0 \
        -e 10 \
        -m 1 \
        -g 3 \
        -P 15 \
        -I 50 \
        --quiet) 2>&1 | tail -1 )
    read comp_time memory <<< "$time_output"
    
    if [ -f "$output_file" ]; then
        compressed_size=$(stat -c%s "$output_file" 2>/dev/null || stat -f%z "$output_file" 2>/dev/null)
        bps=$(echo "scale=6; ($compressed_size * 8) / $pixels" | bc -l)
        
        decomp_start=$(date +%s.%N)
        temp_decomp="/tmp/jpegxl_decomp_$$.pgm"
        "$JPEGXL_DECOMP" "$output_file" "$temp_decomp" --quiet > /dev/null 2>&1
        decomp_end=$(date +%s.%N)
        decomp_time=$(echo "$decomp_end - $decomp_start" | bc -l)
        
        if cmp -s "$temp_pgm" "$temp_decomp"; then
            validation="OK"
        else
            validation="FAIL"
            ((num_validation_errors++))
        fi
        rm -f "$temp_decomp"
        
        echo -e "$compressed_size\t$bps\t${comp_time}s\t${decomp_time}s\t${memory}KB\t$validation\t$(basename "$output_file")"
        
        total_compressed_bytes=$((total_compressed_bytes + compressed_size))
        total_pixels=$((total_pixels + pixels))
        total_comp_time=$(echo "$total_comp_time + $comp_time" | bc -l)
        total_decomp_time=$(echo "$total_decomp_time + $decomp_time" | bc -l)
        total_memory=$((total_memory + memory))
        ((num_files++))
    fi
    
    rm -f "$temp_pgm"
done

if [ $total_pixels -gt 0 ]; then
    avg_bps=$(echo "scale=6; ($total_compressed_bytes * 8) / $total_pixels" | bc -l)
else
    avg_bps=0
fi

if [ $num_files -gt 0 ]; then
    avg_comp_time=$(echo "scale=3; $total_comp_time / $num_files" | bc -l)
    avg_decomp_time=$(echo "scale=3; $total_decomp_time / $num_files" | bc -l)
    avg_memory=$(echo "$total_memory / $num_files" | bc)
else
    avg_comp_time=0
    avg_decomp_time=0
    avg_memory=0
fi
echo ""
echo "$avg_bps $avg_comp_time $avg_decomp_time $avg_memory"
if [ $num_validation_errors -gt 0 ]; then
    echo "WARNING: $num_validation_errors validation errors!" >&2
fi

rmdir "$TEMP_DIR" 2>/dev/null
