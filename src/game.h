#ifndef GAME_H
#define GAME_H

    #define WINDOW_W            1280.0
    #define WINDOW_H            720.0
    #define TILE_SIZE           32
    #define PLAYER_W            32
    #define PLAYER_H            80
    #define NUM_ANIM_TILE       8
    #define NUM_FRAME_ANIM      7

    #define GRAVITY_SPEED       0.8
    #define MAX_FALL_SPEED      10
    #define JUMP_HEIGHT         13

    #define GRAVITY_SPEED_WATER 0.16
    #define MAX_FALL_SP_WATER   2
    #define JUMP_HEIGHT_WATER   3

    #define MAX_MOVING_PLAT     10
    #define NUM_LEVEL           7
    #define BOSS_1_LEVEL        4
    #define BOSS_2_LEVEL        7
    #define EXPLOSION_SIZE      64
    #define BULLET_SPEED        8
    #define PEAK_DEATH_OFFSET   15

    #define GUN_LABEL           "Lance-roquettes"
    #define JETPACK_LABEL       "Jetpack"
    #define CHECKPOINT_LABEL    "Point de contrôle"
    #define PERFUME_LABEL       "Flacon de parfum : invincibilité"

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
    #define CRATE_RPG_NUM       26
    #define PEAK_NUM            27
    #define CRATE_JETPACK_NUM   28
    #define TELEPORT_NUM        36

    #define SPAWN_X             70
    #define SPAWN_Y             250

    #include <SDL2/SDL_mixer.h>
    #include <SDL2/SDL2_framerate.h>
    #include "data.h"
    #include "event.h"
    #include "net.h"
    #include "list.h"

    typedef struct
    {
        SDL_Texture *texture;
        int num_elm;
        int dirXmin;
        int dirXmax;
        int dirYmin;
        int dirYmax;
        SDL_Rect *pos_dst[2];
        int *dirX[2];
        int *dirY[2];
        float *scale[2];
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
        SDL_Rect pos;
        int frame_num;
        SDL_Texture *texture;
    } Label;

    typedef struct
    {
        char name[100];
        int number;
        int width;
        int height;
        char sky_filename[100];
        SDL_Texture *sky;
        char tileset_filename[100];
        SDL_Texture *tileset;
        char music_filename[100];
        Mix_Music *music;
        int **map;
        int startX[2];
        int startY[2];
        int maxX, maxY;
        int num_tiles;
        char solid_filename[100];
        char *solid;
        char weather_filename[100];
        Weather *weather;
        int num_moving_plat;
        MovingPlat moving_plat[MAX_MOVING_PLAT];
        MonsterList *monsterList;
        int in_water;
        BulletList *bulletList;
    } Lvl;

    enum {IDLE_LEFT, IDLE_RIGHT, WALK_LEFT, WALK_RIGHT, JUMP_LEFT, JUMP_RIGHT, NUM_STATE};
    enum {LEFT, RIGHT, UP, DOWN};
    enum {PLAY, EDIT};
    enum {NO_POWER_UP, GUN, JETPACK};

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
        int power_up;
        Label label;
    } Player;

    void map(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Mix_Music **music, Settings *settings, const int num_player, Net *net, FPSmanager *fps);
    void loadLevel(SDL_Renderer *renderer, const int lvl_num, Lvl *lvl, int mode, int num_players);
    void freeLevel(Lvl *lvl, int mode, int num_players);
    void freePlayer(Player *player);
    void respawn(Player *player);
    void displaySky(SDL_Renderer *renderer, int num_player, int player_num, Lvl *lvl);
    void displayGame(SDL_Renderer *renderer, Pictures *pictures, Lvl *lvl, Player *player, const int player_num, unsigned long frame_num, int mode, const int num_players);
    void displayHUD(SDL_Renderer *renderer, Player *player, int player_num, Lvl *lvl, Pictures *pictures, Fonts *fonts, int level_num, int num_player, unsigned long frame_num, Settings *settings);
    void displayPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, int player_num, int num_player, Pictures *pictures, Net *net, Fonts *fonts);
    void playGame(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Settings *settings, int level_num, const int mode, const int num_player, Net *net, FPSmanager *fps);
    void waitGame(unsigned long *time1, unsigned long *time2, unsigned long delay);
    int mapCollisionPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, const int player_num, Sounds *sounds, Input *in, Fonts *fonts, Settings *settings);
    int mapCollisionMonster(Lvl *lvl, MonsterList *currentMonster);
    void centerScrollingOnPlayer(Lvl *lvl, Player *player, int player_num, int num_player);
    void gameOver(SDL_Renderer *renderer, Fonts *fonts, Input *in, FPSmanager *fps);
    void displayWeather(SDL_Renderer *renderer, Weather *weather, int player_num, int num_player);
    void setWeatherElement(Weather *weather, int player_num, int elm_num, int is_initted, int num_players);
    void displayLevelName(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Lvl *lvl, Input *in, FPSmanager *fps);
    void createMonster(Lvl *lvl, Pictures *pictures, int x, int y, int lifes);
    void createMovingPlat(Lvl *lvl, int x, int y, const int num_player);
    void displayMonsters(SDL_Renderer *renderer, Lvl *lvl, int player_num, int num_player, Pictures *pictures, unsigned long frame_num);
    void displayMovingPlat(SDL_Renderer *renderer, Lvl *lvl, unsigned long frame_num, int player_num, int num_player);
    void updateMovingPlat(Lvl *lvl, Player *players[], const int num_players);
    void death(Player *player, int player_num, Sounds *sounds, Input *in, Settings *settings);
    int updateMonsters(Lvl *lvl, Player *player[], int num_player, Sounds *sounds, Input *in, Settings *settings);
    int collide(Lvl *lvl, Player *player, MonsterList *currentMonster);
    int checkFall(Lvl *lvl, MonsterList *currentMonster);
    void displayScore(SDL_Renderer *renderer, Player *player, Input *in, Pictures *pictures, Fonts *fonts, FPSmanager *fps);
    void displayBossLife(SDL_Renderer *renderer, Lvl *lvl, Fonts *fonts);
    SDL_Point segment2segment(int Ax0, int Ay0, int Bx0, int By0, int Cx0, int Cy0, int Dx0, int Dy0);
    void getSlopeSegment(int tx, int ty, int pente, SDL_Point *s1, SDL_Point *s2);
    int slopeEquation(int pente, double *a, double *b);
    int checkSlope(Lvl *lvl, Player *player);
    void initPlayer(SDL_Renderer *renderer, Player *player);
    SDL_Texture* getScreenTexture(SDL_Renderer *renderer);
    void updateBullets(BulletList *bulletList, MonsterList *monsterList, Sounds *sounds, Lvl *lvl, Player *players[]);
    void createBullet(Player *player, int player_num, Lvl *lvl, Pictures *pictures);
    void displayBullets(SDL_Renderer *renderer, Lvl *lvl, int player_num, int num_player, Pictures *pictures);
    void displayOtherPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, Packet packet, Fonts *fonts);
    void setLabel(SDL_Renderer *renderer, Fonts *fonts, Lvl *lvl, Player *player, int player_num, char *str);
    void displayLabel(SDL_Renderer *renderer, Player *player);
    int receive_thread(void *data);
    void teleport(Lvl *lvl, Player *player, int x, int y);
    int getOffsetX(const int x, const int w, SDL_Rect *pos_dst);
    int getOffsetY(const int y, const int h, SDL_Rect *pos_dst, int num_players, int player_num);

#endif // GAME_H
