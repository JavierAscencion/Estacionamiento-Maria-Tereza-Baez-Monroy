#ifndef __BASE64_H__
#define __BASE64_H__

void Base64ToString(char *dest, char *src);
void StringToBase64(char *dest, char *src);
int8 StringToBase64XMIT(int16 *srcs, int numSrc, int TCPSocket);
#endif
