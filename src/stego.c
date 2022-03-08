#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stego.h"
#include "error.h"

int EncodeSymb(char c) {
    if ('A' <= c && c <= 'Z')
        return c - 'A';
    else if (c == ' ')
        return 26;
    else if (c == '.')
        return 27;
    else if (c == ',')
        return 28;
    else
        return -1;
}

char DecodeSymb(int n) {
    if (n < 26)
        return n + 'A';
    else if (n == 26)
        return ' ';
    else if (n == 27)
        return '.';
    else if (n == 28)
        return ',';
    else
        return -1;
}

int EncodeColor(char c) {
    if (c == 'R')
        return 2;
    if (c == 'G')
        return 1;
    if (c == 'B')
        return 0;

    return -1;
}

void set_last_bit(char **Pixels, int Width, int Height, int x, int y, int color, int n) {
    y = Height - y - 1; 
    int size_of_pixel = 3;
    (*Pixels)[y * Width * size_of_pixel + x * size_of_pixel + color] |= (n & 1);
    (*Pixels)[y * Width * size_of_pixel + x * size_of_pixel + color] &= 254 + (n & 1);                
}

int reset_last_bit(char **Pixels, int Width, int Height, int x, int y, int color) {
    y = Height - y - 1; 
    int size_of_pixel = 3;
    return (*Pixels)[y * Width * size_of_pixel + x * size_of_pixel + color] & 1;                
}

int EncodeMessage(char *KeyFileName, char *MsgFileName, char **Pixels, int Width, int Height) {
    assert(KeyFileName != NULL && MsgFileName != NULL && Pixels != NULL);

    FILE *KeyFile = NULL, *MsgFile = NULL;

    if ((KeyFile = fopen(KeyFileName, "r")) == NULL || (MsgFile = fopen(MsgFileName, "r")) == NULL) {
        fclose(KeyFile);
        return E_FOPEN;
    }

    int x, y;
    char c, ch;

    while (fscanf(MsgFile, "%c", &ch) != EOF) {
        int n = EncodeSymb(ch);

        if (n != -1) {
            for (int i = 0; i < 5; ++i)
                if (fscanf(KeyFile, "%d %d %c", &x, &y, &c) != 3 || x < 0 || x > Width || y < 0 || y > Height) {
                    fclose(KeyFile);
                    fclose(MsgFile);
                    return E_STEGO;
                } else {
                    int color = EncodeColor(c);
                    if (color == -1) {
                        fclose(KeyFile);
                        fclose(MsgFile);
                        return E_STEGO;
                    }
                    set_last_bit(Pixels, Width, Height, x, y, color, n);
                    n >>= 1;
                }
        }
    }
    fclose(KeyFile);
    fclose(MsgFile);

    return 1;
}

int DecodeMessage(char *KeyFileName, char *MsgFileName, char **Pixels, int Width, int Height) {
    assert(KeyFileName != NULL && MsgFileName != NULL && Pixels != NULL);

    FILE *KeyFile = NULL, *MsgFile = NULL;

    if ((KeyFile = fopen(KeyFileName, "r")) == NULL || (MsgFile = fopen(MsgFileName, "w")) == NULL) {
        fclose(KeyFile);
        return E_FOPEN;
    }

    int x, y, n = 0, len = 0;
    char c;

    while (fscanf(KeyFile, "%d %d %c", &x, &y, &c) == 3) {
        if (x < 0 || x > Width || y < 0 || y > Height) {
            fclose(KeyFile);
            fclose(MsgFile);
            return E_STEGO;
        }
        int color = EncodeColor(c);
        if (color == -1) {
            fclose(KeyFile);
            fclose(MsgFile);
            return E_STEGO;
        }          
        n <<= 1;
        n |= reset_last_bit(Pixels, Width, Height, x, y, color);
        ++len;

        if (len == 5) {
            int res = 0;
            for (int i = 0; i < 5; ++i) {
                res <<= 1;
                res |= n & 1;
                n >>= 1;
            }
            fprintf(MsgFile, "%c", DecodeSymb(res));
            n = len = 0;
        }
    }
    fprintf(MsgFile, "\n");
    fclose(KeyFile);
    fclose(MsgFile);
    return 1;
}