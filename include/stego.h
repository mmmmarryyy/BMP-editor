#ifndef __STEG_H
#define __STEG_H

int EncodeMessage(char *KeyFN, char *MsgFN, char **Pixels, int Width, int Height);

int DecodeMessage(char *KeyFN, char *MsgFN, char **Pixels, int Width, int Height);

#endif