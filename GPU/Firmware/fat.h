//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//смещения в FAT
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//BOOT-сектор и структура BPB
#define BS_jmpBoot            0
#define BS_OEMName            3
#define BPB_BytsPerSec        11
#define BPB_SecPerClus        13
#define BPB_ResvdSecCnt        14
#define BPB_NumFATs            16
#define BPB_RootEntCnt        17
#define BPB_TotSec16        19
#define BPB_Media            21
#define BPB_FATSz16            22
#define BPB_SecPerTrk        24
#define BPB_NumHeads        26
#define BPB_HiddSec            28
#define    BPB_TotSec32        32
#define BS_DrvNum            36
#define BS_Reserved1        37
#define BS_BootSig            38
#define BS_VolID            39
#define BS_VolLab            43
#define BS_FilSysType        54
#define BPB_FATSz32            36
#define BPB_ExtFlags        40
#define BPB_FSVer            42
#define BPB_RootClus        44
#define BPB_FSInfo            48
#define BPB_BkBootSec        50
#define BPB_Reserved        52

//тип файловой системы
#define FAT12 0
#define FAT16 1
#define FAT32 2

//последний кластер
#define FAT16_EOC 0xFFF8UL

enum FAT_RETURN_CODE
{
 FAT_16_OK,
 FAT_12_ERROR,
 FAT_32_ERROR,
};

//----------------------------------------------------------------------------------------------------
//глобальные переменные
//----------------------------------------------------------------------------------------------------
unsigned char Sector[512];//данные для сектора
unsigned long LastReadSector=0xffffffff;//последний считанный сектор
unsigned long FATOffset=0;//смещение FAT

//структура доля поиска файлов внутри каталога
struct SFATRecordPointer
{
 unsigned long BeginFolderAddr;//начальный адрес имён файлов внутри директории
 unsigned long CurrentFolderAddr;//текущий адрес имён файлов внутри директории
 unsigned long BeginFolderCluster;//начальный кластер имени файла внутри директории
 unsigned long CurrentFolderCluster;//текущий кластер имени файла внутри директории
 unsigned long BeginFolderClusterAddr;//начальный адрес текущего кластера
 unsigned long EndFolderClusterAddr;//конечный адрес имён файлов внутри директории (или кластера)
} sFATRecordPointer;

unsigned long FirstRootFolderAddr;//начальный адрес корневой директории

unsigned long SecPerClus;//количество секторов в кластере
unsigned long BytsPerSec;//количество байт в секторе
unsigned long ResvdSecCnt;//размер резервной области

unsigned long FATSz;//размер таблицы FAT
unsigned long DataSec;//количество секторов в регионе данных диска
unsigned long RootDirSectors;//количество секторов, занятых корневой директорией
unsigned long CountofClusters;//количество кластеров для данных (которые начинаются с номера 2! Это КОЛИЧЕСТВО, а не номер последнего кластера)
unsigned long FirstDataSector;//первый сектор данных
unsigned long FirstRootFolderSecNum;//начало корневой директории (для FAT16 - это сектор и отдельная область, для FAT32 - это ФАЙЛ в области данных с кластером BPB_RootClus)
unsigned long ClusterSize;//размер кластера в байтах

unsigned char FATType=FAT12;//тип файловой системы

//----------------------------------------------------------------------------------------------------
//прототипы функций
//----------------------------------------------------------------------------------------------------
enum FAT_RETURN_CODE FAT_Init(void);//инициализация FAT
unsigned long GetByte(unsigned long offset);//считать байт
unsigned long GetShort(unsigned long offset);//считать два байта
unsigned long GetLong(unsigned long offset);//считать 4 байта

bool FAT_RecordPointerStepForward(struct SFATRecordPointer *sFATRecordPointerPtr);//переместиться по записи вперёд
bool FAT_RecordPointerStepReverse(struct SFATRecordPointer *sFATRecordPointerPtr);//переместиться по записи назад

