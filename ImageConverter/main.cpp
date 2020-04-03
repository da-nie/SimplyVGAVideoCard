#include <windows.h>
#include <math.h>


//--------------------------------------------------------------------------
HINSTANCE hProjectInstance;
#include "tga.h"
//------------------------------------------------------------------------------
void Processing(char *InputFileName,char *OutputFileNameIMG,char *OutputFileNameTGA);
//------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevstance,LPSTR lpstrCmdLine,int nCmdShow)
{
 WIN32_FIND_DATA wfd;
 HANDLE Handle_Find=FindFirstFile("*.tga",&wfd);
 if (Handle_Find!=INVALID_HANDLE_VALUE)
 {
  while(1)
  {
   //это файл
   if (wfd.cFileName[0]!='.' && !(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
   {
    //отправляем на обработку
    char *ifn=new char[strlen(wfd.cFileName)+1];
    sprintf(ifn,"%s",wfd.cFileName);
    char *ofnimg=new char[strlen(wfd.cFileName)+1];
    sprintf(ofnimg,"%s",wfd.cFileName);
    int l=strlen(ofnimg);
    ofnimg[l-3]='i';
    ofnimg[l-2]='m';
    ofnimg[l-1]='g';
    char *ofntga=new char[strlen(wfd.cFileName)+10];
    sprintf(ofntga,"%s",wfd.cFileName);
    l=strlen(ofntga);
    ofntga[l-4]='-';
    ofntga[l-3]='o';
    ofntga[l-2]='u';
    ofntga[l-1]='t';
    ofntga[l-0]='.';
    ofntga[l+1]='t';
    ofntga[l+2]='g';
    ofntga[l+3]='a';
    Processing(ifn,ofnimg,ofntga);
    delete(ifn);
    delete(ofntga);
    delete(ofnimg);
   }
   if (FindNextFile(Handle_Find,&wfd)==FALSE) break;
  }
  FindClose(Handle_Find);
 }
 MessageBox(NULL,"Конвертация завершена","",MB_OK);
 return(0);
}
void Processing(char *InputFileName,char *OutputFileNameIMG,char *OutputFileNameTGA)
{
 int x,y;
 if (strstr(InputFileName,"-out.tga")!=NULL) return;
 int Width,Height;
 unsigned char *I=LoadTGAFromFile(InputFileName,Width,Height);
 if (I==NULL) return;
 FILE *file=fopen(OutputFileNameIMG,"wb");
 if (file==NULL)
 {
  delete(I);
  return;
 }
 for(y=0;y<Height;y++)
 {
  for(x=0;x<Width;x++)
  {
   int o=(x+y*Width)*3;
   unsigned char b=I[o];
   unsigned char g=I[o+1];
   unsigned char r=I[o+2];
   //понижаем разрядность
   b=b>>6;
   g=g>>5;
   r=r>>5;
   //формируем код
   unsigned char out=0;
   out|=b;
   out<<=3;
   out|=g;
   out<<=3;
   out|=r;
   fwrite(&out,1,sizeof(unsigned char),file);
   b=b<<6;
   g=g<<5;
   r=r<<5;

   I[o]=b;
   I[o+1]=g;
   I[o+2]=r;
  }
 }
 fclose(file);
 SaveTGA(OutputFileNameTGA,Width,Height,I);
}

