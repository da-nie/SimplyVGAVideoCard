#ifndef LOADTGA_H
#define LOADTGA_H

#include <windows.h>
#include <stdio.h>

//заголовок TGA-файла
#pragma pack(1)
struct STGAHeader
{
 char identsize;//размер поля ID заголовка (0)
 char colorMapType;//если ли палитра:0-нет,1-есть
 char imageType;//тип картинки:0-нет,1-индексные цвета,2-RGB,3-оттенки серого, (3-й бит - RLE- кодирование)
 unsigned short    colorMapStart;//начало карты цветов
 unsigned short    colorMapLength;//количество цветов в карте
 unsigned char     colorMapBits;//размерность палитры
 unsigned short    xstart;//начальные координаты изображения
 unsigned short    ystart;
 unsigned short    width;//размер изображения по X
 unsigned short    height;//размер изображения по Y
 char bits;//количесто бит на пиксель (8,16,24,32)
 char descriptor;//дескриптор изрображения
};
#pragma pack(8)

unsigned char *LoadTGAFromFile(char *FileName,int &Width,int &Height);
unsigned char *LoadTGAFromResource(HMODULE hModule,int ID,int &Width,int &Height);
bool SaveTGA(char *FileName,int Width,int Height,unsigned char *Image);

#endif

