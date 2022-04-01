#include <tcpip/base64.h>

/*
src holds a string in base64 (null terminated), this will convert that string 
to ascii and save to dest (null terminated).  if src is NULL, then it will use
dest for the source and save the result over source.
*/
void Base64ToString(char *dest, char *src)
{
   int32 j;
   int8 i,scr;
   
   if (!src)
      src=dest;
   
   while(*src)
   {
      j=0;
      for (i=0;i<4;i++)
      {
         scr=*src;
         //printf("'%c'->",scr);
         if (scr)
            src++;
         if ((scr>='A')&&(scr<='Z'))
            scr-='A';
         else if ((scr>='a')&&(scr<='z'))
            scr+=26-'a';
         else if ((scr>='0')&&(scr<='9'))
            scr+=52-'0';
         else if (scr==' ')
            scr=62;
         else if (scr=='/')
            scr=63;
         else
            scr=0;
            
         j *= (int32)64; //bit shift left 6 times
         j &= (int32)0xFFFFFFC0;
         j |= scr;
         //printf("%U [%LX]\r\n", scr,j);
      }
      dest[0]=make8(j,2);
      dest[1]=make8(j,1);
      dest[2]=make8(j,0);
      //printf("APPEND: '%c%c%c'\r\n", dest[0],dest[1],dest[2]);
      dest+=3;
   }
   *dest=0;
}

//takes 3 characters from src and converts into 4 base64 encoded characters into dest
//if size=-1, then use strlen to determine size
void StringToBase64Block(char *dest, char *src, int16 size)
{
   const char lookupTable[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 /";
   char orig[3];
   char enc[4];
   int8 i;
      
   memcpy(orig, src, 3);
      
   enc[0] = orig[0] >> 2;
   enc[1] = (orig[0] << 4) | (orig[1] >> 4);
   enc[2] = (orig[1] << 2) | (orig[2] >> 6);
   enc[3] = orig[2];
   
   //printf("[%X%X%X->%X%X%X%X]", orig[0], orig[1], orig[2], enc[0], enc[1], enc[2], enc[3]);
   
   for (i=0;i<4;i++)
   {
      enc[i] = lookupTable[enc[i] & 0x3F];
   }

   if (size==-1)
      size = strlen(src);

   if (size<3)
      enc[3]='=';
   if (size<2)
      enc[2]='=';
   
   memcpy(dest, enc, 4);
}

void StringToBase64(char *dest, char *src)
{
}

// StringToBase64XMIT()
//
// Encodes strings into base64 and then transmits them over TCP.
//
// This function was designed with the SMTP PLAIN authentication in mind,
// which required smushing two strings into one base64 encoded string,
// with a 0x00 encoded into the string before each string.  If you simply
// need to send on base64 encoded string (without the 0x00 added) then
// you can still use this function, just pass '1' into numSrc.
//
// srcs - a pointer to an array of strings.  these are the strings that
//        need to be base64 encoded and then transmitted.
// numSrcs - the number of strings that srcs holds
// TCPSocket - TCP socket to transmit encoded string. We make the assumption 
//             that the socket is ready for transmit and can send the whole 
//             encoded string.
//
// returns - returns TRUE if success, FAIL if error (we tried to send more
//           data than MAC could handle)
//srcs is actually a pointer to a poitner, but CCS is BUGGY!
int8 StringToBase64XMIT(int16 *srcs, int numSrc, int TCPSocket)
{
   int16 src;
   int len,i,pos;
   char blockIn[3], blockOut[4];
   int1 done = FALSE;

   pos = 0;
   if (numSrc > 1)
      blockIn[pos++]=0;
   src = *srcs;
   srcs++;
   numSrc--;
   len = strlen(src);

   while(!done)
   {           
      i = 3-pos;
      memcpy(&blockIn[pos], src, i);
      if (i > len)
      {
         pos += len;
         if (!numSrc)
            done = TRUE;
         else
         {
            blockIn[pos++]=0;
            src = *srcs;
            srcs++;
            numSrc--;
            len = strlen(src);
         }
      }
      else
      {
         src += i;
         pos += i;
         len -= i;
      }
      
      if ((done && (pos>0)) || (pos>=3))
      {
         StringToBase64Block(blockOut, blockIn, pos);
         if (TCPPutArray(TCPSocket, blockOut, 4) != 4)
            return(FALSE);
         pos = 0;
      }
   }
   return(TRUE);
}
