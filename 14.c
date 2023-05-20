#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
//    Bitmap File Header
    char Signature[2];
    uint32_t FileSize;
    uint16_t Reserved1;
    uint16_t Reserved2;
    uint32_t DataOffset;

//    DIB Header
    uint32_t DIBHeaderSize;
    int32_t ImageWidth;
    int32_t ImageHeight;
    uint16_t Planes;
    uint16_t BitsPerPixel;
    uint32_t Compression;
    uint32_t ImageSize;
    int32_t x_PixelsPerMeter;
    int32_t y_PixelsPerMeter;
    uint32_t ColorsUsed;
    uint32_t ColorsImportant;
} BMPHeader;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} RGB24;

typedef struct {
    uint8_t buffer[54];
} BMPHeaderDefault;
#pragma pack(pop)

BMPHeader *getHeaderData(FILE *fptr) {
    BMPHeader *header = malloc(sizeof(BMPHeader));

    fread(header, sizeof(BMPHeader), 1, fptr);

    return header;
}

BMPHeaderDefault *getDefaultHeader(FILE *fptr) {
    BMPHeaderDefault *header = malloc(sizeof(BMPHeaderDefault));

    fread(header, sizeof(BMPHeaderDefault), 1, fptr);

    return header;
}

int getPaddingBytes(int width) {
    if ((width * 3) % 4 != 0) {
        return 4 - ((width * 3) % 4);
    } else {
        return 0;
    }
}

RGB24 **getImageData(FILE *fptr, int width, int height, int padding) {
    uint8_t buffer[padding];

    RGB24 **img = malloc(height * sizeof(RGB24 *));

    for (int i = 0; i < height; ++i) {
        img[i] = malloc(width * sizeof(RGB24));

        fread(img[i], sizeof(RGB24), width, fptr);

//        skipping padding
        if (padding > 0) {
            fread(buffer, sizeof(uint8_t), padding, fptr);
        }
    }

    return img;
}

int **getInitialPlacement(int width, int height, RGB24 **img) {
//    image to 2d placement array
    int **initialPlacement = malloc(height * sizeof(int *));

    for (int y = 0; y < height; ++y) {
        initialPlacement[y] = malloc(width * sizeof(int));

        for (int x = 0; x < width; ++x) {

            RGB24 pixel = img[y][x];

//            1 or 0 - alive or dead
            if (pixel.blue == 0 && pixel.green == 0 && pixel.red == 0) {
                initialPlacement[y][x] = 1;
            } else {
                initialPlacement[y][x] = 0;
            }
        }
    }

    return initialPlacement;
}

int getNeighbours(int **a, int width, int height, int x, int y) {
    int neighbourCount = 0;

//    checking 8 neighbours
    for (int yDelta = -1; yDelta <= 1; ++yDelta) {
        for (int xDelta = -1; xDelta <= 1; ++xDelta) {
//            skipping itself
            if (yDelta == 0 && xDelta == 0) {
                continue;
            }

            int xPosition = (x + yDelta + width) % width;
            int yPosition = (y + xDelta + height) % height;

//            making sure checking is viable
            if ((xPosition >= 0 && xPosition < width) && (yPosition >= 0 && yPosition < height)) {
//                in case neighbour is alive
                if (a[yPosition][xPosition] == 1) {
                    ++neighbourCount;
                }
            }
        }
    }

    return neighbourCount;
}

int **gameOfLife(int **initialPlacement, int width, int height) {
    int neighbours;
    int current;

//    creating new position array
    int **newPlacement = malloc(height * sizeof(int *));

    for (int i = 0; i < height; ++i) {
        newPlacement[i] = malloc(width * sizeof(int));
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            current = initialPlacement[y][x];
            neighbours = getNeighbours(initialPlacement, width, height, x, y);

            // Any live cell with fewer than two live neighbours dies
            // Any live cell with two or three live neighbours lives
            // Any live cell with more than three live neighbours dies
            // Any dead cell with three live neighbours becomes a live cell

            if (current == 1 && (neighbours == 2 || neighbours == 3)) {
                current = 1;
            } else if (current == 0 && neighbours == 3) {
                current = 1;
            } else {
                current = 0;
            }

            newPlacement[y][x] = current;
        }
    }

    return newPlacement;
}

void writeBMP(int **placement, int width, int height, int padding, const void *initialHeader, char *filename) {
//    opening the file
    FILE *newFile = fopen(filename, "wb");
    if (newFile == NULL) {
        printf("Error opening file to write");
    }

//    writing header
    fwrite(initialHeader, sizeof(uint8_t), sizeof(BMPHeader), newFile);

//    transforming the game placement into image data
    RGB24 newImageData[height][width];

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (placement[y][x] == 1) {     // alive
                newImageData[y][x].blue = 0;
                newImageData[y][x].green = 0;
                newImageData[y][x].red = 0;
            } else {                        // dead
                newImageData[y][x].blue = ~0;
                newImageData[y][x].green = ~0;
                newImageData[y][x].red = ~0;
            }
        }
    }

