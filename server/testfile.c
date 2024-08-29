#include <stdio.h>

int main () {
   FILE *fp;

   fp = fopen("file.txt", "w+");

   fputs("This is c programming.", fp);
   fputs("\n",fp);

   fputs("This is a system programming language\n.", fp);
   

   fclose(fp);
   
   return(0);
}