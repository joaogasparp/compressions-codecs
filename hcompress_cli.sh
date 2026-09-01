#!/bin/bash

if [ $# -eq 0 ]; then
    DATASET="INT"
    FITS_DIR="fits_preprocessed/$DATASET"
    OUTPUT_DIR="compressed_images_bash/$DATASET/hcompress"
    mkdir -p "$OUTPUT_DIR"
    if [ ! -d "$FITS_DIR" ]; then
        echo "Error: FITS directory $FITS_DIR not found"
        echo "Run: ./preprocess_to_fits.sh"
        exit 1
    fi
    INPUT_FILES=("$FITS_DIR"/*.fits)
elif [ $# -eq 1 ] && [ ! -f "$1" ]; then
    DATASET="$1"
    FITS_DIR="fits_preprocessed/$DATASET"
    OUTPUT_DIR="compressed_images_bash/$DATASET/hcompress"
    mkdir -p "$OUTPUT_DIR"
    if [ ! -d "$FITS_DIR" ]; then
        echo "Error: FITS directory $FITS_DIR not found"
        echo "Run: ./preprocess_to_fits.sh"
        exit 1
    fi
    INPUT_FILES=("$FITS_DIR"/*.fits)
else
    INPUT_FILES=()
    OUTPUT_DIR="compressed_images_bash/${COMPRESS_DATASET:-INT}/hcompress"
    mkdir -p "$OUTPUT_DIR"
    for input_file in "$@"; do
        if [[ "$input_file" == *.fits ]]; then
            fits_file="$input_file"
        else
            basename_raw=$(basename "$input_file")
            fits_root="${COMPRESS_FITS_DIR:-fits_preprocessed/INT}"
            fits_file="$fits_root/${basename_raw%.raw}.fits"
        fi
        if [ ! -f "$fits_file" ]; then
            echo "Error: FITS file $fits_file not found"
            echo "Run: ./preprocess_to_fits.sh"
            exit 1
        fi
        INPUT_FILES+=("$fits_file")
    done
fi

total_compressed_bytes=0
total_pixels=0
total_comp_time=0
total_decomp_time=0
total_memory=0
num_files=0
num_validation_errors=0

echo -e "Bytes\tBPS\tComp_Time\tDecomp_Time\tMemory\tValid\tFilename"

for fits_file in "${INPUT_FILES[@]}"; do
    [ -f "$fits_file" ] || continue
    
    filename=$(basename "$fits_file")
    base="${filename%.fits}"
    output_file="$OUTPUT_DIR/${base}.hcomp"
    
    if [[ "$filename" =~ -1x([0-9]+)x([0-9]+)\.fits$ ]]; then
        height="${BASH_REMATCH[1]}"
        width="${BASH_REMATCH[2]}"
        pixels=$((width * height))
    else
        continue
    fi
    
    time_output=$( (/usr/bin/time -f "%e %M" fpack -h "$fits_file") 2>&1 )
    read comp_time memory <<< "$time_output"
    
    compressed_fits="${fits_file}.fz"
    
    if [ -f "$compressed_fits" ]; then
        mv "$compressed_fits" "$output_file"
        compressed_size=$(stat -c%s "$output_file" 2>/dev/null || stat -f%z "$output_file" 2>/dev/null)
        bps=$(echo "scale=6; ($compressed_size * 8) / $pixels" | bc -l)
        
        decomp_start=$(date +%s.%N)
        temp_decomp="/tmp/hcompress_decomp_$$.fits"
        funpack -O "$temp_decomp" "$output_file" > /dev/null 2>&1
        decomp_end=$(date +%s.%N)
        decomp_time=$(echo "$decomp_end - $decomp_start" | bc -l)
        
        temp_orig_data="/tmp/hcompress_orig_data_$$.bin"
        temp_decomp_data="/tmp/hcompress_decomp_data_$$.bin"
        dd if="$fits_file" of="$temp_orig_data" bs=2880 skip=1 2>/dev/null
        dd if="$temp_decomp" of="$temp_decomp_data" bs=2880 skip=1 2>/dev/null
        
        if cmp -s "$temp_orig_data" "$temp_decomp_data"; then
            validation="OK"
        else
            validation="FAIL"
            ((num_validation_errors++))
        fi
        rm -f "$temp_decomp" "$temp_orig_data" "$temp_decomp_data"
        
        echo -e "$compressed_size\t$bps\t${comp_time}s\t${decomp_time}s\t${memory}KB\t$validation\t$(basename "$output_file")"
        
        total_compressed_bytes=$((total_compressed_bytes + compressed_size))
        total_pixels=$((total_pixels + pixels))
        total_comp_time=$(echo "$total_comp_time + $comp_time" | bc -l)
        total_decomp_time=$(echo "$total_decomp_time + $decomp_time" | bc -l)
        total_memory=$((total_memory + memory))
        ((num_files++))
    fi
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
