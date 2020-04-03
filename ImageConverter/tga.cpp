#include "tga.h"

unsigned char *LoadTGAFromFile(char *FileName,int &Width,int &Height)
{
 STGAHeader tgaHeader;
 //пробуем считать изображение
 FILE *file=fopen(FileName,"rb");
 if (file==NULL) return (NULL);
 //читаем заголовок
 if (fread(&tgaHeader,sizeof(STGAHeader),1,file)<0)
 {
  fclose(file);
  return(NULL);
 }
 //проверяем на возможность чтения
 if (tgaHeader.imageType&8)
 {
  fclose(file);
  return(NULL);//RLE не поддерживаем
 }
 if (tgaHeader.imageType&7==0 || tgaHeader.imageType&7==3)
 {
  fclose(file);
  return(NULL);//градации серого и отсутствие изображения не поддерживаем
 }
 //задаём параметры изображения
 Width=tgaHeader.width;
 Height=tgaHeader.height;
 //выделяем память для изображения
 int image_length=tgaHeader.width*tgaHeader.height*tgaHeader.bits/8;
 unsigned char *i_buffer=new unsigned char[image_length];
 //считываем изображение
 fseek(file,sizeof(STGAHeader)+tgaHeader.colorMapStart+tgaHeader.colorMapLength*tgaHeader.colorMapBits/8,SEEK_SET);
 if(fread(i_buffer,image_length,1,file)<0)
 {
  fclose(file);
  delete(i_buffer);
  return(NULL);
 }
 //а теперь анализируем формат
 if (tgaHeader.bits==24)
 {
  fclose(file);
  return(i_buffer);
 }

 if (tgaHeader.colorMapType==1 && tgaHeader.colorMapBits/8==3)//есть палитра по 24 бита
 {
  fseek(file,tgaHeader.colorMapStart+sizeof(STGAHeader),SEEK_SET);
  //читаем палитру
  unsigned char *color_map=new unsigned char[tgaHeader.colorMapLength*3];
  if(fread(color_map,tgaHeader.colorMapLength*3,1,file)<0)
  {
   fclose(file);
   delete(color_map);
   delete(i_buffer);
   return(NULL);
  }
  //нам потребуется изменить формат
  unsigned char *out_image=new unsigned char[tgaHeader.width*tgaHeader.height*3];
  unsigned char *oi_ptr=out_image;
  unsigned char *i_ptr=i_buffer;
  for(int n=0;n<tgaHeader.height*tgaHeader.width;n++,i_ptr++)
  {
   int index=(*i_ptr)*3;
   *oi_ptr=color_map[index];oi_ptr++;
   *oi_ptr=color_map[index+1];oi_ptr++;
   *oi_ptr=color_map[index+2];oi_ptr++;
  }


  /*
  unsigned char *out_image=new unsigned char[tgaHeader.width*tgaHeader.height*3];
  unsigned char *oi_ptr=out_image;
  unsigned char *i_ptr=i_buffer+tgaHeader.width*tgaHeader.height-1;
  for(int y=tgaHeader.height-1;y>=0;y--,i_ptr-=tgaHeader.width)
  {
   unsigned char *i_ptrc=i_ptr;
   for(int x=0;x<tgaHeader.width;x++,i_ptrc++)
   {
    int index=(*i_ptrc)*3;
    *oi_ptr=color_map[index];oi_ptr++;
    *oi_ptr=color_map[index+1];oi_ptr++;
    *oi_ptr=color_map[index+2];oi_ptr++;
   }
  }
  */
  delete(i_buffer);
  delete(color_map);
  return(out_image);
 }
 //иные режимы не поддерживаем
 fclose(file);
 delete(i_buffer);
 return(NULL);
}
unsigned char *LoadTGAFromResource(HMODULE hModule,int ID,int &Width,int &Height)
{
 HRSRC hRSRC=FindResource(hModule,(LPSTR)ID,RT_RCDATA);
 if (hRSRC==NULL) return(NULL);
 HGLOBAL hGlobal=LoadResource(hModule,hRSRC);
 if (hGlobal==NULL) return(NULL);
 unsigned char *Data=(unsigned char*)LockResource(hGlobal);
 int DataSize=SizeofResource(hModule,hRSRC);
 //а теперь считываем данные
 if (DataSize<sizeof(STGAHeader))//файл короткий
 {
  GlobalUnlock(hGlobal);
  return(NULL);
 }
 STGAHeader *tgaHeader;
 //читаем заголовок
 tgaHeader=(STGAHeader*)Data;
 //проверяем на возможность чтения
 if (tgaHeader->imageType&8)
 {
  GlobalUnlock(hGlobal);
  return(NULL);//RLE не поддерживаем
 }
 if (tgaHeader->imageType&7==0 || tgaHeader->imageType&7==3)
 {
  GlobalUnlock(hGlobal);
  return(NULL);//градации серого и отсутствие изображения не поддерживаем
 }
 //задаём параметры изображения
 Width=tgaHeader->width;
 Height=tgaHeader->height;
 //выделяем память для изображения
 int image_length=tgaHeader->width*tgaHeader->height*tgaHeader->bits/8;
 unsigned char *i_buffer=new unsigned char[image_length];
 //считываем изображение
 int offset=sizeof(STGAHeader)+tgaHeader->colorMapStart+tgaHeader->colorMapLength*tgaHeader->colorMapBits/8;
 if (offset+image_length>DataSize)//файл короткий
 {
  GlobalUnlock(hGlobal);
  delete(i_buffer);
  return(NULL);
 }
 memcpy(i_buffer,Data+offset,image_length);//копируем данные
 //а теперь анализируем формат
 if (tgaHeader->bits==24)
 {
  GlobalUnlock(hGlobal);
  return(i_buffer);
 }

 if (tgaHeader->colorMapType==1 && tgaHeader->colorMapBits/8==3)//есть палитра по 24 бита
 {
  int offset=tgaHeader->colorMapStart+sizeof(STGAHeader);
  unsigned char *color_map=Data+offset;
  //нам потребуется изменить формат
  unsigned char *out_image=new unsigned char[tgaHeader->width*tgaHeader->height*3];
  unsigned char *oi_ptr=out_image;
  unsigned char *i_ptr=i_buffer;
  for(int n=0;n<tgaHeader->height*tgaHeader->width;n++,i_ptr++)
  {
   int index=(*i_ptr)*3;
   *oi_ptr=color_map[index];oi_ptr++;
   *oi_ptr=color_map[index+1];oi_ptr++;
   *oi_ptr=color_map[index+2];oi_ptr++;
  }

/*
  unsigned char *out_image=new unsigned char[tgaHeader->width*tgaHeader->height*3];
  unsigned char *oi_ptr=out_image;
  unsigned char *i_ptr=i_buffer+tgaHeader->width*tgaHeader->height-1;
  for(int y=tgaHeader->height-1;y>=0;y--,i_ptr-=tgaHeader->width)
  {
   unsigned char *i_ptrc=i_ptr;
   for(int x=0;x<tgaHeader->width;x++,i_ptrc++)
   {
    int index=(*i_ptrc)*3;
    *oi_ptr=color_map[index];oi_ptr++;
    *oi_ptr=color_map[index+1];oi_ptr++;
    *oi_ptr=color_map[index+2];oi_ptr++;
   }
  }
  */
  delete(i_buffer);
  GlobalUnlock(hGlobal);
  return(out_image);
 }
 //иные режимы не поддерживаем
 delete(i_buffer);
 GlobalUnlock(hGlobal);
 return(NULL);
}

bool SaveTGA(char *FileName,int Width,int Height,unsigned char *Image)
{
 struct STGAHeader sTGAHeader;
 sTGAHeader.identsize=0;
 sTGAHeader.colorMapType=0;
 sTGAHeader.imageType=2;
 sTGAHeader.colorMapStart=0;
 sTGAHeader.colorMapLength=0;
 sTGAHeader.colorMapBits=24;
 sTGAHeader.xstart=0;
 sTGAHeader.ystart=0;
 sTGAHeader.width=Width;
 sTGAHeader.height=Height;
 sTGAHeader.bits=24;
 sTGAHeader.descriptor=32;
 FILE *file=fopen(FileName,"wb");
 if (file==NULL) return(false);//ошибка создания файла
 //сохраняем заголовок
 if (fwrite(&sTGAHeader,1,sizeof(struct STGAHeader),file)<sizeof(struct STGAHeader))//ошибка записи
 {
  fclose(file);
  return(false);
 }
 //сохраняем в файл
 if (fwrite(Image,1,Width*Height*3,file)<(unsigned int)(Width*Height*3))//ошибка записи
 {
  fclose(file);
  return(false);
 }
 //закрываем файл
 fclose(file);
 return(true);
}


