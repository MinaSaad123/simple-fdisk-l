#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//				                    User define datatype
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

//=================================<partition table typedef>
typedef struct 
{
	uint8_t Active;
	uint8_t CHS_Start[3];
	uint8_t Type;
	uint8_t CHS_End[3];
	uint32_t Start_Sector;
	uint32_t NumOfSectors;	

} PartitionTable_Entry_t;

//================================<Type Typedef>
typedef enum
{
	GPT_Protective = 0xEE
	//todo: Continue writing possible type for MBR.
	
} MBR_Type_t;
//=========================================================================================


//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//				                          Macros
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
#define Sector_Size   512
#define PATH "/dev/nvme1n1"
//=========================================================================================


//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//				                      Implementation
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
void main(int argc, char** argv)
{
	char buf[Sector_Size];
	char Buffer[200];
	int fd = 0, i = 0, flag = 0, c = 5;
	PartitionTable_Entry_t* EntryX = NULL;
	const char* format1 = "%-15s %-15s %-15s %-15s %-15s %-15s %-15s";
	const char* format2 = "%s%-9d %-15c %-15u %-15u %-15u %-15u";
	uint32_t Selector_Counter = 0, Extended_StartSector = 0;

	if (argc <  1)
	{
		if ( write(STDOUT_FILENO, "Not enough arguments", strlen("Not enough arguments")) )
		{
			perror("write");	
		}
		
		return;
	}
	
	if( ( fd = open(PATH, O_RDONLY) ) == -1)
	{
		perror("open");
		return;
	}

	if ( read(fd, buf, sizeof(buf)) == -1)
	{
		perror("read");
		return;
	}
	
	EntryX = (PartitionTable_Entry_t*)&buf[446];

	if ( EntryX->Type == GPT_Protective) //Partition type is GPT
	{
		//todo
	
	} else  //Partion Type is MBR
	{
		snprintf(Buffer, sizeof(Buffer), format1, "Device", "Boot", "Start", "End", "Sectors", "Size", "Type");
		printf("%s\n",Buffer);

		for (i = 0; i < 4; ++i)
		{
			if (EntryX[i].NumOfSectors != 0)
			{
				snprintf(Buffer, sizeof(Buffer), format2, 
				"nvme1n",
			    i + 1,
			    EntryX[i].Active == 0x80 ? '*' : ' ',
			    EntryX[i].Start_Sector,
			    EntryX[i].Start_Sector + EntryX[i].NumOfSectors - 1,
			    EntryX[i].NumOfSectors,
			    ( ( (uint64_t)EntryX[i].NumOfSectors * Sector_Size ) / (1024 * 1024 * 1024) ) );

			printf("%s\n", Buffer);

			}
		}

		i = 0;
		while ( i < 4 )
		{
			if (EntryX[i].Type == 0x05)
			{
				flag = 1;
				break;
			}

			++i;
		}

		Selector_Counter = EntryX[i].Start_Sector;
		Extended_StartSector = Selector_Counter;

		if ( flag == 1 ) /*there is a extended partition*/
		{
			while ( EntryX[i].NumOfSectors != 0 )
			{
				if (lseek(fd, ( (uint64_t)Selector_Counter) * Sector_Size, SEEK_SET) == -1)
				{
					perror("lseek");
					fclose(fd);
					return;
				}
				if ( read(fd, buf, sizeof(buf)) == -1)
				{
					perror("read");
					fclose(fd);
					return;
				}

				EntryX = ( PartitionTable_Entry_t* )&buf[446];
			
				snprintf(Buffer, sizeof(Buffer), format2, 
						"nvme1n",
			        	c,
			        	EntryX->Active == 0x80 ? '*' : ' ',
			        	EntryX->Start_Sector + Selector_Counter,
			        	EntryX->Start_Sector + Selector_Counter + EntryX->NumOfSectors - 1,
			        	EntryX->NumOfSectors,
			        	( ( (uint64_t)(EntryX->NumOfSectors) * Sector_Size ) / (1024 * 1024 * 1024) ) );

				printf("%s\n", Buffer);
				c++; //increase partition number.

				if ( EntryX[1].NumOfSectors != 0)
				{
					Selector_Counter = Extended_StartSector + EntryX[1].NumOfSectors;
					i= 0;

				} else
				{
					break;
				}
			}
		}	
	}
}
//=========================================================================================
