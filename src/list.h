#ifndef LIST_H
#define LIST_H

    typedef struct MonsterList MonsterList;
    struct MonsterList
    {
        SDL_Rect pos;
        int is_boss;
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

    typedef struct BulletList BulletList;
    struct BulletList
    {
        SDL_Rect pos;
        int dirX;
        int player_num;

        BulletList *next;
    };

    void initBulletList(BulletList *bulletList);
    void insertBullet(BulletList *bulletList, BulletList *bullet);
    void removeBulletFromIndex(BulletList *bulletList, int index);
    void removeAllBullets(BulletList *bulletList);


#endif // LIST_H
