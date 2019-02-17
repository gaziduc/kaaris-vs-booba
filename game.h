#ifndef GAME_H
#define GAME_H

    #define WINDOW_W            1280.0
    #define WINDOW_H            720.0
    #define TILE_SIZE           32
    #define PLAYER_W            40
    #define PLAYER_H            68
    #define NUM_ANIM_TILE       8
    #define NUM_FRAME_ANIM      7
    #define GRAVITY_SPEED       0.8
    #define MAX_FALL_SPEED      10
    #define JUMP_HEIGHT         13
    #define DELAY_GAME          1000 / 60
    #define MAX_MOVING_PLAT     10
    #define NUM_LEVEL           4
    #define EXPLOSION_SIZE      64

    #define BUMPER_NUM          6
    #define LIFE_NUM            7
    #define MONEY_NUM           8
    #define MONSTER_NUM         9
    #define MOVING_PLAT_NUM     10
    #define MOVING_PLAT_END_NUM 11
    #define CHECKPOINT_NUM      13
    #define INVICIBLE_NUM       21
    #define TILE_PENTE_BenH_1   22
    #define TILE_PENTE_BenH_2   23
    #define TILE_PENTE_HenB_1   24
    #define TILE_PENTE_HenB_2   25

    #include <SDL2/SDL_mixer.h>
    #include "data.h"
    #include "event.h"

    typedef struct
    {
        SDL_Texture *texture;
        Mix_Chunk *sfx;
        int num_elm;
        int dirXmin;
        int dirXmax;
        int dirYmin;
        int dirYmax;
        SDL_Rect *pos;
        int *dirX;
        int *dirY;
    } Weather;

    typedef struct
    {
        int direction;
        int beginX, beginY;
        SDL_Rect pos;
        int endX, endY;
        int is_player_on;
    } MovingPlat;

    typedef struct
    {
        char name[100];
        int number;
        int width;
        int height;
        SDL_Texture *sky;
        SDL_Texture *tileset;
        Mix_Music *music;
        int **map;
        int startX, startY;
        int maxX, maxY;
        int num_tiles;
        char *solid;
        Weather *weather;
        int num_moving_plat;
        MovingPlat moving_plat[MAX_MOVING_PLAT];
    } Lvl;

    enum {IDLE_LEFT, IDLE_RIGHT, WALK_LEFT, WALK_RIGHT, JUMP_LEFT, JUMP_RIGHT, NUM_STATE};
    enum {LEFT, RIGHT, UP, DOWN};
    enum {PLAY, EDIT};


    typedef struct
    {
        SDL_Rect pos;
        int direction;
        int on_ground;
        float dirX, dirY;
        int saveX, saveY;
        int frame_explosion;
        int lifes;
    } Monster;

    #include "list.h"

    typedef struct
    {
        SDL_Texture *texture[NUM_STATE];
        SDL_Rect pos;
        int direction;
        int state;
        int frame;
        int on_ground;
        float dirX, dirY;
        int can_jump;
        int lifes;
        int money;
        MonsterList *monsterList;
        int killed;
        int dead;
        int frame_explosion;
        int respawnX, respawnY;
        int isCheckpointActive;
        int invicibleFramesLeft;
        float dirXmem, dirYmem;
        int posXmem, posYmem;
        int wasOnGround;
        int wasOnSlope;
    } Player;


    void loadLevel(SDL_Renderer *renderer, const int lvl_num, Lvl *lvl, int mode, Settings *settings);
    void freeLevel(Lvl *lvl, int mode);
    void freePlayer(Player *player);
    void respawn(Player *player);
    void displayGame(SDL_Renderer *renderer, Pictures *pictures, Lvl *lvl, Player *player, unsigned long frame_num, int mode);
    void displayHUD(SDL_Renderer *renderer, Player *player, Pictures *pictures, Fonts *fonts, int level_num);
    void displayPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, Pictures *pictures);
    void playGame(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Settings *settings);
    void wait(unsigned long *time1, unsigned long *time2, int delay);
    void mapCollisionPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, Sounds *sounds, Input *in, Pictures *pictures, Fonts *fonts, Settings *settings);
    void mapCollisionMonster(Lvl *lvl, Player *player, MonsterList *currentMonster, int monsterIndex);
    void centerScrollingOnPlayer(Lvl *lvl, Player *player);
    void gameOver(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in);
    void displayWeather(SDL_Renderer *renderer, Player *player, Weather *weather, unsigned long frame_num);
    void setWeatherElement(Weather *weather, int elm_num, int is_initted);
    void displayLevelName(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Lvl *lvl, Input *in);
    void createMonster(Player *player, const int x, const int y, const int lifes);
    void createMovingPlat(Lvl *lvl, int x, int y);
    void displayMonsters(SDL_Renderer *renderer, Lvl *lvl, Player *player, Pictures *pictures, unsigned long frame_num);
    void displayMovingPlat(SDL_Renderer *renderer, Lvl *lvl, unsigned long frame_num);
    void updateMovingPlat(Lvl *lvl, Player *player);
    void death(Player *player, Sounds *sounds, Input *in);
    int updateMonsters(Lvl *lvl, Player *player, Sounds *sounds, Input *in);
    int collide(Player *player, MonsterList *currentMonster);
    int checkFall(Lvl *lvl, Player *player, MonsterList *currentMonster);
    void displayScore(SDL_Renderer *renderer, Player *player, Input *in, Pictures *pictures, Fonts *fonts);
    void displayBossLife(SDL_Renderer *renderer, Player *player, Fonts *fonts);
    SDL_Point segment2segment(int Ax0, int Ay0, int Bx0, int By0, int Cx0, int Cy0, int Dx0, int Dy0);
    void getSlopeSegment(int tx, int ty, int pente, SDL_Point *s1, SDL_Point *s2);
    int slopeEquation(int pente, double *a, double *b);
    int checkSlope(Lvl *lvl, Player *player);

#endif // GAME_H
