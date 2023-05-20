#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

union ID3Header {
    char buffer[10];
    struct {
        char ID[3];
        char Version1;
        char Version2;
        char Flags;
        char Size[4];
    } data;
};

union ID3FrameHeader {
    char buffer[10];

    struct {
        char FrameID[4];
        char Size[4];
        char Flags[2];
    } data;
};

union char_intConversion {
    int outputInt;
    struct {
        char first: 8;
        char second: 8;
        char third: 8;
        char fourth: 8;
    } inputChars;
};

char *getDirectory(char *filename) {
    char *directory = NULL;
    int dirLength;
    int length = strlen(filename);

    for (int i = length - 1; i > 0; --i) {
        if (filename[i] == '/') {
            dirLength = i;
            break;
        }
    }

    if (directory == NULL) {
        return directory;
    }

    strncpy(directory, filename, dirLength + 1);

    return directory;
}

int getLength(char *size) {
    union char_intConversion newInt;

    newInt.inputChars.fourth = size[0];
    newInt.inputChars.third = size[1];
    newInt.inputChars.second = size[2];
    newInt.inputChars.first = size[3];

    return newInt.outputInt;
}

void setLength(char *size, int newLength) {
    union char_intConversion newInt;

    newInt.outputInt = newLength;

    size[0] = newInt.inputChars.fourth;
    size[1] = newInt.inputChars.third;
    size[2] = newInt.inputChars.second;
    size[3] = newInt.inputChars.first;
}

void show(FILE *fptr) {
//    ID3 header
    union ID3Header header;
    fread(header.buffer, sizeof(header.buffer), 1, fptr);
    printf("%sv%d.%d\n", header.data.ID, header.data.Version1, header.data.Version2);

//    ID3 frames
    union ID3FrameHeader thisFrameHeader;

    int allTagsLength = getLength(header.data.Size);
    while (ftell(fptr) - 10 < allTagsLength) {
        fread(thisFrameHeader.buffer, sizeof(thisFrameHeader), 1, fptr);

//        exit
        if (thisFrameHeader.data.FrameID[0] == 0 ||
            thisFrameHeader.data.FrameID[1] == 0 ||
            thisFrameHeader.data.FrameID[2] == 0 ||
            thisFrameHeader.data.FrameID[3] == 0) {
            break;
        }

//        reading frame content
        int frameLength = getLength(thisFrameHeader.data.Size);
        char *frameData = malloc(frameLength);

        fread(frameData, frameLength, 1, fptr);

//        printing it out
        printf("%s: ", thisFrameHeader.data.FrameID);

        for (int i = 0; i < frameLength; ++i) {
            if (frameData[i] >= 32 && frameData[i] <= 126) {
                printf("%c", frameData[i]);
            }
        }
        printf("\n");

        free(frameData);
    }

    fclose(fptr);
}

void set(FILE *initialFile, FILE *newFile, char *prop_name, char *prop_value) {
//    ID3 header
    union ID3Header header;
    fread(header.buffer, sizeof(header.buffer), 1, initialFile);

//    ID3 frames
    union ID3FrameHeader thisFrameHeader;

    int allTagsLength = getLength(header.data.Size);

    int initialFrameLength;
    int newFrameLength = strlen(prop_value);

    while (ftell(initialFile) - 10 < allTagsLength) {
        fread(thisFrameHeader.buffer, sizeof(thisFrameHeader), 1, initialFile);

//        reading frame content
        int frameLength = getLength(thisFrameHeader.data.Size);
        char *frameData = malloc(frameLength);

        fread(frameData, frameLength, 1, initialFile);

//        finding the one
        if (!strcmp(thisFrameHeader.data.FrameID, prop_name)) {
            initialFrameLength = getLength(thisFrameHeader.data.Size);
            break;
        }
    }

//    offset
    int offset = newFrameLength - initialFrameLength;
    allTagsLength += offset;

//    writing everything
    fwrite(header.buffer, sizeof(header.buffer), 1, newFile);
    fseek(initialFile, sizeof(header.buffer), SEEK_SET);

    while (ftell(initialFile) - 10 < allTagsLength) {
        fread(thisFrameHeader.buffer, sizeof(thisFrameHeader), 1, initialFile);

//        exit
        if (thisFrameHeader.data.FrameID[0] == 0 ||
            thisFrameHeader.data.FrameID[1] == 0 ||
            thisFrameHeader.data.FrameID[2] == 0 ||
            thisFrameHeader.data.FrameID[3] == 0) {
            break;
        }

//        reading frame content
        int frameLength = getLength(thisFrameHeader.data.Size);
        char *frameData = malloc(frameLength);

        fread(frameData, frameLength, 1, initialFile);

//        writing untouched stuff
        if (strcmp(thisFrameHeader.data.FrameID, prop_name)) {
            int thisLength = getLength(thisFrameHeader.data.Size);

            fwrite(thisFrameHeader.buffer, sizeof(thisFrameHeader.buffer), 1, newFile);
            fwrite(frameData, thisLength, 1, newFile);

            continue;
        }

//        writing new stuff
        setLength(thisFrameHeader.data.Size, newFrameLength);

        fwrite(thisFrameHeader.buffer, sizeof(thisFrameHeader.buffer), 1, newFile);
        fwrite(prop_value, newFrameLength, 1, newFile);
    }

//    writing untouched stuff again + the song itself
    fseek(initialFile, (long) -sizeof(thisFrameHeader.buffer), SEEK_CUR);

    char buffer;
    while (!feof(newFile)) {
        if (fread(&buffer, sizeof(buffer), 1, initialFile)) {
            fwrite(&buffer, sizeof(buffer), 1, newFile);
        } else {
            break;
        }
    }

//    cleanup
    fclose(initialFile);
    fclose(newFile);
}

