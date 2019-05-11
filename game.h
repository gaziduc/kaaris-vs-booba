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
    #define NUM_LEVEL           6
    #define BOSS_1_LEVEL        4
    #define BOSS_2_LEVEL        6
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
    #include "net.h"
    #include "list.h"

    typedef struct
    {
        SDL_Texture *texture;
        Mix_Chunk *sfx;
        int num_elm;
        int dirXmin;
        int dirXmax;
        int dirYmin;
        int dirYmax;
        SDL_Rect *pos_dst;
        int *dirX;
        int *dirY;
        float *scale;
    } Weather;

    typedef struct
    {
        int direction;
        int beginX, beginY;
        SDL_Rect pos;
        int endX, endY;
        int is_player_on[2];
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
        int startX[2];
        int startY[2];
        int maxX, maxY;
        int num_tiles;
        char *solid;
        Weather *weather;
        int num_moving_plat;
        MovingPlat moving_plat[MAX_MOVING_PLAT];
        MonsterList *monsterList;
    } Lvl;

    enum {IDLE_LEFT, IDLE_RIGHT, WALK_LEFT, WALK_RIGHT, JUMP_LEFT, JUMP_RIGHT, NUM_STATE};
    enum {LEFT, RIGHT, UP, DOWN};
    enum {PLAY, EDIT};


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
        unsigned long timer;
    } Player;

    void map(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Mix_Music **music, Settings *settings, const int mode, Net *net);
    void loadLevel(SDL_Renderer *renderer, const int lvl_num, Lvl *lvl, int mode, Settings *settings);
    void freeLevel(Lvl *lvl, int mode);
    void freePlayer(Player *player);
    void respawn(Player *player);
    void displaySky(SDL_Renderer *renderer, Player *player, int num_player, int player_num, Lvl *lvl);
    void displayGame(SDL_Renderer *renderer, Pictures *pictures, Lvl *lvl, Player *player, const int player_num, unsigned long frame_num, int mode, const int num_players);
    void displayHUD(SDL_Renderer *renderer, Player *player, int player_num, Lvl *lvl, Pictures *pictures, Fonts *fonts, int level_num, int num_player);
    void displayPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, int player_num, int num_player, Pictures *pictures);
    void playGame(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Settings *settings, int level_num, const int mode, const int num_player, Net *net);
    void waitGame(unsigned long *time1, unsigned long *time2, int delay);
    int mapCollisionPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, const int player_num, Sounds *sounds, Input *in, Pictures *pictures, Fonts *fonts, Settings *settings, const int num_player, int mode);
    void mapCollisionMonster(Lvl *lvl, MonsterList *currentMonster, int monsterIndex);
    void centerScrollingOnPlayer(Lvl *lvl, Player *player, int player_num, int num_player);
    void gameOver(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in);
    void displayWeather(SDL_Renderer *renderer, Weather *weather, int player_num, int num_player);
    void setWeatherElement(Weather *weather, int elm_num, int is_initted);
    void displayLevelName(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Lvl *lvl, Input *in);
    void createMonster(Lvl *lvl, int x, int y, int lifes, int can_jump);
    void createMovingPlat(Lvl *lvl, int x, int y, const int num_player);
    void displayMonsters(SDL_Renderer *renderer, Lvl *lvl, Player *player, int player_num, int num_player, Pictures *pictures, unsigned long frame_num);
    void displayMovingPlat(SDL_Renderer *renderer, Lvl *lvl, unsigned long frame_num, int player_num, int num_player);
    void updateMovingPlat(Lvl *lvl, Player *players[], const int num_players);
    void death(Player *player, int player_num, Sounds *sounds, Input *in);
    int updateMonsters(Lvl *lvl, Player *player[], int num_player, Sounds *sounds, Input *in, Fonts *fonts, SDL_Renderer *renderer);
    int collide(Player *player, MonsterList *currentMonster);
    int checkFall(Lvl *lvl, MonsterList *currentMonster);
    void displayScore(SDL_Renderer *renderer, Player *player, Input *in, Pictures *pictures, Fonts *fonts);
    void displayBossLife(SDL_Renderer *renderer, Lvl *lvl, Fonts *fonts);
    SDL_Point segment2segment(int Ax0, int Ay0, int Bx0, int By0, int Cx0, int Cy0, int Dx0, int Dy0);
    void getSlopeSegment(int tx, int ty, int pente, SDL_Point *s1, SDL_Point *s2);
    int slopeEquation(int pente, double *a, double *b);
    int checkSlope(Lvl *lvl, Player *player);
    void initPlayer(SDL_Renderer *renderer, Player *player, int player_num);
    SDL_Texture* getScreenTexture(SDL_Renderer *renderer);
    void displayOtherPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, Packet packet);
    int receive_thread(void *data);

#endif // GAME_H
