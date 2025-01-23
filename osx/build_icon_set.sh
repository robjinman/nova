#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <path_to_source_image>"
  exit 1
fi

SOURCE_IMAGE=$1
ICONSET_NAME="nova.iconset"
ICNS_FILE="nova.icns"

if [ ! -f "$SOURCE_IMAGE" ]; then
  echo "Error: Source image '$SOURCE_IMAGE' not found!"
  exit 1
fi

mkdir -p $ICONSET_NAME

ICON_SIZES=(16 32 64 128 256 512 1024)

for SIZE in "${ICON_SIZES[@]}"; do
  OUTPUT_FILE="$ICONSET_NAME/icon_${SIZE}x${SIZE}.png"
  echo "Generating $OUTPUT_FILE..."
  convert "$SOURCE_IMAGE" -resize ${SIZE}x${SIZE} "$OUTPUT_FILE"

  if [ "$SIZE" -lt 1024 ]; then
    OUTPUT_FILE_2X="$ICONSET_NAME/icon_${SIZE}x${SIZE}@2x.png"
    echo "Generating $OUTPUT_FILE_2X..."
    convert "$SOURCE_IMAGE" -resize $((SIZE * 2))x$((SIZE * 2)) "$OUTPUT_FILE_2X"
  fi

done

iconutil -c icns $ICONSET_NAME -o $ICNS_FILE

if [ $? -eq 0 ]; then
  echo "Successfully created $ICNS_FILE"
else
  echo "Failed to create .icns file"
  exit 1
fi

echo "Done."
