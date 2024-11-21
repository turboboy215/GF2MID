/*Game Freak (GB/GBC) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
int format = 1;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long seqPtrs[4];
long songPtr;
long bankAmt;
int foundTable = 0;
long firstPtr = 0;
int lowNibble1 = 0;
int highNibble1 = 0;
int lowNibble2 = 0;
int highNibble2 = 0;
int totalChan = 0;
int chanNum = 0;
long startMus = 0;
int activeChan[4];
int curTrack = 0;
int masterBank = 0;
long songLoc = 0;

unsigned static char* romData;
unsigned static char* exRomData;

const char MagicBytesA[3] = { 0xFF, 0xFF, 0xFF };
const char MagicBytesB[4] = { 0x73, 0x23, 0x72, 0x21};

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
unsigned short ReadBE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptrs[4]);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

/*Store big-endian pointer*/
unsigned short ReadBE16(unsigned char* Data)
{
	return (Data[0] << 8) | (Data[1] << 0);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("Game Freak (GB/GBC) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: GF2TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
			fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);

			/*Try to search the bank for song table loader*/
			/*Earlier version (Mario & Yoshi, Pokémon: Red/Blue/Yellow Version)*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesA, 3)) && foundTable != 1)
				{
					tableOffset = i + bankAmt;
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					format = 1;
					printf("Detected version: 1\n");
				}
			}

			/*Later version*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesB, 4)) && foundTable != 1 && romData[i + 6] == 0x19)
				{
					tablePtrLoc = bankAmt + i + 4;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					/*Multi-banked variant (Pokémon: Gold/Silver/Crystal Version)*/
					if (romData[i + 8] == 0x19)
					{
						printf("Detected version: 2 (multi-banked)\n");
						format = 3;
					}
					/*Single-banked variant (Pokémon Pinball)*/
					else
					{
						printf("Detected version: 2 (single-banked)\n");
						format = 2;
					}
					foundTable = 1;
				}
			}

			if (foundTable == 1)
			{
				if (format == 1)
				{
					i = tableOffset - bankAmt + 3;
					/*Skip the sound effects*/

					masterBank = bank;
					startMus = 0;
					chanNum = 5;
					firstPtr = ReadLE16(&romData[i + 1]);
					while (startMus == 0 && i < 0x4000)
					{
						lowNibble1 = (romData[i] >> 4);
						highNibble1 = (romData[i] & 15);
						switch (lowNibble1)
						{
						case 0x00:
							totalChan = 1;
							break;
						case 0x04:
							totalChan = 2;
							break;
						case 0x08:
							totalChan = 3;
							break;
						case 0x0C:
							totalChan = 4;
							break;
						}
						chanNum = highNibble1;

						if (chanNum > 3)
						{
							i += (3 * totalChan);
						}
						else
						{
							if (totalChan > 2)
							{
								startMus = i + bankAmt;
								printf("Music pointers start at offset: 0x%01X\n", startMus);
								break;
							}
							else
							{
								i += (3 * totalChan);
							}
						}
					}

					/*Get the pointers*/
					if (startMus > 0)
					{
						songNum = 1;
						
						lowNibble2 = (romData[i] >> 4);
						highNibble2 = (romData[i] & 15);
						while (i != (firstPtr - bankAmt) && i < bankSize && highNibble2 < 0x05)
						{
							lowNibble2 = (romData[i] >> 4);
							highNibble2 = (romData[i] & 15);
							switch (lowNibble2)
							{
							case 0x00:
								totalChan = 1;
								break;
							case 0x04:
								totalChan = 2;
								break;
							case 0x08:
								totalChan = 3;
								break;
							case 0x0C:
								totalChan = 4;
								break;
							}
							for (curTrack = 0; curTrack < 4; curTrack++)
							{
								seqPtrs[curTrack] = -1;
							}
							printf("Song %i number of channels: %i\n", songNum, totalChan);

							for (curTrack = 0; curTrack < totalChan; curTrack++)
							{
								lowNibble1 = (romData[i] >> 4);
								highNibble1 = (romData[i] & 15);
								if (highNibble1 < 4)
								{
									seqPtrs[curTrack] = ReadLE16(&romData[i + 1]);
									printf("Song %i channel %i: 0x%04X\n", songNum, (highNibble1 + 1), seqPtrs[curTrack]);
								}
								i += 3;
							}
							fseek(rom, ((bank - 1)* bankSize), SEEK_SET);
							exRomData = (unsigned char*)malloc(bankSize);
							fread(exRomData, 1, bankSize, rom);

							song2txt(songNum, seqPtrs);
							songNum++;
						}

					}

				}

				else if (format == 2)
				{
					/*Get the pointers*/
					songNum = 1;
					i = tableOffset - bankAmt;
					masterBank = bank;
					while (ReadLE16(&romData[i]) > bankAmt && ReadLE16(&romData[i]) < bankSize * 2)
					{
						songLoc = ReadLE16(&romData[i]);
						printf("Song %i: 0x%04X\n", songNum, songLoc);
						j = songLoc - bankAmt;

						for (curTrack = 0; curTrack < 4; curTrack++)
						{
							seqPtrs[curTrack] = -1;
						}
						lowNibble1 = (romData[j] >> 4);
						highNibble1 = (romData[j] & 15);
						switch (lowNibble1)
						{
						case 0x00:
							totalChan = 1;
							break;
						case 0x04:
							totalChan = 2;
							break;
						case 0x08:
							totalChan = 3;
							break;
						case 0x0C:
							totalChan = 4;
							break;
						}
						printf("Song %i number of channels: %i\n", songNum, totalChan);
						for (curTrack = 0; curTrack < totalChan; curTrack++)
						{
							lowNibble1 = (romData[j] >> 4);
							highNibble1 = (romData[j] & 15);
							if (highNibble1 < 4)
							{
								seqPtrs[curTrack] = ReadLE16(&romData[j + 1]);
								printf("Song %i channel %i: 0x%04X\n", songNum, (highNibble1 + 1), seqPtrs[curTrack]);
							}
							j += 3;
						}
						fseek(rom, ((bank - 1)* bankSize), SEEK_SET);
						exRomData = (unsigned char*)malloc(bankSize);
						fread(exRomData, 1, bankSize, rom);
						song2txt(songNum, seqPtrs);

						songNum++;
						i += 2;
					}
				}

				else if (format == 3)
				{
					/*Get the pointers*/
					songNum = 1;
					i = tableOffset - bankAmt;
					while (ReadLE16(&romData[i + 1]) >= bankAmt && ReadLE16(&romData[i + 1]) < bankSize * 2)
					{
						bank = romData[i] + 1;
						songLoc = ReadLE16(&romData[i + 1]);
						printf("Song %i: 0x%04X\n", songNum, songLoc);
						j = songLoc - bankAmt;

						for (curTrack = 0; curTrack < 4; curTrack++)
						{
							seqPtrs[curTrack] = -1;
						}

						fseek(rom, ((bank - 1)* bankSize), SEEK_SET);
						exRomData = (unsigned char*)malloc(bankSize);
						fread(exRomData, 1, bankSize, rom);
						lowNibble1 = (exRomData[j] >> 4);
						highNibble1 = (exRomData[j] & 15);
						switch (lowNibble1)
						{
						case 0x00:
							totalChan = 1;
							break;
						case 0x04:
							totalChan = 2;
							break;
						case 0x08:
							totalChan = 3;
							break;
						case 0x0C:
							totalChan = 4;
							break;
						}
						printf("Song %i number of channels: %i\n", songNum, totalChan);
						for (curTrack = 0; curTrack < totalChan; curTrack++)
						{
							lowNibble1 = (exRomData[j] >> 4);
							highNibble1 = (exRomData[j] & 15);
							if (highNibble1 < 4)
							{
								seqPtrs[curTrack] = ReadLE16(&exRomData[j + 1]);
								printf("Song %i channel %i: 0x%04X\n", songNum, (highNibble1 + 1), seqPtrs[curTrack]);
							}
							j += 3;
						}
						song2txt(songNum, seqPtrs);

						songNum++;
						i += 3;
					}
				}
				fclose(rom);
				printf("The operation was completed successfully!\n");
				exit(0);
			}
			else
			{
				printf("ERROR: Magic bytes not found!\n");
				exit(2);
			}

		}
	}
}

	void song2txt(int songNum, long ptrs[4])
	{
		int seqPos = 0;
		int curTrack = 0;
		int seqEnd = 0;
		int octave = 0;
		int tempo = 0;
		int curNote = 0;
		int curNoteLen = 0;
		long macroPos = 0;
		int macroTimes = 0;
		long macroRet = 0;
		long otherAddr = 0;
		int perfPitch = 0;
		int transpose = 0;
		int transposeKey = 0;
		int transposeOct = 0;
		int lowNibble = 0;
		int highNibble = 0;
		long pitchOffset = 0;
		int tempoRel = 0;
		unsigned int command[4];

		sprintf(outfile, "song%i.txt", songNum);
		if ((txt = fopen(outfile, "wb")) == NULL)
		{
			printf("ERROR: Unable to write to file song%i.txt!\n", songNum);
			exit(2);
		}
		else
		{
			for (curTrack = 0; curTrack < 4; curTrack++)
			{
				if (ptrs[curTrack] > bankAmt)
				{
					seqPos = ptrs[curTrack] - bankAmt;
					seqEnd = 0;
					transpose = 0;
					perfPitch = 0;
					fprintf(txt, "Channel %i:\n", curTrack + 1);
					if (format == 1)
					{
						while (seqEnd == 0)
						{
							command[0] = exRomData[seqPos];
							command[1] = exRomData[seqPos + 1];
							command[2] = exRomData[seqPos + 2];
							command[3] = exRomData[seqPos + 3];

							if (command[0] < 0xC0)
							{
								if (curTrack != 3)
								{
									lowNibble = (command[0] >> 4);
									highNibble = (command[0] & 15);
									curNote = lowNibble;
									curNoteLen = highNibble;
									fprintf(txt, "Play note: %i, length: %i\n", curNote, curNoteLen);
									seqPos++;
								}
								else if (curTrack == 3)
								{
									highNibble = (command[0] & 15);
									curNote = highNibble;
									curNoteLen = command[1];
									fprintf(txt, "Drum note: %i, length: %i\n", curNote, curNoteLen);
									seqPos += 2;
								}

							}

							else if (command[0] >= 0xC0 && command[0] < 0xD0)
							{
								highNibble = (command[0] & 15);
								curNoteLen = highNibble;
								fprintf(txt, "Rest, length: %i\n", curNoteLen);
								seqPos++;
							}

							else if (command[0] >= 0xD0 && command[0] < 0xE0)
							{
								if (curTrack != 3)
								{
									highNibble1 = (command[0] & 15);
									lowNibble = (command[1] >> 4);
									highNibble = (command[1] & 15);
									fprintf(txt, "Note type, speed: %i, volume: %i, fade: %i\n", highNibble1, lowNibble, highNibble);
									seqPos += 2;
								}
								else if (curTrack == 3)
								{
									highNibble = (command[0] & 15);
									fprintf(txt, "Set drum speed: %i\n", highNibble);
									seqPos++;
								}
							}

							else if (command[0] >= 0xE0 && command[0] <= 0xE7)
							{
								highNibble = (command[0] & 15);
								octave = highNibble;
								fprintf(txt, "Set octave: %i\n", octave);
								seqPos++;
							}

							else if (command[0] == 0xE8)
							{
								fprintf(txt, "Toggle 'perfect pitch'\n");
								seqPos++;
							}

							else if (command[0] == 0xE9)
							{
								fprintf(txt, "Command E9 (invalid)\n");
								seqPos++;
							}

							else if (command[0] == 0xEA)
							{
								lowNibble = (command[2] >> 4);
								highNibble = (command[2] & 15);
								fprintf(txt, "Vibrato: delay: %i, depth: %i, rate: %i\n", command[1], lowNibble, highNibble);
								seqPos += 3;
							}

							else if (command[0] == 0xEB)
							{
								lowNibble = (command[2] >> 4);
								highNibble = (command[2] & 15);
								fprintf(txt, "Pitch slide, delay: %i, depth: %i, rate: %i\n", command[1], lowNibble, highNibble);
								seqPos += 3;
							}

							else if (command[0] == 0xEC)
							{
								fprintf(txt, "Duty: %i\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xED)
							{
								tempo = ReadBE16(&exRomData[seqPos + 1]);
								fprintf(txt, "Set tempo: %i\n", tempo);
								seqPos += 3;
							}

							else if (command[0] == 0xEE)
							{
								fprintf(txt, "Set pan: %i\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xEF)
							{
								fprintf(txt, "Command EF\n");
								seqPos++;
							}

							else if (command[0] == 0xF0)
							{
								lowNibble = (command[1] >> 4);
								highNibble = (command[1] & 15);
								fprintf(txt, "Set master volume: %i, %i\n", lowNibble, highNibble);
								seqPos += 2;
							}

							else if (command[0] == 0xF1)
							{
								fprintf(txt, "Silent drum note?\n");
								seqPos++;
							}

							else if (command[0] == 0xF8)
							{
								fprintf(txt, "Interpret SFX data as music\n");
								seqPos++;
							}

							else if (command[0] == 0xFC)
							{
								fprintf(txt, "Duty cycle pattern: %i\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xFD)
							{
								macroPos = ReadLE16(&exRomData[seqPos + 1]);
								macroRet = seqPos + 3;
								fprintf(txt, "Call macro: 0x%04X\n", macroPos);
								seqPos += 3;
							}

							else if (command[0] == 0xFE)
							{
								macroPos = ReadLE16(&exRomData[seqPos + 2]);
								macroTimes = command[1];
								macroRet = seqPos + 4;
								if (macroTimes == 0)
								{
									fprintf(txt, "Go to macro infinite times (loop): 0x%04X\n\n", macroPos);
									seqEnd = 1;
								}
								else
								{
									fprintf(txt, "Go to macro %i times: 0x%04X\n", macroTimes, macroPos);
									seqPos += 4;
								}

							}

							else if (command[0] == 0xFF)
							{
								fprintf(txt, "End of track/macro\n");
								seqEnd = 1;
							}

							else
							{
								fprintf(txt, "Unknown command: %01X\n", command[0]);
								seqPos++;
							}
						}
					}

					else if (format == 2 || format == 3)
					{
						while (seqEnd == 0 && seqPos < bankSize)
						{
							command[0] = exRomData[seqPos];
							command[1] = exRomData[seqPos + 1];
							command[2] = exRomData[seqPos + 2];
							command[3] = exRomData[seqPos + 3];

							if (command[0] < 0x10)
							{
								highNibble = (command[0] & 15);
								curNoteLen = highNibble;
								fprintf(txt, "Rest, length: %i\n", curNoteLen);
								seqPos++;
							}

							else if (command[0] >= 0x10 && command[0] < 0xD0)
							{
								lowNibble = (command[0] >> 4);
								highNibble = (command[0] & 15);
								curNote = lowNibble;
								curNoteLen = highNibble;
								fprintf(txt, "Play note: %i, length: %i\n", curNote, curNoteLen);
								seqPos++;
							}

							else if (command[0] >= 0xD0 && command[0] < 0xE0 && curTrack == 3)
							{
								highNibble = (command[0] & 15);
								fprintf(txt, "Drum speed: %i, instrument: %i\n", highNibble, command[1]);
								seqPos += 2;
							}

							else if (command[0] >= 0xD0 && command[0] <= 0xD7)
							{
								highNibble = (command[0] & 15);
								octave = highNibble;
								fprintf(txt, "Set octave: %i\n", octave);
								seqPos++;
							}
							
							else if (command[0] == 0xD8)
							{
								lowNibble = (command[2] >> 4);
								highNibble = (command[2] & 15);
								fprintf(txt, "Note type, speed: %i, volume: %i, fade: %i\n", command[1], lowNibble, highNibble);
								seqPos += 3;
							}

							else if (command[0] == 0xD9)
							{
								lowNibble = (command[1] >> 4);
								highNibble = (command[1] & 15);
								transposeOct = lowNibble;
								transposeKey = highNibble;
								fprintf(txt, "Transpose, octave: %i, key: %i\n", transposeOct, transposeKey);
								seqPos += 2;
							}

							else if (command[0] == 0xDA)
							{
								tempo = ReadBE16(&exRomData[seqPos + 1]);
								fprintf(txt, "Set tempo: %i\n", tempo);
								seqPos += 3;
							}

							else if (command[0] == 0xDB)
							{
								fprintf(txt, "Duty: %i\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xDC)
							{
								lowNibble = (command[1] >> 4);
								highNibble = (command[1] & 15);
								fprintf(txt, "Volume envelope: %i, %i\n", lowNibble, highNibble);
								seqPos += 2;
							}

							else if (command[0] == 0xDD)
							{
								fprintf(txt, "Pitch sweep: %i\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xDE)
							{
								fprintf(txt, "Duty cycle pattern: %i\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xDF)
							{
								fprintf(txt, "Interpret SFX data as music\n");
								seqPos++;
							}

							else if (command[0] == 0xE0)
							{
								lowNibble = (command[2] >> 4);
								highNibble = (command[2] & 15);
								fprintf(txt, "Pitch slide, delay: %i, depth: %i, rate: %i\n", command[1], lowNibble, highNibble);
								seqPos += 3;
							}

							else if (command[0] == 0xE1)
							{
								lowNibble = (command[2] >> 4);
								highNibble = (command[2] & 15);
								fprintf(txt, "Vibrato: delay: %i, depth: %i, rate: %i\n", command[1], lowNibble, highNibble);
								seqPos += 3;
							}

							else if (command[0] == 0xE2)
							{
								fprintf(txt, "Command E2: %01X\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xE3)
							{
								fprintf(txt, "Toggle noise command: %01X\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xE4)
							{
								lowNibble = (command[1] >> 4);
								highNibble = (command[1] & 15);
								fprintf(txt, "Force stereo panning: %i, %i\n", lowNibble, highNibble);
								seqPos += 2;
							}

							else if (command[0] == 0xE5)
							{
								lowNibble = (command[1] >> 4);
								highNibble = (command[1] & 15);
								fprintf(txt, "Set master volume: %i, %i\n", lowNibble, highNibble);
								seqPos += 2;
							}

							else if (command[0] == 0xE6)
							{
								pitchOffset = ReadBE16(&exRomData[seqPos + 1]);
								fprintf(txt, "Set pitch offset: %i\n", pitchOffset);
								seqPos += 3;
							}

							else if (command[0] == 0xE7)
							{
								fprintf(txt, "Command E7: %01X\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xE8)
							{
								fprintf(txt, "Command E8: %01X\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xE9)
							{
								tempoRel = (signed int)command[1];
								fprintf(txt, "Change tempo (relative): %i\n", tempoRel);
								seqPos += 2;
							}

							else if (command[0] == 0xEA)
							{
								otherAddr = ReadLE16(&exRomData[seqPos + 1]);
								fprintf(txt, "Restart channel at position: 0x%04X\n", otherAddr);
								seqPos += 3;
							}

							else if (command[0] == 0xEB)
							{
								otherAddr = ReadLE16(&exRomData[seqPos + 1]);
								fprintf(txt, "New song address: 0x%04X\n", otherAddr);
								seqPos += 3;
							}

							else if (command[0] == 0xEC)
							{
								fprintf(txt, "Set song priority: on\n");
								seqPos++;
							}

							else if (command[0] == 0xED)
							{
								fprintf(txt, "Set song priority: off\n");
								seqPos++;
							}

							else if (command[0] == 0xEE)
							{
								otherAddr = ReadLE16(&exRomData[seqPos + 1]);
								fprintf(txt, "Command EE: 0x%04X\n", otherAddr);
								seqPos += 3;
							}

							else if (command[0] == 0xEF)
							{
								lowNibble = (command[1] >> 4);
								highNibble = (command[1] & 15);
								fprintf(txt, "Set pan: %i, %i\n", lowNibble, highNibble);
								seqPos += 2;
							}

							else if (command[0] == 0xF0)
							{
								fprintf(txt, "Toggle noise\n");
								seqPos++;
							}

							else if (command[0] >= 0xF1 && command[0] <= 0xF9)
							{
								fprintf(txt, "Commands F1-FA: %01X\n", command[0]);
								seqPos++;
							}

							else if (command[0] == 0xFA)
							{
								fprintf(txt, "Set condition command: %01X\n", command[1]);
								seqPos += 2;
							}

							else if (command[0] == 0xFB)
							{
								otherAddr = ReadLE16(&exRomData[seqPos + 2]);
								fprintf(txt, "Jump to address 0x%04X on condition: %01X\n", otherAddr, command[1]);
								seqPos += 3;
							}

							else if (command[0] == 0xFC)
							{
								otherAddr = ReadLE16(&exRomData[seqPos + 1]);
								fprintf(txt, "Jump to address: 0x%04X\n", otherAddr);
								seqPos += 3;
							}

							else if (command[0] == 0xFD)
							{
								macroPos = ReadLE16(&exRomData[seqPos + 2]);
								macroTimes = command[1];
								macroRet = seqPos + 4;
								if (macroTimes == 0)
								{
									fprintf(txt, "Go to macro infinite times (loop): 0x%04X\n\n", macroPos);
									seqEnd = 1;
								}
								else
								{
									fprintf(txt, "Go to macro %i times: 0x%04X\n", macroTimes, macroPos);
									seqPos += 4;
								}

							}

							else if (command[0] == 0xFE)
							{
								macroPos = ReadLE16(&exRomData[seqPos + 1]);
								macroRet = seqPos + 3;
								fprintf(txt, "Call macro: 0x%04X\n", macroPos);
								seqPos += 3;
							}

							else if (command[0] == 0xFF)
							{
								fprintf(txt, "End of track/macro\n");
								seqEnd = 1;
							}

							else
							{
								fprintf(txt, "Unknown command: %01X\n", command[0]);
								seqPos++;
							}
						}
					}

				}
			}
			fclose(txt);
		}

	}