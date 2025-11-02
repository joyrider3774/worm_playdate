#ifndef SAVEDATA_H
#define SAVEDATA_H

#include "Worm.h"


typedef struct SaveData {
	int highScores[MaxGameModes*maxSeed];
} SaveData;

SaveData save;

void LoadSavedData()
{
	for (int i = 0; i < MaxGameModes * maxSeed; i++)
		save.highScores[i] = 0;
	SDFile *Fp;
	Fp = pd->file->open("worm.dat", kFileReadData);
	if (Fp)
	{
		pd->file->read(Fp,&save,sizeof(SaveData));
		pd->file->close(Fp);
	}
}

void SaveSavedData()
{
	SDFile *Fp;
	Fp = pd->file->open("worm.dat", kFileWrite);
	if (Fp)
	{
		pd->file->write(Fp,&save,sizeof(SaveData));
		pd->file->close(Fp);
	}
}

#endif