bool FAT_BeginFileSearch(void);//начать поиск файла в кталоге
bool FAT_PrevFileSearch(void);//вернуться к предыдущему файлу в каталоге
bool FAT_NextFileSearch(void);//продложить поиск файла в каталоге
bool FAT_GetFileSearch(char *filename,unsigned long *FirstCluster,unsigned long *Size,signed char *directory);//получить параметры текущего найденного файла в каталоге
bool FAT_EnterDirectory(unsigned long FirstCluster);//зайти в директорию и найти первый файл
bool FAT_OutputFile(void);//вывести файл на экран
//----------------------------------------------------------------------------------------------------
//Инициализация FAT
//----------------------------------------------------------------------------------------------------
enum FAT_RETURN_CODE FAT_Init(void)
{
 LastReadSector=0xffffffffUL;
 //ищем FAT
 FATOffset=0;
 for(unsigned long fo=0;fo<33554432UL;fo++)
 {
  unsigned char b=GetByte(fo);
  if (b==233 || b==235)
  {
   b=GetByte(fo+511UL);
   if (b==170)
   {
    b=GetByte(fo+510UL);
    if (b==85)
    {
     FATOffset=fo;
     break;
    }
   }
  }
 }
 LastReadSector=0xffffffffUL;

 SecPerClus=GetByte(BPB_SecPerClus);//количество секторов в кластере
 BytsPerSec=GetShort(BPB_BytsPerSec);//количество байт в секторе
 ResvdSecCnt=GetShort(BPB_ResvdSecCnt);//размер резервной области

 //определяем количество секторов, занятых корневой директорией
 RootDirSectors=(unsigned long)(ceil((GetShort(BPB_RootEntCnt)*32UL+(BytsPerSec-1UL))/BytsPerSec));
 //определяем размер таблицы FAT
 FATSz=GetShort(BPB_FATSz16);//размер одной таблицы FAT в секторах
 if (FATSz==0) FATSz=GetLong(BPB_FATSz32);
 //определяем количество секторов в регионе данных диска
 unsigned long TotSec=GetShort(BPB_TotSec16);//общее количество секторов на диске
 if (TotSec==0) TotSec=GetLong(BPB_TotSec32);
 DataSec=TotSec-(ResvdSecCnt+GetByte(BPB_NumFATs)*FATSz+RootDirSectors);
 //определяем количество кластеров для данных (которые начинаются с номера 2! Это КОЛИЧЕСТВО, а не номер последнего кластера)
 CountofClusters=(unsigned long)floor(DataSec/SecPerClus);
 //определяем первый сектор данных
 FirstDataSector=ResvdSecCnt+(GetByte(BPB_NumFATs)*FATSz)+RootDirSectors;
 //определим тип файловой системы

 FATType=FAT12;
 if (CountofClusters<4085UL)
 {
  _delay_ms(5000);
  FATType=FAT12;
 }
 else
 {
  if (CountofClusters<65525UL)
  {
   _delay_ms(2000);
   FATType=FAT16;
  }
  else
  {
   _delay_ms(5000);
   FATType=FAT32;
  }
 }
 if (FATType==FAT12) return(FAT_12_ERROR);//не поддерживаем
 if (FATType==FAT32) return(FAT_32_ERROR);//не поддерживаем
 //определяем начало корневой директории (для FAT16 - это сектор и отдельная область, для FAT32 - это ФАЙЛ в области данных с кластером BPB_RootClus)
 FirstRootFolderSecNum=ResvdSecCnt+(GetByte(BPB_NumFATs)*FATSz);
 ClusterSize=SecPerClus*BytsPerSec;//размер кластера в байтах

