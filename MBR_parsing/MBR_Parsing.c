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

//No need for packing compiler directive\\

//=================================<MBR partition table typedef>
typedef struct 
{
	uint8_t Active;
	uint8_t CHS_Start[3];
	uint8_t Type;
	uint8_t CHS_End[3];
	uint32_t Start_Sector;
	uint32_t NumOfSectors;	

} PartitionTable_Entry_t;

typedef struct
{
	uint64_t PartitionType[2];
	uint64_t UniquePartitionGUID[2];
	uint64_t FirstLBA;
	uint64_t SecondLBA;
	uint64_t AttributeFlag;
	uint64_t PartitionName[9];

} GPT_PartitionEntry_t;

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
//=========================================================================================


//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//				                      Implementation
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
void main(int argc, char** argv)
{
	char buf[Sector_Size];
	char Buffer[200];
	int fd = 0, i = 0, flag = 0, c = 5;
	PartitionTable_Entry_t* MBR_EntryX = NULL;
	const char* MBR_format1 = "%-15s %-15s %-15s %-15s %-15s %-15s %-15s";
	const char* GPT_format1 = "%-15s %-15s %-15s %-15s %-15s";
	const char* MBR_format2 = "%s%-9d %-15c %-15u %-15u %-15u %-15u";
	const char* GPT_format2 = "%s%-9d %-15u %-15u %-15u %-15u";
	uint32_t Selector_Counter = 0, Extended_StartSector = 0;

	if (argc <  1)
	{
		if ( write(STDOUT_FILENO, "Not enough arguments", strlen("Not enough arguments")) )
		{
			perror("write");	
		}
		
		return;
	}
	
	if( ( fd = open(argv[1], O_RDONLY) ) == -1 )
	{
		perror("open");
		return;
	}

	if ( read(fd, buf, sizeof(buf)) == -1 )
	{
		perror("read");
		return;
	}
	
	MBR_EntryX = (PartitionTable_Entry_t*)&buf[446];

	if ( MBR_EntryX->Type == GPT_Protective) //Partition type is GPT
	{
		GPT_PartitionEntry_t* GPT_EntryX = (GPT_PartitionEntry_t*)&buf;

		if (lseek(fd, ( (uint64_t)2 ) * Sector_Size, SEEK_SET) == -1)
		{
			perror("lseek");
			fclose(fd);
			return;
		}

		if ( read(fd, buf, sizeof(buf)) == -1 ) //For right now buf size is enough
		{
			perror("read");
			return;
		}

		snprintf(Buffer, sizeof(Buffer), GPT_format1, "Device", "Start", "End", "Sectors", "Size");
		printf("%s\n",Buffer);

		while (GPT_EntryX->FirstLBA != 0)
		{
			snprintf(Buffer, sizeof(Buffer), GPT_format2, 
					"nvme1np",
			    	i + 1,
			    	GPT_EntryX->FirstLBA,
			    	GPT_EntryX->SecondLBA,
			    	GPT_EntryX->SecondLBA - GPT_EntryX->FirstLBA + 1,
			    	( (uint64_t)( (GPT_EntryX->SecondLBA - GPT_EntryX->FirstLBA + 1) * Sector_Size ) / (1024 * 1024 * 1024) ) );

			printf("%s\n", Buffer);
			
			++GPT_EntryX;
			++i;
		}

	
	
	} else  //Partion Type is MBR
	{
		snprintf(Buffer, sizeof(Buffer), MBR_format1, "Device", "Boot", "Start", "End", "Sectors", "Size", "Type");
		printf("%s\n",Buffer);

		for (i = 0; i < 4; ++i)
		{
			if (MBR_EntryX[i].NumOfSectors != 0)
			{
				snprintf(Buffer, sizeof(Buffer), MBR_format2, 
				"nvme1np",
			    i + 1,
			    MBR_EntryX[i].Active == 0x80 ? '*' : ' ',
			    MBR_EntryX[i].Start_Sector,
			    MBR_EntryX[i].Start_Sector + MBR_EntryX[i].NumOfSectors - 1,
			    MBR_EntryX[i].NumOfSectors,
			    ( ( (uint64_t)MBR_EntryX[i].NumOfSectors * Sector_Size ) / (1024 * 1024 * 1024) ) );

			printf("%s\n", Buffer);

			}
		}

		i = 0;
		while ( i < 4 )
		{
			if (MBR_EntryX[i].Type == 0x05)
			{
				flag = 1;
				break;
			}

			++i;
		}

		Selector_Counter = MBR_EntryX[i].Start_Sector;
		Extended_StartSector = Selector_Counter;

		if ( flag == 1 ) /*there is a extended partition*/
		{
			while ( MBR_EntryX[i].NumOfSectors != 0 )
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

				MBR_EntryX = ( PartitionTable_Entry_t* )&buf[446];
			
				snprintf(Buffer, sizeof(Buffer), MBR_format2, 
						"nvme1n",
			        	c,
			        	MBR_EntryX->Active == 0x80 ? '*' : ' ',
			        	MBR_EntryX->Start_Sector + Selector_Counter,
			        	MBR_EntryX->Start_Sector + Selector_Counter + MBR_EntryX->NumOfSectors - 1,
			        	MBR_EntryX->NumOfSectors,
			        	( ( (uint64_t)(MBR_EntryX->NumOfSectors) * Sector_Size ) / (1024 * 1024 * 1024) ) );

				printf("%s\n", Buffer);
				c++; //increase partition number.

				if ( MBR_EntryX[1].NumOfSectors != 0)
				{
					Selector_Counter = Extended_StartSector + MBR_EntryX[1].NumOfSectors;
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
