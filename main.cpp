#include <stdio.h>
#include <string>

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("\nUsage: ./makemofs [output image file] [boot record file] [extra files' names] ...\n");
    } else {
        FILE *image = fopen(argv[1], "w+");
        FILE *mbr   = fopen(argv[2], "r");
        fseek(image, 0, SEEK_END);
        unsigned long size = (unsigned long)ftell(image);
        char data[size];
        char MBR[512];
        fread(&MBR, 1, 512, mbr);
        fwrite(&MBR, 1, 512, image);
        fclose(mbr);

        const int numExtraFiles = argc - 3;
        unsigned char FileDescriptors[numExtraFiles * 512];
        const char *FileNames[numExtraFiles];
        long int StartLBA[numExtraFiles];
        long int EndLBA[numExtraFiles];
        unsigned long szFiles[numExtraFiles];
        for(int i = 3; i < argc; i++) {
            FileNames[i - 3] = argv[i];
            
            FILE *infile = fopen(argv[i], "r");
            fseek(infile, 0, SEEK_END);
            unsigned long fsize = (unsigned long)ftell(infile);
            szFiles[i - 3] = fsize / 512;
            if((fsize % 512) > 0) {
                szFiles[i - 3] += 1;
            }
            
            if(fsize == 0) { szFiles[i - 3] = 1; }
        }

        for(int i = 0; i < numExtraFiles; i++) {
            if(i == 0) {
                StartLBA[i] = 3;
                EndLBA[i] = StartLBA[i] + szFiles[i];
            } else {
                for(int j = 0; j < numExtraFiles; j++) {
                    StartLBA[j] += 1; EndLBA[j] += 1;
                }

                StartLBA[i] = EndLBA[i - 1];
                EndLBA[i] = StartLBA[i] + szFiles[i];
            }
        }

        for(int i = 0; i < sizeof(FileDescriptors); i++) {
            FileDescriptors[i] = 0;
        }

        for(int i = 0; i < sizeof(FileDescriptors); i+=512) {
            unsigned char cFilename[128];
            for(int j = 0; j < 128; j++) {
                std::string str(FileNames[i / 512]);
                if(j > str.size()) {
                    cFilename[j] = 0;
                } else {
                    cFilename[j] = (char)* (FileNames[i / 512] + j);
                }
            }

            for(int j = i; j < 128 + i; j++) {
                FileDescriptors[j] = cFilename[j - i];
            }

            long int sz;
            sz = (long int)StartLBA[i / 512];
            FileDescriptors[i + 128] = sz;

            sz = (long int)EndLBA[i / 512];
            FileDescriptors[i + 128 + 8] = sz;
        }

        fwrite(FileDescriptors, 1, sizeof(FileDescriptors), image);
        
        char FSector[512];
        for(int i = 0; i < 512; i++) {
            FSector[i] = 0xFF;
        }

        fwrite(FSector, 1, 512, image);

        for(int i = 0; i < numExtraFiles; i++) {
            fseek(image, 512 * StartLBA[i], SEEK_SET);
            FILE *infile = fopen(argv[i + 3], "r");
            char buffer[szFiles[i] * 512];
            for(int i = 0; i < sizeof(buffer); i++) { buffer[i] = 0; }
            fseek(infile, 0, SEEK_END);
            unsigned long insize = (unsigned long)ftell(infile);
            fseek(infile, 0, SEEK_SET);
            fread(&buffer, 1, insize - 1, infile);
            fclose(infile);

            fwrite(buffer, 512, szFiles[i], image);
        }

        fclose(image);
    }
    
    printf("\n");
    
    return 0;
}