 //читаем корневую директорию
 FirstRootFolderAddr=FirstRootFolderSecNum*BytsPerSec;//начальный адрес корневой директории
 //настраиваем структуру для поиска внутри директории
 sFATRecordPointer.BeginFolderAddr=FirstRootFolderAddr;//начальный адрес имён файлов внутри директории
 sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;//текущий адрес имён файлов внутри директории
 sFATRecordPointer.BeginFolderCluster=0;//начальный кластер имени файла внутри директории
 sFATRecordPointer.CurrentFolderCluster=0;//текущий кластер имени файла внутри директории
 sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//конечный адрес имён файлов внутри директории (или кластера)
 sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.CurrentFolderAddr;//адрес начального кластера директории
 return(FAT_16_OK);
}
//----------------------------------------------------------------------------------------------------
//переместиться по записи вперёд
//----------------------------------------------------------------------------------------------------
bool FAT_RecordPointerStepForward(struct SFATRecordPointer *sFATRecordPointerPtr)
{
 sFATRecordPointerPtr->CurrentFolderAddr+=32UL;//переходим к следующей записи
 if (sFATRecordPointerPtr->CurrentFolderAddr>=sFATRecordPointerPtr->EndFolderClusterAddr)//вышли за границу кластера или директории
 {
  if (sFATRecordPointerPtr->BeginFolderAddr==FirstRootFolderAddr)//если у нас закончилась корневая директория
  {
   return(false);
  }
  else//для не корневой директории узнаём новый адрес кластера
  {
   unsigned long FATClusterOffset=0;//смещение по таблице FAT в байтах (в FAT32 они 4-х байтные, а в FAT16 - двухбайтные)
   if (FATType==FAT16) FATClusterOffset=sFATRecordPointerPtr->CurrentFolderCluster*2UL;//узнаём смещение в таблице FAT
   unsigned long NextClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//адрес следующего кластера
   //считываем номер следующего кластера файла
   unsigned long NextCluster=0;
   if (FATType==FAT16) NextCluster=GetShort(NextClusterAddr);
   if (NextCluster==0 || NextCluster>=CountofClusters+2UL || NextCluster>=FAT16_EOC)//такого кластера нет
   {
    return(false);
   }
   sFATRecordPointerPtr->CurrentFolderCluster=NextCluster;//переходим к следующему кластеру
   unsigned long FirstSectorofCluster=((sFATRecordPointerPtr->CurrentFolderCluster-2UL)*SecPerClus)+FirstDataSector;
   sFATRecordPointerPtr->CurrentFolderAddr=FirstSectorofCluster*BytsPerSec;
   sFATRecordPointerPtr->BeginFolderClusterAddr=sFATRecordPointerPtr->CurrentFolderAddr;
   sFATRecordPointerPtr->EndFolderClusterAddr=sFATRecordPointerPtr->CurrentFolderAddr+SecPerClus*BytsPerSec;
  }
 }
 return(true);
}
//----------------------------------------------------------------------------------------------------
//переместиться по записи назад
//----------------------------------------------------------------------------------------------------
bool FAT_RecordPointerStepReverse(struct SFATRecordPointer *sFATRecordPointerPtr)
{
 sFATRecordPointerPtr->CurrentFolderAddr-=32UL;//возвращаемся на запись назад
 if (sFATRecordPointerPtr->CurrentFolderAddr<sFATRecordPointerPtr->BeginFolderClusterAddr)//вышли за нижнюю границу кластера
 {
  if (sFATRecordPointerPtr->BeginFolderAddr==FirstRootFolderAddr)//если у нас корневая директория
  {
   return(false);//вышли за пределы директории
  }
  else//для не корневой директории узнаём новый адрес
  {
   unsigned long PrevCluster=sFATRecordPointerPtr->BeginFolderCluster;//предыдущий кластер
   while(1)
   {
    unsigned long FATClusterOffset=0;//смещение по таблице FAT в байтах (в FAT32 они 4-х байтные, а в FAT16 - двухбайтные)
    if (FATType==FAT16) FATClusterOffset=PrevCluster*2UL;
    unsigned long ClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//адрес предыдущего кластера
    unsigned long cluster=GetShort(ClusterAddr);
    if (cluster<=2 || cluster>=FAT16_EOC)//такого кластера нет
    {
     return(false);//вышли за пределы директории
    }
    if (cluster==sFATRecordPointerPtr->CurrentFolderCluster) break;//мы нашли предшествующий кластер
    PrevCluster=cluster;
   }
   if (PrevCluster<=2 || PrevCluster>=FAT16_EOC)//такого кластера нет
   {
    return(false);//вышли за пределы директории
   }
   sFATRecordPointerPtr->CurrentFolderCluster=PrevCluster;//переходим к предыдущему кластеру
   unsigned long FirstSectorofCluster=((sFATRecordPointerPtr->CurrentFolderCluster-2UL)*SecPerClus)+FirstDataSector;
   sFATRecordPointerPtr->BeginFolderClusterAddr=FirstSectorofCluster*BytsPerSec;
   sFATRecordPointerPtr->EndFolderClusterAddr=sFATRecordPointerPtr->BeginFolderClusterAddr+SecPerClus*BytsPerSec;
   sFATRecordPointerPtr->CurrentFolderAddr=sFATRecordPointerPtr->EndFolderClusterAddr-32UL;//на запись назад
  }
 }
 return(true);
}
//----------------------------------------------------------------------------------------------------
//начать поиск файла в кталоге
//----------------------------------------------------------------------------------------------------
bool FAT_BeginFileSearch(void)
{
 unsigned long FirstCluster;//первый кластер файла
 unsigned long Size;//размер файла
 signed char Directory;//не директория ли файл

 sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;
 sFATRecordPointer.CurrentFolderCluster=sFATRecordPointer.BeginFolderCluster;
 sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.CurrentFolderAddr;
 if (sFATRecordPointer.BeginFolderAddr!=FirstRootFolderAddr)//это не корневая директория
 {
  sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+SecPerClus*BytsPerSec;
 }
 else sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//конечный адрес имён файлов внутри директории (или кластера)
 //переходим к первому нужному нам файлу
 while(1)
 {
  if (FAT_GetFileSearch(NULL,&FirstCluster,&Size,&Directory)==false)
  {
   if (FAT_NextFileSearch()==false) return(false);
  }
  else return(true);
 }
 return(false);
}
//----------------------------------------------------------------------------------------------------
//перейти к предыдущему файлу в каталоге
//----------------------------------------------------------------------------------------------------
bool FAT_PrevFileSearch(void)
{
 struct SFATRecordPointer sFATRecordPointer_Copy=sFATRecordPointer;
 while(1)
 {
  if (FAT_RecordPointerStepReverse(&sFATRecordPointer_Copy)==false) return(false);
  //анализируем имя файла
  unsigned char n;
  bool res=true;
  for(n=0;n<11;n++)
  {
   unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+(unsigned long)(n));
   if (n==0)
   {
    if (b==0x20 || b==0xE5)
    {
     res=false;
     break;
    }
   }
   if (b<0x20)
   {
    res=false;
    break;
   }
   if (n==1)
   {
    unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr);
    unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+1UL);
    if (a==(unsigned char)'.' && b!=(unsigned char)'.')
    {
     res=false;
     break;
    }
   }
  }
  //смотрим расширение
  if (res==true)
  {
   unsigned char type=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+11UL);
   if ((type&0x10)==0)//это файл
   {
    unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+10UL);
    unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+9UL);
    unsigned char c=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+8UL);
    if (!(a=='G' && b=='M' && c=='I')) continue;//неверное расширение
   }
   sFATRecordPointer=sFATRecordPointer_Copy;
   return(true);
  }
 }
 return(false);
}
//----------------------------------------------------------------------------------------------------
//перейти к следующему файлу в каталоге
//----------------------------------------------------------------------------------------------------
bool FAT_NextFileSearch(void)
{
 struct SFATRecordPointer sFATRecordPointer_Copy=sFATRecordPointer;
 while(1)
 {
  if (FAT_RecordPointerStepForward(&sFATRecordPointer_Copy)==false) return(false);
  unsigned char n;
  bool res=true;
  for(n=0;n<11;n++)
  {
   unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+(unsigned long)(n));
   if (n==0)
   {
    if (b==0x20 || b==0xE5)
    {
     res=false;
     break;
    }
   }
   if (b<0x20)
   {
    res=false;
    break;
   }
   if (n==1)
   {
    unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr);
    unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+1UL);
    if (a==(unsigned char)'.' && b!=(unsigned char)'.')
    {
     res=false;
     break;
    }
   }
  }
  if (res==true)
  {
   unsigned char type=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+11UL);
   if ((type&0x10)==0)//это файл
   {
    unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+10UL);
    unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+9UL);
    unsigned char c=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+8UL);
    if (!(a=='G' && b=='M' && c=='I')) continue;//неверное расширение
   }
   sFATRecordPointer=sFATRecordPointer_Copy;
   return(true);
  }
 }
 return(false);
}
//----------------------------------------------------------------------------------------------------
//получить параметры текущего найденного файла в каталоге
//----------------------------------------------------------------------------------------------------
bool FAT_GetFileSearch(char *filename,unsigned long *FirstCluster,unsigned long *Size,signed char *directory)
{
 unsigned char n;
 bool res=true;
 *directory=0;
 if (filename!=NULL)
 {
  for(n=0;n<11;n++) filename[n]=32;
 }
 for(n=0;n<11;n++)
 {
  unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+(unsigned long)(n));
  if (n==0)
  {
   if (b==0x20 || b==0xE5)
   {
    res=false;
    break;
   }
  }
  if (b<0x20)
  {
   res=false;
   break;
  }
  if (filename!=NULL)
  {
   if (n<8) filename[n]=b;
       else filename[n+1]=b;
  }
  if (n==1)
  {
   unsigned char a=GetByte(sFATRecordPointer.CurrentFolderAddr);
   unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+1UL);
   if (a==(unsigned char)'.' && b!=(unsigned char)'.')
   {
    res=false;
    break;
   }
  }
 }
 if (res==true)
 {
  unsigned char type=GetByte(sFATRecordPointer.CurrentFolderAddr+11UL);
  if ((type&0x10)==0)//это файл
  {
   unsigned char a=GetByte(sFATRecordPointer.CurrentFolderAddr+10UL);
   unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+9UL);
   unsigned char c=GetByte(sFATRecordPointer.CurrentFolderAddr+8UL);
   if (!(a=='G' && b=='M' && c=='I')) return(false);//неверное расширение
  }
  else//если это директория
  {
   unsigned char a=GetByte(sFATRecordPointer.CurrentFolderAddr);
   unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+1UL);
   if (a==(unsigned char)'.' && b==(unsigned char)'.') *directory=-1;//на директорию выше
                                                   else *directory=1;//на директорию ниже
  }
  //первый кластер файла
  *FirstCluster=(GetShort(sFATRecordPointer.CurrentFolderAddr+20UL)<<16)|GetShort(sFATRecordPointer.CurrentFolderAddr+26UL);
  //узнаём размер файла в байтах
  *Size=GetLong(sFATRecordPointer.CurrentFolderAddr+28UL);
  if (filename!=NULL)
  {
   if ((type&0x10)==0) filename[8]='.';//файлу добавляем точку
   filename[12]=0;
   //поищем длинное имя файла
   struct SFATRecordPointer sFATRecordPointer_Local=sFATRecordPointer;
   unsigned char long_name_length=0;
   while(1)
   {
    if (FAT_RecordPointerStepReverse(&sFATRecordPointer_Local)==false) break;
    unsigned char attr=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+11UL);
    if (attr&0xf)//это длинное имя
    {
     //собираем полное имя
     unsigned char name_index=GetByte(sFATRecordPointer_Local.CurrentFolderAddr);
     for(n=0;n<10 && long_name_length<=16;n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+1UL);
     for(n=0;n<12 && long_name_length<=16;n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+14UL);
     for(n=0;n<4 && long_name_length<=16;n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+28UL);
     if (long_name_length>16) break;
     if (name_index&0x40) break;//последняя часть имени
    }
    else break;//это не длинное имя
   }
   if (long_name_length>16) long_name_length=16;
   if (long_name_length>0) filename[long_name_length]=0;
  }
  return(true);
 }
 return(false);
}
//----------------------------------------------------------------------------------------------------
//зайти в директорию и найти первый файл
//----------------------------------------------------------------------------------------------------
bool FAT_EnterDirectory(unsigned long FirstCluster)
{
 if (FirstCluster==0UL)//это корневая директория (номер первого кластера, распределяемого директории)
 {
  sFATRecordPointer.BeginFolderAddr=FirstRootFolderAddr;
  sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//конечный адрес имён файлов внутри директории (или кластера)
 }
 else
 {
  unsigned long FirstSectorofCluster=((FirstCluster-2UL)*SecPerClus)+FirstDataSector;
  sFATRecordPointer.BeginFolderAddr=FirstSectorofCluster*BytsPerSec;//начальный адрес имён файлов внутри директории
  sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+SecPerClus*BytsPerSec;
 }
 sFATRecordPointer.BeginFolderCluster=FirstCluster;//начальный кластер имени файла внутри директории
 sFATRecordPointer.CurrentFolderCluster=sFATRecordPointer.BeginFolderCluster;//текущий кластер имени файла внутри директории
 sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;//текущий адрес имён файлов внутри директории
 sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.BeginFolderAddr;
 return(FAT_BeginFileSearch());
}
//----------------------------------------------------------------------------------------------------
//отобразить файл на экране
//----------------------------------------------------------------------------------------------------
bool FAT_OutputFile(void)
{
 char string[20];
 unsigned long CurrentCluster;
 unsigned long Size;

 int x=0;
 int y=0;

 unsigned long i=0;//номер считываемого байта файла
 signed char Directory;//не директория ли файл
 if (FAT_GetFileSearch(string,&CurrentCluster,&Size,&Directory)==false) return(false);
 while(i<Size)
 {
  //считываем данные
  unsigned long length=ClusterSize;
  if (length+i>=Size) length=Size-i;
  //получаем первый сектор кластера
  unsigned long FirstSectorofCluster=((CurrentCluster-2UL)*SecPerClus)+FirstDataSector;
  unsigned long addr=FirstSectorofCluster*BytsPerSec;
  for(unsigned long m=0;m<length;m++,i++)
  {
   unsigned char b=GetByte(addr+m);
   SetColor(y,x,b);
   x++;
   if (x>250)
   {
    x=0;
    y++;
   }
   if (y>=240) y=240;
  }
  //переходим к следующему кластеру файла
  unsigned long FATClusterOffset=0;//смещение по таблице FAT в байтах (в FAT32 они 4-х байтные, а в FAT16 - двухбайтные)
  if (FATType==FAT16) FATClusterOffset=CurrentCluster*2UL;
  unsigned long NextClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//адрес следующего кластера
  //считываем номер следующего кластера файла
  unsigned long NextCluster=0;
  if (FATType==FAT16) NextCluster=GetShort(NextClusterAddr);
  if (NextCluster==0) break;//неиспользуемый кластер
  if (NextCluster>=CountofClusters+2UL) break;//номер больше максимально возможного номера кластера - конец файла или сбой
  CurrentCluster=NextCluster;
 }
 //конец файла
 return(false);
}
//----------------------------------------------------------------------------------------------------
//считать байт
//----------------------------------------------------------------------------------------------------
unsigned long GetByte(unsigned long offset)
{
 offset+=FATOffset;
 unsigned long s=offset>>9UL;//делим на 512
 if (s!=LastReadSector)
 {
  LastReadSector=s;
  SD_ReadBlock(offset&0xfffffe00UL,Sector);
  //ошибки не проверяем, всё равно ничего сделать не сможем - либо работает, либо нет
 }
 return(Sector[offset&0x1FFUL]);
}
//----------------------------------------------------------------------------------------------------
//считать два байта
//----------------------------------------------------------------------------------------------------
unsigned long GetShort(unsigned long offset)
{
 unsigned long v=GetByte(offset+1UL);
 v<<=8UL;
 v|=GetByte(offset);
 return(v);
}
//----------------------------------------------------------------------------------------------------
//считать 4 байта
//----------------------------------------------------------------------------------------------------
unsigned long GetLong(unsigned long offset)
{
 unsigned long v=GetByte(offset+3UL);
 v<<=8UL;
 v|=GetByte(offset+2UL);
 v<<=8UL;
 v|=GetByte(offset+1UL);
 v<<=8UL;
 v|=GetByte(offset);
 return(v);
}


