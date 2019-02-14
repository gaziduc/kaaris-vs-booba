#include <SDL2/SDL.h>
#include "game.h"
#include "list.h"


void initMonsterList(MonsterList *monsterList)
{
    monsterList->next = NULL;
}


void insertMonster(MonsterList *monsterList, MonsterList *monster)
{
    monster->next = monsterList->next;
    monsterList->next = monster;
}


void removeMonsterFromIndex(MonsterList *monsterList, int index)
{
    if(index == 0)
    {
        MonsterList *toDelete = monsterList->next;
        monsterList->next = monsterList->next->next;
        free(toDelete);
    }
    else
        removeMonsterFromIndex(monsterList->next, index - 1);
}


void removeAllMonsters(MonsterList *monsterList)
{
    if(monsterList->next != NULL)
        removeAllMonsters(monsterList->next);

    free(monsterList->next);
    monsterList->next = NULL;
}


