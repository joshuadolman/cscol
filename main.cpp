#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define internal static

#define Assert(x)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef bool b32;

struct file {
    s64 Size;
    char* Data;
};

struct weighting_entry {
   u64 DiscordID;
   
   char* Name;
   u8 NameLength;

   union {
      u8 Weights[5];
      struct {
         u8 Blue;
         u8 Green;
         u8 Orange;
         u8 Purple;
         u8 Yellow;
      };
   };
};

struct weighting_list {
   weighting_entry* Entries;
   s64 Count;
};

internal void*
Allocate(s64 Size)
{
   return VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

internal b32
Free(void* Memory)
{
   return VirtualFree(Memory, NULL, MEM_DECOMMIT | MEM_RELEASE);
}

internal file
ReadEntireFile(const char* Filename)
{
   file Result = {0};
   HANDLE OpenedFile = CreateFileA(Filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

   if ( GetLastError()!=0 )
   {
      CloseHandle(OpenedFile);
      return Result;
   }

   LARGE_INTEGER FileSize = {0};
   if ( GetFileSizeEx(OpenedFile, &FileSize)==0 )
   {
      CloseHandle(OpenedFile);
      return Result;
   }

   Result.Size = FileSize.QuadPart;
   Result.Data = (char*)Allocate(Result.Size);

   if (!Result.Data)
   {
      CloseHandle(OpenedFile);
      return Result;
   }

   DWORD BytesRead = 0;
   if (!ReadFile(OpenedFile, Result.Data, Result.Size, &BytesRead, NULL) || BytesRead!=Result.Size)
   {
      CloseHandle(OpenedFile);
      Free(Result.Data);
      Result = {0};
      return Result;
   }


   CloseHandle(OpenedFile);
   return Result;
}

#define MAX_COL_COUNT (5)

internal void
MemCopy(char* Src, char* Dst, s64 Count)
{
   for (int i=0; i < Count; i++)
   {
      *(Dst+i) = *(Src+i);
   }
}

internal void
PrintEntry(weighting_entry E)
{
   if (E.DiscordID==0) {
      printf("%s, ID: NO_DISCORD_ID_STORED, {B:%d, G:%d, O:%d, P:%d, Y:%d}\n", E.Name, E.Blue, E.Green, E.Orange, E.Purple, E.Yellow);
   } else {
      printf("%s, ID: %lld, {B:%d, G:%d, O:%d, P:%d, Y:%d}\n", E.Name, E.DiscordID, E.Blue, E.Green, E.Orange, E.Purple, E.Yellow);
   }
}

internal u64
StrToU64(const char* String, s64 Length)
{
   u64 Result = 0;
   for(int i = 0; i < Length; ++i)
   {
      Result = Result * 10 + (String[i] - '0');
   }
   return Result;
}

internal weighting_list
ParseCSV(file File)
{
   char* At = File.Data;
   s64 LineCount = 0;
   s64 SkipCount = 0;
   while (*At)
   {
      if (*At=='\n')
      {
         if (!SkipCount) {
            SkipCount = (At - File.Data) + 1;
         }
         LineCount++;
      }
      At++;
   }


   // printf("LineCount %lld\n", LineCount);
   // printf("%s\n", At);

   weighting_list Result = {0};
   Result.Count = LineCount;
   Result.Entries = (weighting_entry*)Allocate(sizeof(weighting_entry)*Result.Count);
   if (!Result.Entries) {
      Result = {0};
      return Result;
   }

   At = File.Data + SkipCount;
   s64 CurrentEntry = 0;
   s64 CurrentCol = 0;
   char* LineStart = At;
   while (*At)
   {

      while (*At) // Loop over each line of the CSV
      {
         weighting_entry* CurEntry = &Result.Entries[CurrentEntry]; // Convenience
         if (CurEntry->Name==0) // Name has not yet been set, so we are currently at that stage.
         {
            while (*At!=',') // Move At point to just past the name.
            {
               At++;
            }
            CurEntry->NameLength = At - LineStart;
            CurEntry->Name = (char*)Allocate(CurEntry->NameLength + 1); // (TODO) Get from string allocator

            MemCopy(LineStart, CurEntry->Name, CurEntry->NameLength); // We now have the name
         }

         At++;
         if (*At==',') // There is no discord ID currently in the file
         {
            // Go get discord ID !
         } else { // Discord ID is in the file, so we parse
            LineStart = At;
            while (*At!=',')
            {
               At++;
            }
            CurEntry->DiscordID = StrToU64(LineStart, At - LineStart);
            
         }

         At++;
         LineStart = At;
         while(CurrentCol < MAX_COL_COUNT)
         {
            if (*At==',' || *At=='\n' || *At=='\r' || *At=='\0') {
               CurEntry->Weights[CurrentCol] = (u8)StrToU64(LineStart, At - LineStart);
               CurrentCol++;
               if (*At=='\r') { At++; }
               LineStart = At + 1;
            }

            At++;
         }
         
         CurrentCol=0;
         CurrentEntry++; // This should only happen at the very end after all the data has been filled out to end the line iteration loop
      }

      if (*At!='\0') { At++; }

   }

   return Result;
}

int
main(int argc, char* argv[])
{
   file File = ReadEntireFile("friendships.rnk");
   weighting_list List = ParseCSV(File);
   // printf("%s\n", File.Data);
   for (int i=0; i < List.Count; i++)
   {
      PrintEntry(List.Entries[i]);
   }
   return 0;
}