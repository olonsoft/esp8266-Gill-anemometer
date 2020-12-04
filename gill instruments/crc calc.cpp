#include <iostream>
#include <string>
#include <string.h>

uint16_t calcCRC(char* str)
{
  uint16_t crc=0; // starting value as you like, must be the same before each calculation
  for (int i=0; i<strlen(str); i++) // for each character in the string
  {
    crc ^= str[i]; // update the crc value
  }
  return crc;
}

char str[]="Q,346,017.17,N,00,";


int main()
{
    printf(str);
    printf("\n");
    printf("%x",calcCRC(str));
}