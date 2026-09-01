#!/bin/bash

if [ $# -eq 0 ]; then
    DATASET="INT"
    INPUT_DIR="../astronomical_images/$DATASET"
    OUTPUT_DIR="compressed_images_bash/$DATASET/ccsds123"
    mkdir -p "$OUTPUT_DIR"
    INPUT_FILES=("$INPUT_DIR"/*.raw)
elif [ $# -eq 1 ] && [ ! -f "$1" ]; then
    DATASET="$1"
    INPUT_DIR="../astronomical_images/$DATASET"
    OUTPUT_DIR="compressed_images_bash/$DATASET/ccsds123"
    mkdir -p "$OUTPUT_DIR"
    INPUT_FILES=("$INPUT_DIR"/*.raw)
else
    INPUT_FILES=("$@")
    OUTPUT_DIR="compressed_images_bash/${COMPRESS_DATASET:-INT}/ccsds123"
    mkdir -p "$OUTPUT_DIR"
fi

TEMP_DIR="temp_ccsds"
HEADER_TOOL="ccsds/bin/lcnl_header_tool"
ENCODER="ccsds/bin/lcnl_encoder"
DECODER="ccsds/bin/lcnl_decoder"

mkdir -p "$TEMP_DIR"

total_compressed_bytes=0
total_pixels=0
total_comp_time=0
total_decomp_time=0
total_memory=0
num_files=0
num_validation_errors=0

echo -e "Bytes\tBPS\tComp_Time\tDecomp_Time\tMemory\tValid\tFilename"

if [ ! -x "$HEADER_TOOL" ]; then
    echo "Error: $HEADER_TOOL not found or is not executable"
    exit 1
fi

if [ ! -x "$ENCODER" ]; then
    echo "Error: $ENCODER not found or is not executable"
    exit 1
fi

for input_file in "${INPUT_FILES[@]}"; do
    filename=$(basename "$input_file")
    base="${filename%.raw}"
    output_file="$OUTPUT_DIR/${base}.ccsds"
    header_file="$TEMP_DIR/${base}_header.bin"
    
    if [[ "$filename" =~ -1x([0-9]+)x([0-9]+)\.raw$ ]]; then
        height="${BASH_REMATCH[1]}"
        width="${BASH_REMATCH[2]}"
        pixels=$((width * height))
    else
        continue
    fi
    
    "$HEADER_TOOL" \
        -x large_n_x=$width \
        -x large_n_y=$height \
        -x large_n_z=1 \
        -x sample_type=0 \
        -x large_d=16 \
        -x entropy_coder_type=0 \
        -x gamma_star=8 \
        -x large_u_max=12 \
        "$header_file" > /dev/null 2>&1
    
    if [ ! -f "$header_file" ]; then
        continue
    fi
    
    time_output=$( (/usr/bin/time -f "%e %M" "$ENCODER" "$header_file" u16be "$input_file" "$output_file") 2>&1 | tail -1 )
    read comp_time memory <<< "$time_output"
    
    if [ -f "$output_file" ]; then
        compressed_size=$(stat -c%s "$output_file" 2>/dev/null || stat -f%z "$output_file" 2>/dev/null)
        bps=$(echo "scale=6; ($compressed_size * 8) / $pixels" | bc -l)
        
        decomp_start=$(date +%s.%N)
        temp_decomp="$TEMP_DIR/decomp_$$.raw"
        "$DECODER" "$output_file" u16be "$temp_decomp" > /dev/null 2>&1
        decomp_end=$(date +%s.%N)
        decomp_time=$(echo "$decomp_end - $decomp_start" | bc -l)
        
        if [ -f "$temp_decomp" ] && cmp -s "$input_file" "$temp_decomp"; then
            validation="OK"
        else
            validation="FAIL"
            ((num_validation_errors++))
        fi
        rm -f "$temp_decomp" "$header_file"
        
        echo -e "$compressed_size\t$bps\t${comp_time}s\t${decomp_time}s\t${memory}KB\t$validation\t$(basename "$output_file")"
        
        total_compressed_bytes=$((total_compressed_bytes + compressed_size))
        total_pixels=$((total_pixels + pixels))
        total_comp_time=$(echo "$total_comp_time + $comp_time" | bc -l)
        total_decomp_time=$(echo "$total_decomp_time + $decomp_time" | bc -l)
        total_memory=$((total_memory + memory))
        ((num_files++))
    fi
done

rm -rf "$TEMP_DIR"

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
