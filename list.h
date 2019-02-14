#ifndef LIST_H
#define LIST_H

    typedef struct MonsterList MonsterList;
    struct MonsterList
    {
        Monster monster;
        MonsterList *next;
    };

    void initMonsterList(MonsterList *monsterList);
    void insertMonster(MonsterList *monsterList, MonsterList *monster);
    void removeMonsterFromIndex(MonsterList *monsterList, int index);
    void removeAllMonsters(MonsterList *monsterList);

#endif // LIST_H
