#include <cstdio>
#include "bitbank2.h"


void *bb2OpenFile(const char *fname, int32_t *pSize) {
    FILE *infile = fopen(fname, "r");
    // setvbuf(infile, nullptr, _IOFBF, 4096);

    if (infile) {
        fseek(infile, 0, SEEK_END);
        *pSize = ftell(infile);
        rewind(infile);
        return infile;
    }
    return nullptr;
} /* GIFOpenFile() */

void bb2CloseFile(void *pHandle) {
    FILE *infile = (FILE *)(pHandle);
    if (infile != nullptr) {
        fclose(infile);
    }
} /* GIFCloseFile() */

int32_t bb2ReadFile(bb2_file_tag *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    FILE *infile = (FILE *)(pFile->fHandle);
    if (iBytesRead <= 0)
        return 0;
    iBytesRead = (int32_t) fread(pBuf, 1, iBytesRead, infile);
    pFile->iPos = ftell(infile);
    return iBytesRead;
} /* GIFReadFile() */

int32_t bb2SeekFile(bb2_file_tag *pFile, int32_t iPosition) {
    FILE *infile = (FILE *)(pFile->fHandle);
    fseek(infile, iPosition, SEEK_SET);
    pFile->iPos = ftell(infile);
    return pFile->iPos;
} /* GIFSeekFile() */
