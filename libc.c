/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

void perror() {
	if (errno == 14) write(1, "Bad address\n", 12);
	else if (errno == 13) write(1,"Permission denied\n", 18);
	else if (errno == 9) write(1, "bad file number\n", 16);
}


void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

