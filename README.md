# GF2MID
Game Freak (GB/GBC) to MIDI converter

This tool converts music from Game Boy and Game Boy Color games using Game Freak's sound engine to MIDI format. This sound engine is used almost exclusively in Pokémon games; however, it was first used in Mario & Yoshi (or simply Yoshi). There are two versions of the driver. Some unlicensed games from the company Sintax (e.g. Donkey Kong 5: The Journey of over Time and Space) use audio from the game Pokémon Pinball.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).
For games that contain multiple banks of music (most games), you must run the program multiple times specifying where each different bank is located.  However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed. However, for Pokémon: Gold, Silver, and Crystal Versions, this is not required, since there is only one pointer table referencing the songs and the banks they are stored in.

Note that for later games, there are "empty" tracks (usually the first or last track). This is normal.

Examples:
* GF2MID "Pokemon - Red Version (UE) [S][!].gb" 3
* GF2MID "Pokemon - Red Version (UE) [S][!].gb" 9
* GF2MID "Pokemon - Red Version (UE) [S][!].gb" 20
* GF2MID "Mario & Yoshi (E) [!].gb" 2
* GF2MID "Pokemon - Gold Version (UE) [C][!].gbc" 3B

This tool was based on the following disassemblies of Pokémon: Red and Gold versions:
https://github.com/pret/pokered
https://github.com/pret/pokegold

Also included is another program, GF2TXT, which prints out information about the song data from each game. This is essentially a prototype of GF2MID.
Supported games:
  * Mario & Yoshi
  * Pocket Monsters Midori (Pokémon: Green Version)
  * Pokémon: Blue Version
  * Pokémon: Crystal Version
  * Pokémon: Gold Version
  * Pokémon: Red Version
  * Pokémon: Silver Version
  * Pokémon: Yellow Version: Special Pikachu Edition
  * Pokémon Pinball

## To do:
  * Panning support
  * Support for the NES version of the sound engine (used in at least 2 games)
  * GBS file support