//    writing data
    for (int i = 0; i < height; ++i) {

        fwrite(newImageData[i], sizeof(RGB24), sizeof(newImageData[i]) / sizeof(RGB24), newFile);

        if (padding > 0) {
            uint8_t paddingData[padding];
            for (int j = 0; j < padding; ++j) {
                paddingData[j] = 0x00;
            }

            fwrite(paddingData, sizeof(uint8_t), sizeof(paddingData) / sizeof(uint8_t), newFile);
        }
    }

    fclose(newFile);
}

int isSavable(int currentSave, int freq) {
    if ((currentSave % freq) == 0) {
        return 1;
    } else {
        return 0;
    }
}

int isGameOver(int **oldPlacement, int **newPlacement, int width, int height) {
    int result = 1;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (oldPlacement[y][x] != newPlacement[y][x]) {
                result = 0;
                return result;
            }
        }
    }

    return result;
}

int main(int argc, char **argv) {
    int width;
    int height;
    int padding;

    int iter;
    int iterChanged = 0;
    int freq = 1;
    int currentSave = 1;

    FILE *initialFile;
    char initialFilename[100];
    char destinationDirectory[100];

    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "--input")) {
            strcpy(initialFilename, argv[++i]);
        } else if (!strcmp(argv[i], "--output")) {
            strcpy(destinationDirectory, argv[++i]);
        } else if (!strcmp(argv[i], "--max_iter")) {
            iter = atoi(argv[++i]);
            iterChanged = 1;
        } else if (!strcmp(argv[i], "--dump_freq")) {
            freq = atoi(argv[++i]);
        }
    }

//    opening preliminary file
    initialFile = fopen(initialFilename, "rb");
    if (initialFile == NULL) {
        printf("Error opening file to read");
        return 1;
    }

//    getting BMP header data
    BMPHeader *header = getHeaderData(initialFile);
    fseek(initialFile, 0L, SEEK_SET);

    width = header->ImageWidth;
    height = header->ImageHeight;
    padding = getPaddingBytes(width);

    if (iterChanged == 1) {
        for (int i = 0; i < iter; ++i) {
//            reading buffer data
            BMPHeaderDefault *headerDefault = getDefaultHeader(initialFile);

//            getting image data; closing the file
            RGB24 **img = getImageData(initialFile, width, height, padding);
            fclose(initialFile);

//            getting data for the GOL
            int **initialPlacement = getInitialPlacement(width, height, img);

//            GOL
            int **newPlacement = gameOfLife(initialPlacement, width, height);

//            getting new filename; checking and saving; changing initial file
            char destinationFilename[100];
            sprintf(destinationFilename, "%s\\%d.bmp", destinationDirectory, currentSave);

            if (isSavable(currentSave, freq) == 1) {
                writeBMP(newPlacement, width, height, padding, headerDefault, destinationFilename);
            }
            ++currentSave;

//            ending game
            if (isGameOver(initialPlacement, newPlacement, width, height) == 1) {
                break;
            }

//            opening new file
            strcpy(initialFilename, destinationFilename);

            initialFile = fopen(initialFilename, "rb");
            if (initialFile == NULL) {
                printf("Error opening file to read");
                return 1;
            }
        }
    } else {
        while (1) {
//            reading buffer data
            BMPHeaderDefault *headerDefault = getDefaultHeader(initialFile);

//            getting image data; closing the file
            RGB24 **img = getImageData(initialFile, width, height, padding);
            fclose(initialFile);

//            getting data for the GOL
            int **initialPlacement = getInitialPlacement(width, height, img);

//            GOL
            int **newPlacement = gameOfLife(initialPlacement, width, height);

//            getting new filename; checking and saving; changing initial file
            char destinationFilename[100];
            sprintf(destinationFilename, "%s\\%d.bmp", destinationDirectory, currentSave);

            if (isSavable(currentSave, freq) == 1) {
                writeBMP(newPlacement, width, height, padding, headerDefault, destinationFilename);
            }
            ++currentSave;

//            ending game
            if (isGameOver(initialPlacement, newPlacement, width, height) == 1) {
                break;
            }

//            opening new file
            strcpy(initialFilename, destinationFilename);

            initialFile = fopen(initialFilename, "rb");
            if (initialFile == NULL) {
                printf("Error opening file to read");
                return 1;
            }
        }
    }

    return 0;
}

