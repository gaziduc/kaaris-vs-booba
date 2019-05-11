#ifndef LIST_H
#define LIST_H

    typedef struct MonsterList MonsterList;
    struct MonsterList
    {
        SDL_Rect pos;
        int direction;
        int on_ground;
        float dirX, dirY;
        int saveX, saveY;
        int frame_explosion;
        int lifes;

        MonsterList *next;
    };

    void initMonsterList(MonsterList *monsterList);
    void insertMonster(MonsterList *monsterList, MonsterList *monster);
    void removeMonsterFromIndex(MonsterList *monsterList, int index);
    void removeAllMonsters(MonsterList *monsterList);

#endif // LIST_H
