#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bmp.h"
#include "error.h"

int get_alignment(BMPINFO *Info) {
    return (4 - (3 * Info->Width) % 4) % 4;
    //(3 * Info->Width) % 4 позволяет узнать, сколько байтов в строке нельзя упаковать в группы по 4
    //вычтя это значение из 4, мы узнаем, сколько нулевых байтов нужно дописать в конце каждой строки, чтобы сумма всех байтов в строке была кратна 4
    //еще одно деление по модулю на 4 нужно, если кол-во байтов в строке изначально было кратно 4, то есть (3 * Info->Width) % 4 = 0 => 4 - (3 * Info->Width) % 4 = 4, а дописывать нужно 0 лишних байтов
}

int load_bmp(char *FileName, BMPHEADER *Header, BMPINFO *Info, char **Pixels) {
    assert(FileName != NULL && Header != NULL && Info != NULL);

    FILE *File;

    if ((File = fopen(FileName, "rb")) == NULL)
        return E_FOPEN;

    if (fread(Header, sizeof(BMPHEADER), 1, File) != 1 || fread(Info, sizeof(BMPINFO), 1, File) != 1) {
        fclose(File);
        return E_FREAD;
    }

    if (Header->Type != SIGNATURE_1 && Header->Type != SIGNATURE_2) {
        fclose(File);
        return E_WRONG_FORMAT;
    }

    if (Info->SizeImage == 0)
        Info->SizeImage = Info->Width * Info->Height;

    *Pixels = malloc(Info->SizeImage);
    
    int width_in_px = Info->Width * 3;

    if (*Pixels == NULL) {
        fclose(File);
        return E_MALLOC;
    }

    if (fread(*Pixels, 1, Info->SizeImage, File) != Info->SizeImage) {
        fclose(File);
        free(*Pixels);
        *Pixels = NULL;
        return E_FREAD;
    }

    if (Info->Width % 4 != 0) {
        int alignment = get_alignment(Info);
        char *Tmp = malloc(Info->Height * width_in_px);

        if (Tmp == NULL) {
            fclose(File);
            return E_MALLOC;
        }

        for (int i = 0; i < Info->Height; ++i)
            memcpy(Tmp + width_in_px * i, (*Pixels) + width_in_px * i + alignment * i, width_in_px);

        free(*Pixels);
        *Pixels = Tmp;
    }

    fclose(File);
    return 1;
}

int save_bmp(char *FileName, BMPHEADER *Header, BMPINFO *Info, char **Pixels) {
    assert(FileName != NULL && Header != NULL && Info != NULL && Pixels != NULL);

    FILE *File;

    if ((File = fopen(FileName, "wb")) == NULL)
        return E_FOPEN;

    int alignment = get_alignment(Info);
    int width_in_px = Info->Width * 3;
    Info->SizeImage = width_in_px * Info->Height + Info->Height * alignment; 
    Header->Size = sizeof(*Header) + sizeof(*Info) + Info->SizeImage;

    if (fwrite(Header, sizeof(BMPHEADER), 1, File) != 1 || fwrite(Info, sizeof(BMPINFO), 1, File) != 1) {
        fclose(File);
        return E_FWRITE;
    }

    if (Info->Width % 4 != 0) { 
        char *Tmp = malloc(Info->SizeImage);

        if (Tmp == NULL) {
            fclose(File);
            return E_MALLOC;
        }

        memset(Tmp, 0, Info->SizeImage);

        for (int i = 0; i < Info->Height; ++i)
            memcpy(Tmp + width_in_px * i + alignment * i, (*Pixels) + width_in_px * i, width_in_px);

        free(*Pixels);
        *Pixels = Tmp;
    }
    
    if (fwrite(*Pixels, 1, Info->SizeImage, File) != Info->SizeImage) {
        fclose(File);
        return E_FWRITE;
    }

    fclose(File);
    return 1;
}

int crop(BMPINFO *Info, char **Pixels, char **NewPicture, int x, int y, int width, int height) {
    assert(Info != NULL && Pixels != NULL && NewPicture != NULL);
    int sizeOfPixel = 3;
        
    if (width <= 0 || height <= 0 || x < 0 || y < 0 || x + width > Info->Width || y + height > Info->Height)
        return E_CROP_PARAMS;

    *NewPicture = malloc(width * height * sizeOfPixel);

    if (*NewPicture == NULL)
        return E_MALLOC;

    for (int i = 0; i < height; ++i)
        for (int j = 0; j < width; ++j) {
            char *to = (*NewPicture) + (height - 1 - i) * sizeOfPixel * width + j * sizeOfPixel;
            char *from = (*Pixels) + (Info->Height - 1 - y - i) * sizeOfPixel * Info->Width + (x + j) * sizeOfPixel;
            memcpy(to, from, sizeOfPixel);
            //попиксельно считываем картинку в NewPicture из Pixels (указатель для NewPicture и Pixels вычисляется практически одинаково, только вместо -1 мы используем -1-y, а вместо j используем x+j, так как нужно учитывать то, что мы обрезаем картинку (т.е. координаты верхнего левого угла))
        }

    Info->Width = width;
    Info->Height = height;
    Info->SizeImage = width * height * sizeOfPixel;

    return 1;
}

int rotate(BMPINFO *Info, char **Pixels) {
    assert(Info != NULL && Pixels != NULL);
    int sizeOfPixel = 3;
    
    char *Tmp;

    Tmp = malloc(Info->SizeImage);
    if (Tmp == NULL)
        return E_MALLOC;
    
    for (int i = 0; i < Info->Height; ++i)
        for (int j = 0; j < Info->Width; ++j) {
            char *from = (*Pixels) + i * sizeOfPixel * Info->Width + j * sizeOfPixel;
            char *to = Tmp + j * sizeOfPixel * Info->Height + i * sizeOfPixel;
            memcpy(to, from, sizeOfPixel);
            //попиксельно считываем в Tmp картинку, поворачивая ее (достигается за счет того, что в Tmp мы сначала выбираем столбец, потом строку, а не, как обычно, наоборот)
        }
    
	int time = Info->Width;
    Info->Width = Info->Height;
    Info->Height = time;
    
    for (int i = 0; i < Info->Height; ++i)
        for (int j = 0; j < Info->Width; ++j) {
            char *to = (*Pixels) + i * sizeOfPixel * Info->Width + j * sizeOfPixel;
            char *from = Tmp + (Info->Height - 1 - i) * sizeOfPixel * Info->Width + j * sizeOfPixel;
            memcpy(to, from, sizeOfPixel);
            //попиксельно считываем картинку обратно в Pixels из Tmp
        }

    free(Tmp);

    return 1;
}