void get(FILE *fptr, char *prop_name) {
//    ID3 header
    union ID3Header header;
    fread(header.buffer, sizeof(header.buffer), 1, fptr);

//    ID3 frames
    union ID3FrameHeader thisFrameHeader;

    int allTagsLength = getLength(header.data.Size);

    while (ftell(fptr) - 10 < allTagsLength) {
        fread(thisFrameHeader.buffer, sizeof(thisFrameHeader), 1, fptr);

//        exit
        if (thisFrameHeader.data.FrameID[0] == 0 ||
            thisFrameHeader.data.FrameID[1] == 0 ||
            thisFrameHeader.data.FrameID[2] == 0 ||
            thisFrameHeader.data.FrameID[3] == 0) {
            break;
        }

//        reading frame content
        int frameLength = getLength(thisFrameHeader.data.Size);
        char *frameData = malloc(frameLength);

        fread(frameData, frameLength, 1, fptr);

//        checking whether it is the prop_name tag
        if (strcmp(thisFrameHeader.data.FrameID, prop_name) != 0) {
            continue;
        }

//        printing it out
        printf("%s: ", thisFrameHeader.data.FrameID);

        for (int i = 0; i < frameLength; ++i) {
            if (frameData[i] >= 32 && frameData[i] <= 126) {
                printf("%c", frameData[i]);
            }
        }
        printf("\n");

        free(frameData);
    }

    fclose(fptr);
}

int main(int argc, char **argv) {
    FILE *initialFile;
    char path[100];

    char prop_name[5];
    char prop_value[100];

    int showFlag = 0;
    int setFlag = 0;
    int valueFlag = 0;
    int getFlag = 0;

    for (int i = 0; i < argc; ++i) {
        if (!strncmp(argv[i], "--filepath=", 11)) {
            strcpy(path, argv[i] + 11);
        } else if (!strcmp(argv[i], "--show")) {
            showFlag = 1;
        } else if (!strncmp(argv[i], "--set=", 6)) {
            strcpy(prop_name, argv[i] + 6);
            setFlag = 1;
        } else if (!strncmp(argv[i], "--value=", 8)) {
            strcpy(prop_value, argv[i] + 8);
            valueFlag = 1;
        } else if (!strncmp(argv[i], "--get=", 6)) {
            strcpy(prop_name, argv[i] + 6);
            getFlag = 1;
        }
    }

    initialFile = fopen(path, "rb");

    if (showFlag == 1) {
        show(initialFile);
        return 0;
    } else if (setFlag == 1 && valueFlag == 1) {
//        opening new file
        char *directory = getDirectory(path);

        char *newFilename = "set.mp3";
        if (directory != NULL) {
            newFilename = strcat(directory, "set.mp3");
        }

        FILE *newFile = fopen(newFilename, "wb");

        set(initialFile, newFile, prop_name, prop_value);
        return 0;
    } else if (getFlag == 1) {
        get(initialFile, prop_name);

        return 0;
    }

    fclose(initialFile);
    return 1;
}

