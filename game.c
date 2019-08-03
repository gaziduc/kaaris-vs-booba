#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "event.h"
#include "game.h"
#include "transition.h"
#include "data.h"
#include "text.h"
#include "list.h"
#include "file.h"


Packet other_packet;
int receive;

void loadLevel(SDL_Renderer *renderer, const int lvl_num, Lvl *lvl, int mode, Settings *settings)
{
    char str[200] = "";
    FILE *file = NULL;

    sprintf(str, "data/maps/map_%d.txt", lvl_num);
    file = fopen(str, "r");
    if(file == NULL)
        exit(EXIT_FAILURE);

    lvl->number = lvl_num;

    lvl->weather = malloc(sizeof(Weather));
    if(lvl->weather == NULL)
        exit(EXIT_FAILURE);

    fscanf(file, "[METADATA]\n");
    fgets(str, sizeof(str) / sizeof(str[0]), file);

    int size = strlen("name=");
    memset(lvl->name, '\0', sizeof(lvl->name));
    for(int i = size; str[i] != '\r' && str[i] != '\n'; i++)
        lvl->name[i - size] = str[i];

    fscanf(file, "width=%d\nheight=%d\nsky=%s\ntileset=%s\ncollision=%s\nmusic=%s\nweather=%s\nweather_num_elm=%d\nweather_dir_x=[%d,%d]\nweather_dir_y=[%d,%d]\nin_water=%d\n\n[LEVEL]\n", &lvl->width, &lvl->height, lvl->sky_filename, lvl->tileset_filename, lvl->solid_filename, lvl->music_filename, lvl->weather_filename, &lvl->weather->num_elm, &lvl->weather->dirXmin, &lvl->weather->dirXmax, &lvl->weather->dirYmin, &lvl->weather->dirYmax, &lvl->in_water);

    lvl->sky = IMG_LoadTexture(renderer, lvl->sky_filename);
    if(lvl->sky == NULL)
        exit(EXIT_FAILURE);

    lvl->tileset = IMG_LoadTexture(renderer, lvl->tileset_filename);
    if(lvl->tileset == NULL)
        exit(EXIT_FAILURE);

    lvl->weather->texture = IMG_LoadTexture(renderer, lvl->weather_filename);
    if(lvl->weather->texture == NULL)
        exit(EXIT_FAILURE);

    lvl->map = malloc(sizeof(int*) * lvl->width);
    if(lvl->map == NULL)
        exit(EXIT_FAILURE);

    for(int x = 0; x < lvl->width; x++)
    {
        lvl->map[x] = malloc(sizeof(int) * lvl->height);
        if(lvl->map[x] == NULL)
            exit(EXIT_FAILURE);
    }

    for(int y = 0; y < lvl->height; y++)
    {
        char line[lvl->width + 3];
        fgets(line, lvl->width + 3, file); // + 3 => for \r eventually, \n and \0

        for(int x = 0; x < lvl->width; x++)
            lvl->map[x][y] = line[x] - '0';
    }

    fclose(file);

    for(int i = 0; i < 2; i++)
    {
        lvl->startX[i] = 0;
        lvl->startY[i] = 0;
    }

    lvl->maxX = lvl->width * TILE_SIZE;
    lvl->maxY = lvl->height * TILE_SIZE;


    file = fopen(lvl->solid_filename, "r");
    if(file == NULL)
        exit(EXIT_FAILURE);

    fscanf(file, "num_tiles=%d\n", &lvl->num_tiles);
    lvl->solid = malloc(sizeof(char) * lvl->num_tiles);
    if(lvl->solid == NULL)
        exit(EXIT_FAILURE);

    for(int i = 0; i < lvl->num_tiles; i++)
    {
        fscanf(file, "%c", &lvl->solid[i]);
        lvl->solid[i] -= '0';
    }

    fclose(file);

    lvl->weather->pos_dst = malloc(lvl->weather->num_elm * sizeof(SDL_Rect));
    if(lvl->weather->pos_dst == NULL)
        exit(EXIT_FAILURE);

    lvl->weather->dirX = malloc(lvl->weather->num_elm * sizeof(int));
    if(lvl->weather->dirX == NULL)
        exit(EXIT_FAILURE);

    lvl->weather->dirY = malloc(lvl->weather->num_elm * sizeof(int));
    if(lvl->weather->dirY == NULL)
        exit(EXIT_FAILURE);

    lvl->weather->scale = malloc(lvl->weather->num_elm * sizeof(float));
    if(lvl->weather->scale == NULL)
        exit(EXIT_FAILURE);

    lvl->num_moving_plat = 0;

    if(mode == PLAY)
    {
        lvl->monsterList = malloc(sizeof(MonsterList));
        initMonsterList(lvl->monsterList);

        lvl->bulletList = malloc(sizeof(BulletList));
        initBulletList(lvl->bulletList);

        lvl->music = Mix_LoadMUS(lvl->music_filename);
        if(lvl->music == NULL)
            exit(EXIT_FAILURE);

        Mix_PlayMusic(lvl->music, -1);

        for(int i = 0; i < lvl->weather->num_elm; i++)
            setWeatherElement(lvl->weather, i, 0);
    }
}


void setWeatherElement(Weather *weather, int elm_num, int is_initted)
{
    weather->pos_dst[elm_num].x = rand() % (int) WINDOW_W;
    SDL_QueryTexture(weather->texture, NULL, NULL, &weather->pos_dst[elm_num].w, &weather->pos_dst[elm_num].h);

    weather->scale[elm_num] = (float) rand() / RAND_MAX;

    weather->pos_dst[elm_num].w *= weather->scale[elm_num];
    if(weather->pos_dst[elm_num].w <= 0)
        weather->pos_dst[elm_num].w = 1;
    weather->pos_dst[elm_num].h *= weather->scale[elm_num];
    if(weather->pos_dst[elm_num].h <= 0)
        weather->pos_dst[elm_num].h = 1;

    if(is_initted)
        weather->pos_dst[elm_num].y = -weather->pos_dst[elm_num].h;
    else
        weather->pos_dst[elm_num].y = rand() % (int) WINDOW_H;

    weather->dirX[elm_num] = (rand() % (weather->dirXmax - weather->dirXmin + 1)) + weather->dirXmin;
    weather->dirY[elm_num] = (rand() % (weather->dirYmax - weather->dirYmin + 1)) + weather->dirYmin;
}

void freeLevel(Lvl *lvl, int mode)
{
    for(int x = 0; x < lvl->width; x++)
        free(lvl->map[x]);

    free(lvl->map);

    SDL_DestroyTexture(lvl->sky);
    SDL_DestroyTexture(lvl->tileset);

    free(lvl->solid);

    if(mode == PLAY)
    {
        removeAllMonsters(lvl->monsterList);
        free(lvl->monsterList);

        removeAllBullets(lvl->bulletList);
        free(lvl->bulletList);

        Mix_HaltMusic();
        Mix_FreeMusic(lvl->music);
    }


    free(lvl->weather->pos_dst);
    free(lvl->weather->dirX);
    free(lvl->weather->dirY);
    free(lvl->weather->scale);
    SDL_DestroyTexture(lvl->weather->texture);

    free(lvl->weather);
}


void displayGame(SDL_Renderer *renderer, Pictures *pictures, Lvl *lvl, Player *player, const int player_num, unsigned long frame_num, int mode, const int num_players)
{
    int x, y, mapX, x1, x2, mapY, y1, y2;

    mapX = lvl->startX[player_num] / TILE_SIZE;
    x1 = (lvl->startX[player_num] % TILE_SIZE) * -1;
    x2 = x1 + WINDOW_W + (x1 == 0 ? 0 : TILE_SIZE);

    mapY = lvl->startY[player_num] / TILE_SIZE;
    y1 = (lvl->startY[player_num] % TILE_SIZE) * -1 + (WINDOW_H / num_players) * player_num;
    y2 = y1 + WINDOW_H / num_players + (y1 == 0 ? 0 : TILE_SIZE);

    for (y = y1; y < y2; y += TILE_SIZE)
    {
        mapX = lvl->startX[player_num] / TILE_SIZE;

        for (x = x1; x < x2; x += TILE_SIZE)
        {
            if(lvl->map[mapX][mapY] == MONSTER_NUM && mode == PLAY)
            {
                 createMonster(lvl, pictures, mapX * TILE_SIZE, mapY * TILE_SIZE, (lvl->number == BOSS_1_LEVEL || lvl->number == BOSS_2_LEVEL) ? 10 : 1, 0);
                 lvl->map[mapX][mapY] = 0;
            }
            else if(lvl->map[mapX][mapY] == MOVING_PLAT_NUM && mode == PLAY)
            {
                createMovingPlat(lvl, mapX * TILE_SIZE, mapY * TILE_SIZE, num_players);
                lvl->map[mapX][mapY] = 0;
            }

            SDL_Rect pos_dst;

            int offsetX = 0;
            if(x < 0)
            {
                pos_dst.x = 0;
                offsetX = -x;
            }
            else
                pos_dst.x = x;

            int offsetY = 0;
            if(y < (WINDOW_H / num_players) * player_num)
            {
                pos_dst.y = (WINDOW_H / num_players) * player_num;
                offsetY = (WINDOW_H / num_players) * player_num - y;
            }
            else
                pos_dst.y = y;

            pos_dst.w = TILE_SIZE - offsetX;
            pos_dst.h = TILE_SIZE - offsetY;

            SDL_Rect pos_src;
            pos_src.w = TILE_SIZE - offsetX;
            pos_src.h = TILE_SIZE - offsetY;
            pos_src.x = lvl->map[mapX][mapY] * TILE_SIZE + offsetX;
            pos_src.y = ((frame_num % (NUM_ANIM_TILE * NUM_FRAME_ANIM)) / NUM_FRAME_ANIM) * TILE_SIZE + offsetY;

            if(mode == PLAY && lvl->map[mapX][mapY] == CHECKPOINT_NUM && player->isCheckpointActive)
                pos_src.x += TILE_SIZE;

            SDL_RenderCopy(renderer, lvl->tileset, &pos_src, &pos_dst);


            mapX++;
        }

        mapY++;
    }
}


void initPlayer(SDL_Renderer *renderer, Player *player)
{
    player->texture[IDLE_LEFT] = IMG_LoadTexture(renderer, "data/gfx/idleleft.png");
    player->texture[IDLE_RIGHT] = IMG_LoadTexture(renderer, "data/gfx/idleright.png");
    player->texture[WALK_LEFT] = IMG_LoadTexture(renderer, "data/gfx/walkleft.png");
    player->texture[WALK_RIGHT] = IMG_LoadTexture(renderer, "data/gfx/walkright.png");
    player->texture[JUMP_LEFT] = IMG_LoadTexture(renderer, "data/gfx/jumpleft.png");
    player->texture[JUMP_RIGHT] = IMG_LoadTexture(renderer, "data/gfx/jumpright.png");
    player->direction = RIGHT;
    player->state = IDLE_RIGHT;
    player->frame = 0;
    player->pos.w = PLAYER_W;
    player->pos.h = PLAYER_H;
    player->lifes = 5;
    player->money = 0;
    player->killed = 0;
    player->isCheckpointActive = 0;
    respawn(player);
    player->dead = 0;
    player->frame_explosion = 0;
    player->label.frame_num = -1;
}

void respawn(Player *player)
{
    player->dirX = 0;
    player->dirY = 0;

    if(player->isCheckpointActive)
    {
        player->pos.x = player->respawnX;
        player->pos.y = player->respawnY;
    }
    else
    {
        player->pos.x = 70;
        player->pos.y = 250;
    }

    player->on_ground = 0;
    player->can_jump = 1;
    player->invicibleFramesLeft = 0;
    player->power_up = NO_POWER_UP;
}



void displayLevelName(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Lvl *lvl, Input *in, FPSmanager *fps)
{
    SDL_Texture *texture[2];
    SDL_Rect pos_dst[2];
    SDL_Color white = {255, 255, 255};
    char str[100] = "";
    int escape = 0;

    sprintf(str, "Niveau %d", lvl->number);
    texture[0] = RenderTextBlended(renderer, fonts->preview_title, str, white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, lvl->name, white);

    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = WINDOW_H / 2 - pos_dst[0].h / 2 - 50;

    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = WINDOW_H / 2 - pos_dst[1].h / 2 + 50;

    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE || KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);
        SDL_RenderCopy(renderer, texture[0], NULL, &pos_dst[0]);
        SDL_RenderCopy(renderer, texture[1], NULL, &pos_dst[1]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);
    }

    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0, fps);

    SDL_DestroyTexture(texture[0]);
    SDL_DestroyTexture(texture[1]);
}


void map(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Mix_Music **music, Settings *settings, const int num_player, Net *net, FPSmanager *fps)
{
    SDL_Color white = {255, 255, 255};
    int escape = 0, selected = 1, goToCenter = 0, modified = 0;
    unsigned long frame_num = 0;
    char str[100] = "";
    SDL_Texture *texture[8];
    SDL_Rect pos_dst[8];
    int visible[2];
    unsigned long times[NUM_TIMES];
    loadTimes(times);

    texture[0] = RenderTextBlended(renderer, fonts->preview_title, "Niveau 1", white);
    texture[6] = RenderTextBlended(renderer, fonts->preview_title, "<", white);
    texture[7] = RenderTextBlended(renderer, fonts->preview_title, ">", white);

    visible[0] = 0;
    visible[1] = 1;

    int j = (selected - 1) * 5;

    for(int i = 1; i < 6; i++)
    {
        if(times[j + i - 1] == 0)
            sprintf(str, "%d) --------", i);
        else
            sprintf(str, "%d) %lu.%lu%lu%lu s", i, times[j + i - 1] / 1000, (times[j + i - 1] % 1000 < 100) ? 0 : (times[j + i - 1] % 1000) / 100, (times[j + i - 1] % 100 < 10) ? 0 : (times[j + i - 1] % 100) / 10, (times[j + i - 1] % 10 == 0) ? 0 : times[j + i - 1] % 10);
        texture[i] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    }

    for(int i = 0; i < 8; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

        if(i == 0 || i == 6 || i == 7)
            pos_dst[i].y = WINDOW_H / 2 - pos_dst[i].h / 2 - 170;
        else
            pos_dst[i].y = 310 + (i - 1) * 50;
    }

    pos_dst[6].x -= pos_dst[0].w / 2 + 60;
    pos_dst[7].x += pos_dst[0].w / 2 + 60;

    transition(renderer, pictures->title, 6, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
        {
            if(net != NULL)
            {
                ChoosePacket packet;
                packet.level_num = -1;
                SDLNet_TCP_Send(net->client, &packet, sizeof(ChoosePacket));
            }

            exit(EXIT_SUCCESS);
        }
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }
        if(KEY_LEFT_MENU)
        {
            in->key[SDL_SCANCODE_LEFT] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[0] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[0] = 0;

            if(selected > 1)
            {
                selected--;
                modified = 1;
                Mix_PlayChannel(-1, sounds->select, 0);
            }

        }
        if(KEY_RIGHT_MENU)
        {
            in->key[SDL_SCANCODE_RIGHT] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[0] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[0] = 0;

            if(selected < NUM_LEVEL)
            {
                selected++;
                modified = 1;
                Mix_PlayChannel(-1, sounds->select, 0);
            }

        }
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;

            if(net != NULL)
            {
                ChoosePacket packet;
                packet.level_num = selected;
                SDLNet_TCP_Send(net->client, &packet, sizeof(ChoosePacket));
            }

            Mix_PlayChannel(-1, sounds->enter, 0);

            transition(renderer, pictures->title, 6, texture, pos_dst, ENTERING, 0, fps);

            Mix_HaltMusic();
            Mix_FreeMusic(*music);

            playGame(renderer, in, pictures, fonts, sounds, settings, selected, ONE_LEVEL, num_player, net, fps);

            *music = Mix_LoadMUS("data/musics/menu.mp3");
            Mix_PlayMusic(*music, -1);

            loadTimes(times);
            modified = 1;

            transition(renderer, pictures->title, 6, texture, pos_dst, EXITING, 1, fps);
        }


        if(modified)
        {
            if(selected == 1)
                visible[0] = 0;
            else
                visible[0] = 1;

            if(selected == NUM_LEVEL)
                visible[1] = 0;
            else
                visible[1] = 1;

            int j = (selected - 1) * 5;

            for(int i = 0; i < 6; i++)
                SDL_DestroyTexture(texture[i]);

            sprintf(str, "Niveau %d", selected);
            texture[0] = RenderTextBlended(renderer, fonts->preview_title, str, white);

            for(int i = 1; i < 6; i++)
            {
                if(times[j + i - 1] == 0)
                    sprintf(str, "%d) --------", i);
                else
                    sprintf(str, "%d) %lu.%lu%lu%lu s", i, times[j + i - 1] / 1000, (times[j + i - 1] % 1000 < 100) ? 0 : (times[j + i - 1] % 1000) / 100, (times[j + i - 1] % 100 < 10) ? 0 : (times[j + i - 1] % 100) / 10, (times[j + i - 1] % 10 == 0) ? 0 : times[j + i - 1] % 10);
                texture[i] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
            }

            for(int i = 0; i < 8; i++)
            {
                SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
                pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

                if(i == 0 || i == 6 || i == 7)
                    pos_dst[i].y = WINDOW_H / 2 - pos_dst[i].h / 2 - 170;
                else
                    pos_dst[i].y = 310 + (i - 1) * 50;
            }

            pos_dst[6].x -= pos_dst[0].w / 2 + 60;
            pos_dst[7].x += pos_dst[0].w / 2 + 60;

            modified = 0;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        frame_num++;
        if(frame_num % 30 == 0)
            goToCenter = !goToCenter;

        if(goToCenter)
        {
            pos_dst[6].x++;
            pos_dst[7].x--;
        }
        else
        {
            pos_dst[6].x--;
            pos_dst[7].x++;
        }

        for(int i = 0; i < 8; i++)
            if(i < 6 || visible[i - 6])
                SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);
    }

    transition(renderer, pictures->title, 6, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 8; i++)
        SDL_DestroyTexture(texture[i]);
}


void playGame(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Settings *settings, int level_num, const int mode, const int num_player, Net *net, FPSmanager *fps)
{
    Lvl *lvl = malloc(sizeof(Lvl));
    if(lvl == NULL)
        exit(EXIT_FAILURE);

    loadLevel(renderer, level_num, lvl, PLAY, settings);
    unsigned long frame_num = 0;
    int escape = 0, finished = 0;
    int lvl_finished[num_player];
    int has_updated_scores[num_player];
    for(int i = 0; i < num_player; i++)
    {
         lvl_finished[i] = 0;
         has_updated_scores[i] = 0;
    }


    Player *player[num_player];
    for(int i = 0; i < num_player; i++)
    {
        player[i] = malloc(sizeof(Player));
        initPlayer(renderer, player[i]);
        player[i]->timer = 0;
    }

    if(net == NULL)
        displayLevelName(renderer, pictures, fonts, lvl, in, fps);
    else
    {
        receive = 1;
        SDL_CreateThread(receive_thread, "receive_thread", net);
    }


    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE || KEY_PAUSE)
        {
            in->key[SDL_SCANCODE_P] = 0;
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[0].buttons[7] = 0;
            in->controller[1].buttons[6] = 0;
            in->controller[1].buttons[7] = 0;

            SDL_Texture *texture = getScreenTexture(renderer);

            if(net != NULL)
                receive = 0;

            if(pauseGame(renderer, texture, pictures, fonts, in, sounds, fps) == 2)
                escape = 1;

            if(net != NULL && !escape)
            {
                receive = 1;
                SDL_CreateThread(receive_thread, "receive_thread", net);
            }

            SDL_DestroyTexture(texture);
        }


        for(int i = 0; i < num_player; i++)
        {
            if(lvl_finished[i] && !has_updated_scores[i])
            {
                updateTimes(level_num, player[i]->timer);
                has_updated_scores[i] = 1;
            }
        }


        if(lvl_finished[0] && (num_player < 2 || lvl_finished[1]) && (KEY_ENTER_MENU))
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;

            for(int i = 0; i < num_player; i++)
            {
                lvl_finished[i] = 0;
                has_updated_scores[i] = 0;
            }

            if(mode == ALL_LEVELS)
            {
                if(lvl->number < NUM_LEVEL)
                {
                    level_num++;
                    freeLevel(lvl, PLAY);
                    loadLevel(renderer, level_num, lvl, PLAY, settings);

                    for(int i = 0; i < num_player; i++)
                    {
                        player[i]->isCheckpointActive = 0;
                        player[i]->timer = 0;
                        respawn(player[i]);
                    }

                    if(net == NULL)
                        displayLevelName(renderer, pictures, fonts, lvl, in, fps);
                }
                else
                {
                    escape = 1;
                    finished = 1;
                }
            }
            else
                escape = 1;
        }

        updateMovingPlat(lvl, player, num_player);

        if(updateMonsters(lvl, player, num_player, sounds, in, fonts, renderer, settings)) // If the boss was killed
        {
            Mix_PlayChannel(-1, sounds->complete, 0);
            for(int i = 0; i < num_player; i++)
                 lvl_finished[i] = 1;
        }

        updateBullets(lvl->bulletList, lvl->monsterList, sounds, lvl, player);

        for(int i = 0; i < num_player; i++)
        {
            if(!lvl_finished[i])
            {
                if(KEY_LEFT_GAME)
                {
                    player[i]->direction = LEFT;
                    player[i]->dirX = -4;

                    if(player[i]->state != WALK_LEFT && player[i]->on_ground)
                    {
                        player[i]->state = WALK_LEFT;
                        player[i]->frame = 0;
                    }
                }
                if(KEY_RIGHT_GAME)
                {
                    player[i]->direction = RIGHT;
                    player[i]->dirX = 4;

                    if(player[i]->state != WALK_RIGHT && player[i]->on_ground)
                    {
                        player[i]->state = WALK_RIGHT;
                        player[i]->frame = 0;
                    }
                }
                if(KEY_UP_GAME)
                {
                    if(lvl->in_water)
                    {
                        player[i]->dirY = -JUMP_HEIGHT_WATER;
                        Mix_PlayChannel(-1, sounds->jump, 0);
                    }
                    else if(player[i]->on_ground)
                    {
                        player[i]->dirY = -JUMP_HEIGHT;
                        player[i]->on_ground = 0;
                        player[i]->can_jump = 1;
                        Mix_PlayChannel(-1, sounds->jump, 0);
                    }
                    else if(player[i]->can_jump)
                    {
                        player[i]->dirY = -JUMP_HEIGHT;
                        player[i]->can_jump = 0;
                        Mix_PlayChannel(-1, sounds->jump, 0);
                    }

                    in->key[settings->controls[i].jump] = 0;
                    if(num_player == 1)
                        in->key[settings->controls[i + 1].jump] = 0;
                    in->controller[i].buttons[0] = 0;
                }
                if(KEY_POWER_UP_GAME)
                {
                    if(player[i]->power_up == GUN)
                    {
                        if(!in->repeat || in->controller[i].buttons[2])
                        {
                            in->key[settings->controls[i].power_up] = 0;
                            if(num_player == 1)
                                in->key[settings->controls[i + 1].power_up] = 0;
                            in->controller[i].buttons[2] = 0;

                            createBullet(player[i], i, lvl, pictures);
                            Mix_PlayChannel(-1, sounds->gun, 0);
                        }
                    }
                    else if(player[i]->power_up == JETPACK)
                    {
                        player[i]->dirY -= 2 * GRAVITY_SPEED;
                        if(player[i]->dirY < -MAX_FALL_SPEED / 2)
                            player[i]->dirY = -MAX_FALL_SPEED / 2;
                    }
                }
            }


            if(!player[i]->on_ground)
            {
                if(player[i]->direction == RIGHT && player[i]->state != JUMP_RIGHT)
                {
                    player[i]->state = JUMP_RIGHT;
                    player[i]->frame = 0;
                }
                if(player[i]->direction == LEFT && player[i]->state != JUMP_LEFT)
                {
                    player[i]->state = JUMP_LEFT;
                    player[i]->frame = 0;
                }
            }
            else if(!(KEY_LEFT_GAME) && !(KEY_RIGHT_GAME))
            {
                if(player[i]->state == JUMP_LEFT || player[i]->state == WALK_LEFT || player[i]->state == IDLE_LEFT)
                    player[i]->state = IDLE_LEFT;
                else
                    player[i]->state = IDLE_RIGHT;
            }

            if(!player[i]->dead)
                if(mapCollisionPlayer(renderer, lvl, player[i], i, sounds, in, pictures, fonts, settings, num_player, mode))
                {
                    Mix_PlayChannel(-1, sounds->complete, 0);
                    lvl_finished[i] = 1;
                }
        }

        SDL_RenderClear(renderer); // put this line after mapCollisionPlayer(...), because of the animation of the end of a level.

        for(int i = 0; i < num_player; i++)
        {
            centerScrollingOnPlayer(lvl, player[i], i, num_player);
            displaySky(renderer, player[i], num_player, i, lvl);
            displayWeather(renderer, lvl->weather, i, num_player);
            displayGame(renderer, pictures, lvl, player[i], i, frame_num, PLAY, num_player);
            displayMonsters(renderer, lvl, player[i], i, num_player, pictures, frame_num);
            displayMovingPlat(renderer, lvl, frame_num, i, num_player);
            displayBullets(renderer, lvl, player[i], i, num_player, pictures);
            displayHUD(renderer, player[i], i, lvl, pictures, fonts, lvl->number, num_player, frame_num, settings);
            displayLabel(renderer, player[i]);
        }

        for(int i = 0; i < num_player; i++)
            displayPlayer(renderer, lvl, player[i], i, num_player, pictures);

        if(net != NULL)
        {
            Packet packet;
            packet.point.x = player[0]->pos.x;
            packet.point.y = player[0]->pos.y;
            packet.state = player[0]->state;
            packet.frame = player[0]->frame;
            sendPos(net, &packet);

            displayOtherPlayer(renderer, lvl, player[0], other_packet);
        }

        for(int i = 0; i < num_player; i++)
        {
            if(lvl_finished[i])
                levelFinished(renderer, fonts, player[i], level_num, num_player, i, lvl_finished);
        }


        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;

        for(int i = 0; i < num_player; i++)
        {
            player[i]->frame++;

            int w;
            SDL_QueryTexture(player[i]->texture[player[i]->state], NULL, NULL, &w, NULL);
            if(player[i]->frame >= w / PLAYER_W)
                player[i]->frame = 0;

            player[i]->dirX = 0;

            if(lvl->in_water)
            {
                player[i]->dirY += GRAVITY_SPEED_WATER;
                if(player[i]->dirY >= MAX_FALL_SP_WATER)
                    player[i]->dirY = MAX_FALL_SP_WATER;
            }
            else
            {
                player[i]->dirY += GRAVITY_SPEED;
                if(player[i]->dirY >= MAX_FALL_SPEED)
                    player[i]->dirY = MAX_FALL_SPEED;
            }

            if(!lvl_finished[i])
                player[i]->timer += APPR_DELAY_GAME;
        }

        for(int i = 0; i < num_player; i++)
        {
            if(num_player < 2 && player[i]->lifes <= 0 && !player[i]->dead)
            {
                receive = 0;
                gameOver(renderer, pictures, fonts, in, fps);
                escape = 1;
            }

            if(player[i]->invicibleFramesLeft > 0)
                player[i]->invicibleFramesLeft--;
        }
    }


    freeLevel(lvl, PLAY);
    free(lvl);

    if(finished && num_player < 2)
        displayScore(renderer, player[0], in, pictures, fonts, fps);

    for(int i = 0; i < num_player; i++)
        freePlayer(player[i]);

    if(net != NULL)
        receive = 0;
}


void displayOtherPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, Packet packet)
{
    SDL_Rect pos_dst;
    pos_dst.x = packet.point.x - lvl->startX[0];
    pos_dst.y = packet.point.y - lvl->startY[0];
    pos_dst.w = PLAYER_W;
    pos_dst.h = PLAYER_H;

    SDL_Rect pos_src;
    pos_src.x = (packet.frame / 2) * PLAYER_W;
    pos_src.y = 0;
    pos_src.w = PLAYER_W;
    pos_src.h = PLAYER_H;

    SDL_SetTextureBlendMode(player->texture[packet.state], SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(player->texture[packet.state], 192);
    SDL_RenderCopy(renderer, player->texture[packet.state], &pos_src, &pos_dst);
    SDL_SetTextureAlphaMod(player->texture[packet.state], 255);
}

void displaySky(SDL_Renderer *renderer, Player *player, int num_player, int player_num, Lvl *lvl)
{
    SDL_Rect pos_src[2];
    SDL_Rect pos_dst[2];

    pos_src[0].x = (lvl->startX[player_num] % (int) (WINDOW_W * 4)) / 4;
    pos_src[0].w = WINDOW_W - pos_src[0].x;
    pos_dst[0].x = 0;
    pos_dst[0].w = pos_src[0].w;

    pos_src[1].x = 0;
    pos_src[1].w = WINDOW_W - pos_src[0].w;
    pos_dst[1].x = pos_src[0].w;
    pos_dst[1].w = pos_src[0].x;

    for(int i = 0; i < 2; i++)
    {
        pos_src[i].y = num_player == 1 ? 0 : WINDOW_H / 4;
        pos_src[i].h = WINDOW_H / num_player;
        pos_dst[i].y = (WINDOW_H / num_player) * player_num;
        pos_dst[i].h = WINDOW_H / num_player;

        SDL_RenderCopy(renderer, lvl->sky, &pos_src[i], &pos_dst[i]);
    }
}



void displayScore(SDL_Renderer *renderer, Player *player, Input *in, Pictures *pictures, Fonts *fonts, FPSmanager *fps)
{
    unsigned long score = player->lifes * 1000 + player->killed * 100 + player->money * 10;
    int escape = 0;
    SDL_Texture *texture[5];
    SDL_Rect pos_dst[5];
    SDL_Color white = {255, 255, 255};
    char str[200] = "";

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Score :", white);
    sprintf(str, "%d vies x 1000 = %d", player->lifes, player->lifes * 1000);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    sprintf(str, "+ %d mini-Booba tués x 100 = %d", player->killed, player->killed * 100);
    texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    sprintf(str, "+ %d pièces x 10 = %d", player->money, player->money * 10);
    texture[3] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    sprintf(str, "= %ld", score);
    texture[4] = RenderTextBlended(renderer, fonts->ocraext_score, str, white);

    for(int i = 0; i < 5; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - 220;
        pos_dst[i].y = 160 + (i * 80);
    }


    transition(renderer, pictures->title, 5, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ENTER_MENU || KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < 5; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);
    }

    transition(renderer, pictures->title, 5, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 5; i++)
        SDL_DestroyTexture(texture[i]);



    unsigned long scores[NUM_SCORES];
    char names[NUM_SCORES][NAME_LEN];
    loadScores(scores, names);

    memset(str, '\0', sizeof(str));
    enterName(renderer, fonts, pictures, in, str, fps);

    int i = NUM_SCORES - 1;
    while(i >= 0 && score > scores[i])
    {
        if(i == NUM_SCORES - 1)
        {
            scores[i] = score;
            strcpy(names[i], str);
        }
        else
        {
            unsigned long temp = scores[i];
            scores[i] = score;
            scores[i + 1] = temp;

            char strTemp[NAME_LEN];
            memset(strTemp, '\0', sizeof(strTemp));
            strcpy(strTemp, names[i]);
            strcpy(names[i], str);
            strcpy(names[i + 1], strTemp);
        }

        i--;
    }



    saveScores(scores, names);
    displayScoreList(renderer, pictures, fonts, in, scores, names, fps);
}


void displayWeather(SDL_Renderer *renderer, Weather *weather, int player_num, int num_player)
{
    for(int i = 0; i < weather->num_elm; i++)
    {
        if(weather->pos_dst[i].y >= WINDOW_H || weather->pos_dst[i].x >= WINDOW_W || weather->pos_dst[i].x + weather->pos_dst[i].w < 0)
            setWeatherElement(weather, i, 1);
        else if(weather->pos_dst[i].y + weather->pos_dst[i].h >= (WINDOW_H / num_player) * player_num && weather->pos_dst[i].y < (WINDOW_H / num_player) * (player_num + 1))
        {
            weather->pos_dst[i].x += weather->dirX[i];
            weather->pos_dst[i].y += weather->dirY[i];
        }

        if(weather->pos_dst[i].y + weather->pos_dst[i].h >= (WINDOW_H / num_player) * player_num && weather->pos_dst[i].y < (WINDOW_H / num_player) * (player_num + 1))
        {
            SDL_Rect pos_src;
            pos_src.x = 0;

            int w, h;
            SDL_QueryTexture(weather->texture, NULL, NULL, &w, &h);

            if(weather->pos_dst[i].y + weather->pos_dst[i].h >= (WINDOW_H / num_player) * player_num && weather->pos_dst[i].y < (WINDOW_H / num_player) * player_num)
            {
                pos_src.y = weather->pos_dst[i].y + weather->pos_dst[i].h - (WINDOW_H / num_player) * player_num;
                pos_src.h = h - pos_src.y;

                weather->pos_dst[i].y = (WINDOW_H / num_player) * player_num;
                weather->pos_dst[i].h = pos_src.h * weather->scale[i];
            }
            else
            {
                pos_src.y = 0;
                pos_src.h = h;
            }

            pos_src.w = w;

            SDL_RenderCopy(renderer, weather->texture, &pos_src, &weather->pos_dst[i]);

            weather->pos_dst[i].h = h * weather->scale[i];
        }
    }
}




void gameOver(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, FPSmanager *fps)
{
    SDL_Color white = {255, 255, 255};
    SDL_Texture *texture[2];

    texture[0] = RenderTextBlended(renderer, fonts->preview_title, "GAME OVER", white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, "Appuyez sur ENTREE", white);

    SDL_Texture *screen = getScreenTexture(renderer);

    SDL_Rect pos_dst[2];
    unsigned long frame = 0;
    int escape = 0;

    for(int i = 0; i < 2; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = WINDOW_H / 2 - 50 - pos_dst[i].h / 2 + 140 * i;
    }


    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller[0].buttons[0] = 0;
            in->controller[1].buttons[0] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen, NULL, NULL);

        roundedBoxRGBA(renderer, WINDOW_W / 6, WINDOW_H / 4, (WINDOW_W / 6) * 5, (WINDOW_H / 4) * 3, 10, 64, 64, 64, 192);
        roundedRectangleRGBA(renderer, WINDOW_W / 6, WINDOW_H / 4, (WINDOW_W / 6) * 5, (WINDOW_H / 4) * 3, 10, 255, 255, 255, 192);

        SDL_RenderCopy(renderer, texture[0], NULL, &pos_dst[0]);

        if(frame % 60 < 30)
            SDL_RenderCopy(renderer, texture[1], NULL, &pos_dst[1]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame++;
    }

    for(int i = 0; i < 2; i++)
        SDL_DestroyTexture(texture[i]);

    SDL_DestroyTexture(screen);
}


void freePlayer(Player *player)
{
    for(int i = 0; i < NUM_STATE; i++)
        SDL_DestroyTexture(player->texture[i]);

    free(player);
}


void displayHUD(SDL_Renderer *renderer, Player *player, int player_num, Lvl *lvl, Pictures *pictures, Fonts *fonts, int level_num, int num_player, unsigned long frame_num, Settings *settings)
{
    SDL_Color color = {0, 255, 128};
    SDL_Texture *texture = NULL;
    char str[100] = "";
    SDL_Rect pos_dst;
    pos_dst.x = 10;
    pos_dst.y = 10 + (WINDOW_H / 2) * player_num;

    if(num_player < 2)
    {
        SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_dst.w, &pos_dst.h);
        SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_dst);

        sprintf(str, "x %d", player->lifes);
        texture = RenderTextBlended(renderer, fonts->ocraext_message, str, color);
        pos_dst.x += pos_dst.w + 10;
        pos_dst.y += pos_dst.h / 2;
        SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
        pos_dst.y -= pos_dst.h / 2;
        SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
        SDL_DestroyTexture(texture);

        pos_dst.x += pos_dst.w + 20;
        pos_dst.y = 10 + (WINDOW_H / 2) * player_num;
    }

    SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_dst.w, &pos_dst.h);
    SDL_RenderCopy(renderer, pictures->HUDcoin, NULL, &pos_dst);

    sprintf(str, "x %d", player->money);
    texture = RenderTextBlended(renderer, fonts->ocraext_message, str, color);
    pos_dst.x += pos_dst.w + 10;
    pos_dst.y += pos_dst.h / 2;
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.y -= pos_dst.h / 2;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);

    pos_dst.x += pos_dst.w + 20;
    pos_dst.y = 10 + (WINDOW_H / 2) * player_num;
    SDL_QueryTexture(pictures->HUDtimer, NULL, NULL, &pos_dst.w, &pos_dst.h);
    SDL_RenderCopy(renderer, pictures->HUDtimer, NULL, &pos_dst);

    sprintf(str, "%lu.%lu", player->timer / 1000, (player->timer % 1000 < 100) ? 0 : (player->timer % 1000) / 100);
    texture = RenderTextBlended(renderer, fonts->ocraext_message, str, color);
    pos_dst.x += pos_dst.w + 10;
    pos_dst.y += pos_dst.h / 2;
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.y -= pos_dst.h / 2;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);

    pos_dst.x += pos_dst.w + 30;
    pos_dst.y = 10 + (WINDOW_H / 2) * player_num;
    pos_dst.w = TILE_SIZE;
    pos_dst.h = TILE_SIZE;


    if(player->power_up != NO_POWER_UP)
    {
        SDL_Rect pos_src;
        if(player->power_up == GUN)
            pos_src.x = CRATE_RPG_NUM * TILE_SIZE;
        else if(player->power_up == JETPACK)
            pos_src.x = CRATE_JETPACK_NUM * TILE_SIZE;
        pos_src.y = ((frame_num % (NUM_ANIM_TILE * NUM_FRAME_ANIM)) / NUM_FRAME_ANIM) * TILE_SIZE;
        pos_src.w = TILE_SIZE;
        pos_src.h = TILE_SIZE;

        SDL_RenderCopy(renderer, lvl->tileset, &pos_src, &pos_dst);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &pos_dst);

        char str[100] = "";

        if(player->power_up == GUN)
            sprintf(str, "Tirer : %s%s%s", SDL_GetScancodeName(settings->controls[player_num].power_up), num_player < 2 ? " / " : "", num_player < 2 ? SDL_GetScancodeName(settings->controls[player_num + 1].power_up) : "");
        else if(player->power_up == JETPACK)
            sprintf(str, "Voler : Maintenir %s%s%s", SDL_GetScancodeName(settings->controls[player_num].power_up), num_player < 2 ? " / " : "", num_player < 2 ? SDL_GetScancodeName(settings->controls[player_num + 1].power_up) : "");

        texture = RenderTextBlended(renderer, fonts->preview_intro, str, color);
        pos_dst.x += pos_dst.w + 10;
        pos_dst.y += pos_dst.h / 2;
        SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
        pos_dst.y -= pos_dst.h / 2;
        SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
        SDL_DestroyTexture(texture);
    }



    sprintf(str, "Mini-Booba tués : %d", player->killed);
    texture = RenderTextBlended(renderer, fonts->ocraext_message, str, color);
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W - pos_dst.w - 20;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);

    if(level_num == BOSS_1_LEVEL || level_num == BOSS_2_LEVEL)
        displayBossLife(renderer, lvl, fonts);
}


void displayPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, int player_num, int num_player, Pictures *pictures)
{
    SDL_Rect pos_src;
    SDL_Rect pos_dst;

    for(int i = 0; i < num_player; i++)
    {
        if(!player->dead)
        {
            if(player->invicibleFramesLeft < 0 || player->invicibleFramesLeft % 16 < 8)
            {
                pos_src.x = player->frame * PLAYER_W;
                pos_src.y = 0;
                pos_src.w = PLAYER_W;
                pos_src.h = PLAYER_H;

                pos_dst.x = player->pos.x - lvl->startX[i];
                pos_dst.y = player->pos.y - lvl->startY[i] + (WINDOW_H / 2) * i;
                pos_dst.w = player->pos.w;
                pos_dst.h = player->pos.h;

                if(pos_dst.x + PLAYER_W >= 0 && pos_dst.x < WINDOW_W && pos_dst.y + PLAYER_H >= (WINDOW_H / num_player) * i && pos_dst.y < (WINDOW_H / num_player) * (i + 1))
                {
                    if(player_num != i)
                    {
                        SDL_SetTextureBlendMode(player->texture[player->state], SDL_BLENDMODE_BLEND);
                        SDL_SetTextureAlphaMod(player->texture[player->state], 192);
                    }

                    SDL_RenderCopy(renderer, player->texture[player->state], &pos_src, &pos_dst);

                    if(player_num != i)
                        SDL_SetTextureAlphaMod(player->texture[player->state], 255);
                }
            }
        }
        else
        {
            pos_src.x = (player->frame_explosion / 8) * EXPLOSION_SIZE;
            pos_src.y = 0;
            pos_src.w = EXPLOSION_SIZE;
            pos_src.h = EXPLOSION_SIZE;

            pos_dst.x = player->pos.x + TILE_SIZE / 2 - EXPLOSION_SIZE / 2 - lvl->startX[i];
            pos_dst.y = player->pos.y + TILE_SIZE / 2 - EXPLOSION_SIZE / 2 - lvl->startY[i] + (WINDOW_H / 2) * i;
            pos_dst.w = EXPLOSION_SIZE;
            pos_dst.h = EXPLOSION_SIZE;

            if(pos_dst.y + EXPLOSION_SIZE >= (WINDOW_H / num_player) * (player_num + 1))
            {
                pos_dst.h = (WINDOW_H / num_player) * (player_num + 1) - pos_dst.y;
                pos_src.h = pos_dst.h;
            }

            if(pos_dst.x + EXPLOSION_SIZE >= 0 && pos_dst.x < WINDOW_W && pos_dst.y + EXPLOSION_SIZE >= (WINDOW_H / num_player) * i && pos_dst.y < (WINDOW_H / num_player) * (i + 1))
                SDL_RenderCopy(renderer, pictures->explosion, &pos_src, &pos_dst);

            if(i == 0)
            {
                player->frame_explosion++;
                if(player->frame_explosion >= 56)
                {
                    player->dead = 0;
                    respawn(player);
                }
            }
        }
    }


}



void displayMovingPlat(SDL_Renderer *renderer, Lvl *lvl, unsigned long frame_num, int player_num, int num_player)
{
    for(int i = 0; i < lvl->num_moving_plat; i++)
    {
        SDL_Rect pos_src;
        pos_src.x = MOVING_PLAT_NUM * TILE_SIZE;
        pos_src.y = ((frame_num % (NUM_ANIM_TILE * NUM_FRAME_ANIM)) / NUM_FRAME_ANIM) * TILE_SIZE;
        pos_src.w = TILE_SIZE;
        pos_src.h = TILE_SIZE;

        SDL_Rect pos_dst;
        pos_dst.x = lvl->moving_plat[i].pos.x - lvl->startX[player_num];
        pos_dst.y = lvl->moving_plat[i].pos.y - lvl->startY[player_num] + (WINDOW_H / 2) * player_num;
        pos_dst.w = TILE_SIZE;
        pos_dst.h = TILE_SIZE;

        if(pos_dst.x + TILE_SIZE >= 0 && pos_dst.x < WINDOW_W && pos_dst.y + TILE_SIZE >= (WINDOW_H / num_player) * player_num && pos_dst.y < (WINDOW_H / num_player) * (player_num + 1))
            SDL_RenderCopy(renderer, lvl->tileset, &pos_src, &pos_dst);
    }
}



void displayMonsters(SDL_Renderer *renderer, Lvl *lvl, Player *player, int player_num, int num_player, Pictures *pictures, unsigned long frame_num)
{
    MonsterList *current = lvl->monsterList->next;

    while(current != NULL)
    {
        SDL_Rect pos_src;
        SDL_Rect pos_dst;

        if(current->lifes > 0)
        {
            if(!current->is_boss)
            {
                pos_src.x = MONSTER_NUM * TILE_SIZE;
                pos_src.y = ((frame_num % (NUM_ANIM_TILE * NUM_FRAME_ANIM)) / NUM_FRAME_ANIM) * TILE_SIZE;
                pos_src.w = TILE_SIZE;
                pos_src.h = TILE_SIZE;

                pos_dst.x = current->pos.x - lvl->startX[player_num];
                pos_dst.y = (current->pos.y - lvl->startY[player_num]) + (WINDOW_H / 2) * player_num;
                pos_dst.w = TILE_SIZE;
                pos_dst.h = TILE_SIZE;

                if(pos_dst.x + TILE_SIZE >= 0 && pos_dst.x < WINDOW_W && pos_dst.y + TILE_SIZE >= (WINDOW_H / num_player) * player_num && pos_dst.y < (WINDOW_H / num_player) * (player_num + 1))
                    SDL_RenderCopy(renderer, lvl->tileset, &pos_src, &pos_dst);
            }
            else
            {
                pos_dst.x = current->pos.x - lvl->startX[player_num];
                pos_dst.y = (current->pos.y - lvl->startY[player_num]) + (WINDOW_H / 2) * player_num;
                SDL_QueryTexture(pictures->boss, NULL, NULL, &pos_dst.w, &pos_dst.h);

                if(pos_dst.x + pos_dst.w >= 0 && pos_dst.x < WINDOW_W && pos_dst.y + pos_dst.h >= (WINDOW_H / num_player) * player_num && pos_dst.y < (WINDOW_H / num_player) * (player_num + 1))
                    SDL_RenderCopy(renderer, pictures->boss, NULL, &pos_dst);
            }
        }

        if(current->frame_explosion >= 0)
        {
            pos_src.x = (current->frame_explosion / 8) * EXPLOSION_SIZE;
            pos_src.y = 0;
            pos_src.w = EXPLOSION_SIZE;
            pos_src.h = EXPLOSION_SIZE;

            pos_dst.x = current->pos.x + current->pos.w / 2 - EXPLOSION_SIZE / 2 - lvl->startX[player_num];
            pos_dst.y = current->pos.y + TILE_SIZE / 2 - EXPLOSION_SIZE / 2 - lvl->startY[player_num] + (WINDOW_H / 2) * player_num;
            pos_dst.w = EXPLOSION_SIZE;
            pos_dst.h = EXPLOSION_SIZE;

            if(pos_dst.x + EXPLOSION_SIZE >= 0 && pos_dst.x < WINDOW_W && pos_dst.y + EXPLOSION_SIZE >= (WINDOW_H / num_player) * player_num && pos_dst.y < (WINDOW_H / num_player) * (player_num + 1))
                SDL_RenderCopy(renderer, pictures->explosion, &pos_src, &pos_dst);
        }

        current = current->next;
    }
}


void updateMovingPlat(Lvl *lvl, Player *player[], const int num_players)
{
    for(int i = 0; i < lvl->num_moving_plat; i++)
    {
        if(lvl->moving_plat[i].direction == RIGHT)
        {
            lvl->moving_plat[i].pos.x += 2;

            for(int j = 0; j < num_players; j++)
            {
                if(lvl->moving_plat[i].is_player_on[j])
                    player[j]->dirX += 2;
            }
        }
        else
        {
            lvl->moving_plat[i].pos.x -= 2;

            for(int j = 0; j < num_players; j++)
            {
                 if(lvl->moving_plat[i].is_player_on[j])
                    player[j]->dirX -= 2;
            }
        }


        if(lvl->moving_plat[i].pos.x == lvl->moving_plat[i].endX)
            lvl->moving_plat[i].direction = LEFT;
        else if(lvl->moving_plat[i].pos.x == lvl->moving_plat[i].beginX)
            lvl->moving_plat[i].direction = RIGHT;
    }
}



int updateMonsters(Lvl *lvl, Player *player[], int num_player, Sounds *sounds, Input *in, Fonts *fonts, SDL_Renderer *renderer, Settings *settings)
{
    int index = 0;
    MonsterList *current = lvl->monsterList->next;

    while(current != NULL)
    {
        current->dirX = 0;

        if(!lvl->in_water)
            current->dirY += GRAVITY_SPEED;

        if(current->frame_explosion >= 0)
        {
            current->frame_explosion++;
            if(current->frame_explosion >= 56)
            {
                current->frame_explosion = -1;
                if(current->lifes <= 0)
                {
                    current = current->next;
                    removeMonsterFromIndex(lvl->monsterList, index);

                    if(lvl->number == BOSS_1_LEVEL || lvl->number == BOSS_2_LEVEL)
                        return 1;

                    continue;
                }
            }
        }


        if(current->lifes > 0)
        {
            if(lvl->in_water)
            {
                if(current->dirY >= MAX_FALL_SP_WATER)
                    current->dirY = MAX_FALL_SP_WATER;
            }
            else
            {
                if(current->dirY >= MAX_FALL_SPEED)
                    current->dirY = MAX_FALL_SPEED;
            }


            if(current->pos.x == current->saveX || (!lvl->in_water && checkFall(lvl, current)))
            {
                if(current->direction == LEFT)
                    current->direction = RIGHT;
                else
                    current->direction = LEFT;
            }

            if(current->direction == LEFT)
                current->dirX -= (lvl->number == BOSS_2_LEVEL) ? 14 : (lvl->number == BOSS_1_LEVEL) ? 8 : 2;
            else
                current->dirX += (lvl->number == BOSS_2_LEVEL) ? 14 : (lvl->number == BOSS_1_LEVEL) ? 8 : 2;


            current->saveX = current->pos.x;

            mapCollisionMonster(lvl, current, index);

            for(int i = 0; i < num_player; i++)
            {
                if(!player[i]->dead)
                {
                    int collision = collide(lvl, player[i], current);

                    if(collision == 1 || (collision == 2 && player[i]->invicibleFramesLeft > 0))
                    {
                        if(lvl->number != BOSS_1_LEVEL && lvl->number != BOSS_2_LEVEL)
                            player[i]->killed++;

                        if(player[i]->invicibleFramesLeft == 0)
                        {
                            if(lvl->in_water)
                                player[i]->dirY = -JUMP_HEIGHT_WATER;
                            else
                            {
                                player[i]->dirY = -JUMP_HEIGHT;
                                player[i]->can_jump = 1;
                            }
                        }

                        current->lifes--;
                        current->frame_explosion = 0;
                        Mix_PlayChannel(-1, sounds->explosion, 0);
                    }
                    else if(collision == 2)
                        death(player[i], i, sounds, in, settings);
                }
            }
        }

        current = current->next;
        index++;
    }


    return 0;
}


int collide(Lvl *lvl, Player *player, MonsterList *currentMonster)
{
    if(player->pos.x >= currentMonster->pos.x + currentMonster->pos.w || player->pos.x + player->pos.w <= currentMonster->pos.x || player->pos.y >= currentMonster->pos.y + currentMonster->pos.h || player->pos.y + player->pos.h <= currentMonster->pos.y)
        return 0;

    if(player->pos.y + player->pos.h <= currentMonster->pos.y + (lvl->in_water ? MAX_FALL_SP_WATER : MAX_FALL_SPEED))
        return 1;

    return 2;
}


void displayBossLife(SDL_Renderer *renderer, Lvl *lvl, Fonts *fonts)
{
    SDL_Color color = {255, 0, 0};
    SDL_Rect pos_dst;

    SDL_Texture *texture = RenderTextBlended(renderer, fonts->ocraext_message, "Booba ", color);
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = 18;
    pos_dst.y = 70;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    pos_dst.x += pos_dst.w + 10;
    pos_dst.y += pos_dst.h / 2;
    SDL_DestroyTexture(texture);

    if(lvl->monsterList->next != NULL && lvl->monsterList->next->lifes > 0)
    {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, lvl->monsterList->next->lifes * 114, 10);
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, NULL);
        SDL_SetRenderTarget(renderer, NULL);

        SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
        SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
        SDL_DestroyTexture(texture);
    }
}


int checkFall(Lvl *lvl, MonsterList *currentMonster)
 {
    int x, y;

    if (currentMonster->direction == LEFT)
    {
        x = (int) (currentMonster->pos.x + currentMonster->dirX) / TILE_SIZE;
        y = (int) (currentMonster->pos.y + currentMonster->pos.h - 1) /  TILE_SIZE;
        if (y < 0)
            y = 1;
        if (y >= lvl->height)
            y = lvl->height;
        if (x < 0)
            x = 1;
        if (x > lvl->width)
            x = lvl->width;

        if (x < lvl->width && y < lvl->height - 1 && !lvl->solid[lvl->map[x][y + 1]])
            return 1;
        else
            return 0;
    }
    else
    {
        x = (int)(currentMonster->pos.x + currentMonster->pos.w + currentMonster->dirX) / TILE_SIZE;
        y = (int)(currentMonster->pos.y + currentMonster->pos.h - 1) / TILE_SIZE;
        if (y <= 0)
            y = 1;
        if (y >= lvl->height)
            y = lvl->height - 1;
        if (x <= 0)
            x = 1;
        if (x >= lvl->width)
            x = lvl->width - 1;

        if (x < lvl->width && y < lvl->height - 1 && !lvl->solid[lvl->map[x][y + 1]])
            return 1;
        else
            return 0;
    }
}






void waitGame(unsigned long *time1, unsigned long *time2, int delay)
{
    *time2 = SDL_GetTicks();
    if(*time2 - *time1 < delay)
        SDL_Delay(delay - (*time2 - *time1));
    *time1 = SDL_GetTicks();
}


void centerScrollingOnPlayer(Lvl *lvl, Player *player, int player_num, int num_player)
{
    int tempX = player->pos.x - (WINDOW_W / 2) + player->pos.w / 2;

    if(tempX > lvl->startX[player_num])
    {
        int difference = tempX - lvl->startX[player_num];

        if(difference < 25)
            lvl->startX[player_num] += difference;
        else
            lvl->startX[player_num] += 25;
    }
    else if(tempX < lvl->startX[player_num])
    {
        int difference = lvl->startX[player_num] - tempX;

        if(difference < 25)
            lvl->startX[player_num] -= difference;
        else
            lvl->startX[player_num] -= 25;
    }

    if(lvl->startX[player_num] < 0)
        lvl->startX[player_num] = 0;
    if(lvl->startX[player_num] + WINDOW_W >= lvl->maxX)
        lvl->startX[player_num] = lvl->maxX - WINDOW_W;

    int tempY = player->pos.y - WINDOW_H / (2 * num_player) + player->pos.h / 2;

    if(tempY > lvl->startY[player_num])
    {
        int difference = tempY - lvl->startY[player_num];

        if(difference < 25)
            lvl->startY[player_num] += difference;
        else
            lvl->startY[player_num] += 25;
    }
    else if(tempY < lvl->startY[player_num])
    {
        int difference = lvl->startY[player_num] - tempY;

        if(difference < 25)
            lvl->startY[player_num] -= difference;
        else
            lvl->startY[player_num] -= 25;
    }

    if(lvl->startY[player_num] < 0)
        lvl->startY[player_num] = 0;
    if(lvl->startY[player_num] + WINDOW_H / num_player >= lvl->maxY)
        lvl->startY[player_num] = lvl->maxY - WINDOW_H / num_player;
 }


int mapCollisionPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, const int player_num, Sounds *sounds, Input *in, Pictures *pictures, Fonts *fonts, Settings *settings, const int num_player, int mode)
{
    player->dirXmem = player->dirX;
    player->wasOnGround = player->on_ground;
    player->dirYmem = player->dirY;
    player->posXmem = player->pos.x;
    player->posYmem = player->pos.y;

    int i, x1, x2, y1, y2;

    player->on_ground = 0;

    if(player->pos.h > TILE_SIZE)
        i = TILE_SIZE;
    else
        i = player->pos.h;

    for (;;)
    {
        x1 = (player->pos.x + player->dirX) / TILE_SIZE;
        x2 = (player->pos.x + player->dirX + player->pos.w - 1) / TILE_SIZE;

        y1 = (player->pos.y) / TILE_SIZE;
        y2 = (player->pos.y + i - 1) / TILE_SIZE;


        if (x1 >= 0 && x2 < lvl->width && y1 >= 0 && y2 < lvl->height)
        {
            if (player->dirX > 0)
            {
                if(lvl->map[x2][y1] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x2][y1] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x2][y2] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x2][y2] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x2][y1] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x2][y1] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x2][y2] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x2][y2] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x2][y1] == INVICIBLE_NUM || lvl->map[x2][y2] == INVICIBLE_NUM)
                {
                    if(player->invicibleFramesLeft <= 0)
                    {
                        player->invicibleFramesLeft = 360;
                        Mix_PlayChannel(-1, sounds->invicible, 0);
                        setLabel(renderer, fonts, lvl, player, player_num, PERFUME_LABEL);
                    }
                }

                if(lvl->map[x2][y1] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x2][y2] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x2][y1] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x2][y2] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x2][y1] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x2][y2] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x2][y1] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x2, y1);
                }

                if(lvl->map[x2][y2] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x2, y2);
                }

                if(lvl->solid[lvl->map[x2][y1]] || lvl->solid[lvl->map[x2][y2]])
                {
                    player->pos.x = x2 * TILE_SIZE;
                    player->pos.x -= player->pos.w + 1;
                    player->dirX = 0;
                }
            }

            else if (player->dirX < 0)
            {
                if(lvl->map[x1][y1] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x1][y1] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x1][y2] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x1][y2] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x1][y1] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x1][y1] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x1][y2] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x1][y2] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x1][y1] == INVICIBLE_NUM || lvl->map[x1][y2] == INVICIBLE_NUM)
                {
                    if(player->invicibleFramesLeft <= 0)
                    {
                        player->invicibleFramesLeft = 360;
                        Mix_PlayChannel(-1, sounds->invicible, 0);
                        setLabel(renderer, fonts, lvl, player, player_num, PERFUME_LABEL);
                    }
                }

                if(lvl->map[x1][y1] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x1][y2] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x1][y1] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x1][y2] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x1][y1] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x1][y2] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x1][y1] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x1, y1);
                }

                if(lvl->map[x1][y2] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x1, y2);
                }

                if (lvl->solid[lvl->map[x1][y1]] || lvl->solid[lvl->map[x1][y2]])
                {
                    player->pos.x = (x1 + 1) * TILE_SIZE;
                    player->dirX = 0;
                }
            }
        }

        if(i == player->pos.h)
            break;

        i += TILE_SIZE;

        if(i > player->pos.h)
            i = player->pos.h;
    }


    if(player->pos.w > TILE_SIZE)
        i = TILE_SIZE;
    else
        i = player->pos.w;


    for (;;)
    {
        x1 = (player->pos.x) / TILE_SIZE;
        x2 = (player->pos.x + i) / TILE_SIZE;

        y1 = (player->pos.y + player->dirY) / TILE_SIZE;
        y2 = (player->pos.y + player->dirY + player->pos.h) / TILE_SIZE;

        if (x1 >= 0 && x2 < lvl->width && y1 >= 0 && y2 < lvl->height)
        {
            if(player->dirY > 0)
            {
                if(lvl->map[x1][y2] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x1][y2] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x2][y2] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x2][y2] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x1][y2] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x1][y2] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x2][y2] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x2][y2] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x1][y2] == INVICIBLE_NUM || lvl->map[x2][y2] == INVICIBLE_NUM)
                {
                    if(player->invicibleFramesLeft <= 0)
                    {
                        player->invicibleFramesLeft = 360;
                        Mix_PlayChannel(-1, sounds->invicible, 0);
                        setLabel(renderer, fonts, lvl, player, player_num, PERFUME_LABEL);
                    }
                }

                if(lvl->map[x1][y2] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x2][y2] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x1][y2] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x2][y2] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x1][y2] == PEAK_NUM)
                {
                    if(player->invicibleFramesLeft <= 0 && player->pos.x + player->pos.w > x1 * TILE_SIZE + PEAK_DEATH_OFFSET && player->pos.x < (x1 + 1) * TILE_SIZE - PEAK_DEATH_OFFSET)
                        death(player, player_num, sounds, in, settings);
                }
                else if(lvl->map[x2][y2] == PEAK_NUM)
                {
                    if(player->invicibleFramesLeft <= 0 && player->pos.x + player->pos.w > x2 * TILE_SIZE + PEAK_DEATH_OFFSET && player->pos.x < (x2 + 1) * TILE_SIZE - PEAK_DEATH_OFFSET)
                        death(player, player_num, sounds, in, settings);
                }

                if(lvl->map[x1][y2] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x2][y2] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x1][y2] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x1, y2);
                }

                if(lvl->map[x2][y2] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x2, y2);
                }


                if(lvl->map[x1][y2] == BUMPER_NUM || lvl->map[x2][y2] == BUMPER_NUM)
                {
                    Mix_PlayChannel(-1, sounds->bumper, 0);
                    player->dirY = -20;
                    player->on_ground = 1;
                    player->can_jump = 1;
                }
                else if(lvl->solid[lvl->map[x1][y2]] || lvl->solid[lvl->map[x2][y2]])
                {
                    player->pos.y = y2 * TILE_SIZE;
                    player->pos.y -= player->pos.h;
                    player->dirY = 0;
                    player->on_ground = 1;
                    player->can_jump = 1;
                }

                for(int j = 0; j < lvl->num_moving_plat; j++)
                {
                    if(player->pos.x + player->pos.w >= lvl->moving_plat[j].pos.x
                    && player->pos.x <= lvl->moving_plat[j].pos.x + lvl->moving_plat[j].pos.w
                    && player->pos.y + player->pos.h >= lvl->moving_plat[j].pos.y
                    && player->pos.y + player->pos.h < lvl->moving_plat[j].pos.y + lvl->moving_plat[j].pos.h)
                    {
                        player->pos.y = lvl->moving_plat[j].pos.y - player->pos.h;
                        player->dirY = 0;
                        player->on_ground = 1;
                        player->can_jump = 1;
                        lvl->moving_plat[j].is_player_on[player_num] = 1;
                    }
                    else
                        lvl->moving_plat[j].is_player_on[player_num] = 0;
                }
            }

            else if (player->dirY < 0)
            {
                if(lvl->map[x1][y1] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x1][y1] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x2][y1] == LIFE_NUM)
                {
                    player->lifes++;
                    lvl->map[x2][y1] = 0;
                    Mix_PlayChannel(-1, sounds->life, 0);
                }

                if(lvl->map[x1][y1] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x1][y1] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x2][y1] == MONEY_NUM)
                {
                    player->money++;
                    lvl->map[x2][y1] = 0;
                    Mix_PlayChannel(-1, sounds->coin, 0);
                }

                if(lvl->map[x1][y1] == INVICIBLE_NUM || lvl->map[x2][y1] == INVICIBLE_NUM)
                {
                    if(player->invicibleFramesLeft <= 0)
                    {
                        player->invicibleFramesLeft = 360;
                        Mix_PlayChannel(-1, sounds->invicible, 0);
                        setLabel(renderer, fonts, lvl, player, player_num, PERFUME_LABEL);
                    }
                }

                if(lvl->map[x1][y1] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x2][y1] == CRATE_RPG_NUM && player->power_up != GUN)
                {
                    player->power_up = GUN;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, GUN_LABEL);
                }

                if(lvl->map[x1][y1] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x2][y1] == CRATE_JETPACK_NUM && player->power_up != JETPACK)
                {
                    player->power_up = JETPACK;
                    Mix_PlayChannel(-1, sounds->life, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, JETPACK_LABEL);
                }

                if(lvl->map[x1][y1] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x2][y1] == CHECKPOINT_NUM && !player->isCheckpointActive)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                    setLabel(renderer, fonts, lvl, player, player_num, CHECKPOINT_LABEL);
                }

                if(lvl->map[x1][y1] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x1, y1);
                }

                if(lvl->map[x2][y1] == TELEPORT_NUM)
                {
                    teleport(lvl, player, x2, y1);
                }

                if (lvl->solid[lvl->map[x1][y1]] || lvl->solid[lvl->map[x2][y1]])
                {
                    player->pos.y = (y1 + 1) * TILE_SIZE;
                    player->dirY = 0;
                }
            }
        }

        if(i == player->pos.w)
            break;

        i += TILE_SIZE;

        if(i > player->pos.w)
            i = player->pos.w;
    }


    checkSlope(lvl, player);

    player->pos.x += player->dirX;
    player->pos.y += player->dirY;


    if(player->pos.x < 0)
        player->pos.x = 0;

    if(player->pos.y > lvl->maxY)
        death(player, player_num, sounds, in, settings);

    if(player->pos.x + player->pos.w >= lvl->maxX)
    {
        player->pos.x = lvl->maxX - player->pos.w - 1;

        if(lvl->number != BOSS_1_LEVEL && lvl->number != BOSS_2_LEVEL)
        {
            MonsterList *curMonster = lvl->monsterList->next;

            while(curMonster != NULL)
            {
                curMonster->lifes--;
                curMonster->frame_explosion = 0;
                curMonster = curMonster->next;
            }

            return 1;
        }
    }

    return 0;
}


void setLabel(SDL_Renderer *renderer, Fonts *fonts, Lvl *lvl, Player *player, int player_num, char *str)
{
    SDL_Color white = {255, 255, 255};

    player->label.texture = RenderTextBlended(renderer, fonts->ocraext_commands, str, white);
    SDL_QueryTexture(player->label.texture, NULL, NULL, &player->label.pos.w, &player->label.pos.h);
    player->label.frame_num = 0;
    player->label.pos.x = player->pos.x - lvl->startX[player_num] + player->pos.w / 2 - player->label.pos.w / 2;
    player->label.pos.y = player->pos.y - lvl->startY[player_num] - player->label.pos.h - 10 + player_num * (WINDOW_H / 2);
}

SDL_Texture* getScreenTexture(SDL_Renderer *renderer)
{
    SDL_RenderPresent(renderer);

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_W, WINDOW_H, 32, SDL_PIXELFORMAT_ABGR8888);
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ABGR8888, surface->pixels, surface->pitch);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}


void mapCollisionMonster(Lvl *lvl, MonsterList *currentMonster, int monsterIndex)
{
    int i, x1, x2, y1, y2;

    currentMonster->on_ground = 0;

    if(currentMonster->pos.h > TILE_SIZE)
        i = TILE_SIZE;
    else
        i = currentMonster->pos.h;

    for (;;)
    {
        x1 = (currentMonster->pos.x + currentMonster->dirX) / TILE_SIZE;
        x2 = (currentMonster->pos.x + currentMonster->dirX + currentMonster->pos.w - 1) / TILE_SIZE;

        y1 = (currentMonster->pos.y) / TILE_SIZE;
        y2 = (currentMonster->pos.y + i - 1) / TILE_SIZE;


        if (x1 >= 0 && x2 < lvl->width && y1 >= 0 && y2 < lvl->height)
        {
            if (currentMonster->dirX > 0)
            {
                if(lvl->solid[lvl->map[x2][y1]] || lvl->solid[lvl->map[x2][y2]])
                {
                    currentMonster->pos.x = x2 * TILE_SIZE;
                    currentMonster->pos.x -= currentMonster->pos.w + 1;
                    currentMonster->dirX = 0;
                }
            }

            else if(currentMonster->dirX < 0)
            {
                if (lvl->solid[lvl->map[x1][y1]] || lvl->solid[lvl->map[x1][y2]])
                {
                    currentMonster->pos.x = (x1 + 1) * TILE_SIZE;
                    currentMonster->dirX = 0;
                }
            }
        }

        if(i == currentMonster->pos.h)
            break;

        i += TILE_SIZE;

        if(i > currentMonster->pos.h)
            i = currentMonster->pos.h;
    }


    if(currentMonster->pos.w > TILE_SIZE)
        i = TILE_SIZE;
    else
        i = currentMonster->pos.w;


    for (;;)
    {
        x1 = (currentMonster->pos.x) / TILE_SIZE;
        x2 = (currentMonster->pos.x + i) / TILE_SIZE;

        y1 = (currentMonster->pos.y + currentMonster->dirY) / TILE_SIZE;
        y2 = (currentMonster->pos.y + currentMonster->dirY + currentMonster->pos.h) / TILE_SIZE;

        if (x1 >= 0 && x2 < lvl->width && y1 >= 0 && y2 < lvl->height)
        {
            if(currentMonster->dirY > 0)
            {
                if(lvl->solid[lvl->map[x1][y2]] || lvl->solid[lvl->map[x2][y2]])
                {
                    currentMonster->pos.y = y2 * TILE_SIZE;
                    currentMonster->pos.y -= currentMonster->pos.h;
                    currentMonster->dirY = 0;
                    currentMonster->on_ground = 1;
                }
            }
            else if (currentMonster->dirY < 0)
            {
                if (lvl->solid[lvl->map[x1][y1]] || lvl->solid[lvl->map[x2][y1]])
                {
                    currentMonster->pos.y = (y1 + 1) * TILE_SIZE;
                    currentMonster->dirY = 0;
                }
            }
        }

        if(i == currentMonster->pos.w)
            break;

        i += TILE_SIZE;

        if(i > currentMonster->pos.w)
            i = currentMonster->pos.w;
    }

    currentMonster->pos.x += currentMonster->dirX;
    currentMonster->pos.y += currentMonster->dirY;

    if(currentMonster->pos.x < 0)
        currentMonster->pos.x = 0;

    if(currentMonster->pos.x + currentMonster->pos.w >= lvl->maxX)
        currentMonster->pos.x = lvl->maxX - currentMonster->pos.w - 1;

    if(currentMonster->pos.y >= lvl->maxY)
        removeMonsterFromIndex(lvl->monsterList, monsterIndex);
}





void death(Player *player, int player_num, Sounds *sounds, Input *in, Settings *settings)
{
    player->lifes--;
    player->dead = 1;
    player->frame_explosion = 0;
    player->power_up = NO_POWER_UP;
    Mix_PlayChannel(-1, sounds->death, 0);
    Mix_PlayChannel(-1, sounds->explosion, 0);

    if(settings->haptic && in->controller[player_num].haptic != NULL)
        SDL_HapticRumblePlay(in->controller[player_num].haptic, 0.5, 500);
}


void createMonster(Lvl *lvl, Pictures *pictures, int x, int y, int lifes, int can_jump)
{
    MonsterList *monsterList = malloc(sizeof(MonsterList));
    initMonsterList(monsterList);

    monsterList->direction = LEFT;
    monsterList->pos.x = x;

    if(lvl->number != BOSS_1_LEVEL && lvl->number != BOSS_2_LEVEL)
    {
        monsterList->is_boss = 0;
        monsterList->pos.y = y;
        monsterList->pos.w = TILE_SIZE;
        monsterList->pos.h = TILE_SIZE;
    }
    else
    {
        monsterList->is_boss = 1;
        monsterList->pos.y = y - 90;
        SDL_QueryTexture(pictures->boss, NULL, NULL, &monsterList->pos.w, &monsterList->pos.h);
    }

    monsterList->on_ground = 0;
    monsterList->frame_explosion = -1;
    monsterList->lifes = lifes;
    monsterList->dirX = 0;
    monsterList->dirY = 0;
    monsterList->saveX = x - 1;
    monsterList->saveY = y - 1;

    insertMonster(lvl->monsterList, monsterList);
}


void createMovingPlat(Lvl *lvl, int x, int y, const int num_player)
{
    if(lvl->num_moving_plat < MAX_MOVING_PLAT)
    {
        lvl->moving_plat[lvl->num_moving_plat].pos.x = x;
        lvl->moving_plat[lvl->num_moving_plat].pos.y = y;
        lvl->moving_plat[lvl->num_moving_plat].pos.w = TILE_SIZE;
        lvl->moving_plat[lvl->num_moving_plat].pos.h = TILE_SIZE;

        lvl->moving_plat[lvl->num_moving_plat].beginX = x;
        lvl->moving_plat[lvl->num_moving_plat].beginY = y;
        lvl->moving_plat[lvl->num_moving_plat].is_player_on[0] = 0;
        lvl->moving_plat[lvl->num_moving_plat].is_player_on[1] = 0;
        lvl->moving_plat[lvl->num_moving_plat].direction = RIGHT;
        lvl->moving_plat[lvl->num_moving_plat].endX = -1;
        lvl->moving_plat[lvl->num_moving_plat].endY = y;

        while(lvl->moving_plat[lvl->num_moving_plat].endX == -1 && x < lvl->maxX)
        {
            x += TILE_SIZE;
            if(lvl->map[x / TILE_SIZE][y / TILE_SIZE] == MOVING_PLAT_END_NUM)
                lvl->moving_plat[lvl->num_moving_plat].endX = x;
        }

        lvl->num_moving_plat++;
    }
}













SDL_Point segment2segment(int Ax0, int Ay0, int Bx0, int By0, int Cx0, int Cy0, int Dx0, int Dy0)
{
    double Sx;
    double Sy;

    double Ax = Ax0;
    double Ay = Ay0;
    double Bx = Bx0;
    double By = By0;
    double Cx = Cx0;
    double Cy = Cy0;
    double Dx = Dx0;
    double Dy = Dy0;

    SDL_Point point;
    point.x = -1;
    point.y = -1;

    if (Ax == Bx)
    {
        if (Cx == Dx)
            return point;
        else
        {
            double pCD = (Cy - Dy) / (Cx - Dx);
            Sx = Ax;
            Sy = pCD*(Ax - Cx) + Cy;
        }
    }
    else
    {
        if (Cx == Dx)
        {
            double pAB = (Ay - By) / (Ax - Bx);
            Sx = Cx;
            Sy = pAB*(Cx - Ax) + Ay;
        }
        else if ((Ax == Cx && Ay == Cy) || (Ax == Dx && Ay == Dy))
        {
            Sx = Ax;
            Sy = Ay;
        }
        else
        {
            double pCD = (Cy - Dy) / (Cx - Dx);
            double pAB = (Ay - By) / (Ax - Bx);
            double oCD = Cy - pCD*Cx;
            double oAB = Ay - pAB*Ax;
            Sx = (oAB - oCD) / (pCD - pAB);
            Sy = pCD*Sx + oCD;
        }
    }

    if ((Sx<Ax && Sx<Bx) | (Sx>Ax && Sx>Bx) | (Sx<Cx && Sx<Dx) | (Sx>Cx && Sx>Dx) | (Sy<Ay && Sy<By) | (Sy>Ay && Sy>By) | (Sy<Cy && Sy<Dy) | (Sy>Cy && Sy>Dy))
        return point;

    point.x = Sx;
    point.y = Sy;
    return point;
}


void getSlopeSegment(int tx, int ty, int pente, SDL_Point *s1, SDL_Point *s2)
{
    int cy, dy;

    if(pente == TILE_PENTE_BenH_1)
    {
        cy = 0;
        dy = 16;
    }
    else if(pente == TILE_PENTE_BenH_2)
    {
        cy = 16;
        dy = 32;
    }
    else if(pente == TILE_PENTE_HenB_1)
    {
        cy = 32;
        dy = 16;
    }
    else // if(pente == TILE_PENTE_26_HenB_2)
    {
        cy = 16;
        dy = 0;
    }

    s1->x = tx * TILE_SIZE;
    s1->y = (ty + 1) * TILE_SIZE - cy;
    s2->x = (tx + 1) * TILE_SIZE;
    s2->y = (ty + 1) * TILE_SIZE - dy;
}


int slopeEquation(int pente, double *a, double *b)
{
    const double xLeft = 0;
    const double xRight = 32.0;
    int yLeft, yRight;

    if (pente == TILE_PENTE_BenH_1)
    {
        yLeft = 0;
        yRight = 16;
    }
    else if (pente == TILE_PENTE_BenH_2)
    {
        yLeft = 16;
        yRight = 32;
    }
    else if (pente == TILE_PENTE_HenB_1)
    {
        yLeft = 32;
        yRight = 16;
    }
    else // if (pente == TILE_PENTE_26_HenB_2)
    {
        yLeft = 16;
        yRight = 0;
    }

    double cd = (yRight - yLeft) / (xRight - xLeft);
    double oo = yLeft - cd * 0;
    *a = cd;
    *b = oo;

    return 1;
}


int checkSlope(Lvl *lvl, Player *player)
{
    // Initialisation
    int isOnSlope, goOnSlope, goOnSlopeUp, goOnSlopeDown;
    isOnSlope = goOnSlope = goOnSlopeUp = goOnSlopeDown = 0;
    int diagOffSet = 0;
    int yc = 0;
    int resetWasOnSlope = 0, checkWasOnSlope = 1;

    // Si on ne touche plus le sol, on ne se soucis plus de savoir qu'on était sur une pente.
    if(!player->wasOnGround)
        player->wasOnSlope = 0;

    // On récupère la position du Sprite (à noter qu'on effectue les tests avec le point "en bas au centre" du Sprite)
    int posIniX = player->posXmem + player->pos.w / 2;
    int xa = posIniX / TILE_SIZE;
    int posIniY = player->posYmem + player->pos.h - 1;
    int ya = posIniY / TILE_SIZE;

    // On récupère la destination du Sprite
    int posEndX = posIniX + player->dirXmem;
    int xb = posEndX / TILE_SIZE;
    int posEndY = posIniY + 1 + player->dirYmem;
    int yb = posEndY / TILE_SIZE;

    // Est-ce qu'on est sur une pente ?
    if (lvl->map[xa][ya] >= TILE_PENTE_BenH_1 && lvl->map[xa][ya] <= TILE_PENTE_HenB_2)
        isOnSlope = lvl->map[xa][ya];

    // Est-ce qu'on va sur une pente ?
    if (lvl->map[xb][yb] >= TILE_PENTE_BenH_1 && lvl->map[xb][yb] <= TILE_PENTE_HenB_2)
        goOnSlope = lvl->map[xb][yb];

    // Est-ce que la Tile au-dessus de la destination du Sprite est une pente ?
    if (lvl->map[xb][yb - 1] >= TILE_PENTE_BenH_1 && lvl->map[xb][yb - 1] <= TILE_PENTE_HenB_2)
        goOnSlopeUp = lvl->map[xb][yb - 1];

    // Est-ce que la Tile au-dessous de la destination du Sprite est une pente ?
    // La subtilité ici est qu'on est (normalement) déjà sur une pente, mais que le Sprite se
    // déplace si vite, qu'on ne voit pas que la Tile suivante est encore une pente !
    // En fait, ce n'est pas grave, c'est juste un peu plus réaliste de "coller" le Sprite au sol,
    // plutôt que de laisser le Sprite "flotter" dans les airs jusqu'au sol, quelques pixels plus loin...
    // (C'est surtout vrai pour les Tiles à pentes raides ou à grande vitesse)
    if (lvl->map[xb][yb + 1] >= TILE_PENTE_BenH_1 && lvl->map[xb][yb + 1] <= TILE_PENTE_HenB_2)
        goOnSlopeDown = lvl->map[xb][yb + 1];

    // Si on se dirige vers une pente
    if (goOnSlope > 0)
    {
        double a, b;

        // On récupère l'équation de la pente
        if (!slopeEquation(goOnSlope, &a, &b))
            return 0;

        // On determine la position en x du Sprite dans la Tile
        int xPos = posEndX - xb * TILE_SIZE;

        // On calcule sa position en y
        int yPos = a * xPos + b;

        // On borne le ypos à 31
        if (yPos > 31)
            yPos = 31;

        // On calcul l'Offset entre le haut de la Tile et le sol de la pente
        diagOffSet = TILE_SIZE - yPos;

        // La Tile "pente" est à la même hauteur que la Tile où se trouve le Sprite
        yc = yb;

        // Le Sprite est à présent sur une pente
        player->wasOnSlope = goOnSlope;

        // Puisqu'on traite le Sprite sur la pente,
        // inutile de traiter le Sprite quittant la pente
        checkWasOnSlope = 0;
    }

    // S'il y a une pente au dessus de celle où on va
    // (c'est à dire la Tile juste à côté du Sprite, car avec la gravité,
    // on "pointe" toujours la Tile en dessous)
    else if (goOnSlopeUp > 0)
    {
        double a, b;
        if (!slopeEquation(goOnSlopeUp, &a, &b))
            return 0;
        int xPos = posEndX - xb * TILE_SIZE;
        int yPos = a * xPos + b;
        if (yPos > 31)
            yPos = 31;
        diagOffSet = TILE_SIZE - yPos;

        // La Tile "pente" est 1 Tile au-dessus de la Tile où se trouve le Sprite
        yc = yb - 1;

        player->wasOnSlope = goOnSlopeUp;
        checkWasOnSlope = 0;
    }

    // Si on tombe ici, c'est que le Sprite ne va pas sur une pente mais qu'il est sur une pente.
    else if (isOnSlope > 0)
    {
    // Si on est en l'air,
        if (!player->wasOnGround)
        {

            // Il faut vérifier si le Sprite doit être stoppé par la pente.
            // Pour cela, on contrôle si la trajectoire du sprite croise le sol de la pente.
            // On vérifie donc si ces segments se croisent et si oui, en quel point.
            SDL_Point segmentD, segmentF;

            // On récupère le segment de la pente
            getSlopeSegment(xa, ya, isOnSlope, &segmentD, &segmentF);

            // On récupère la position du point de collision entre les segments (s'il y a lieu, sinon -1)
            SDL_Point point = segment2segment(posIniX, posIniY, posEndX, posEndY, segmentD.x, segmentD.y, segmentF.x, segmentF.y);

            // Pas d'intersection
            if (point.x == -1)
            {
                // On applique les valeurs de départ afin d'éviter d'être repoussé par la Tile
                // solide (par mapCollision) lorsqu'on quitte une pente en sautant
                player->pos.x = player->posXmem;
                player->dirX = player->dirXmem;
                return 0;
            }


            // On positionne le Sprite
            player->pos.x = point.x - player->pos.w / 2;
            player->dirX = 0;
            player->pos.y = point.y;
            player->pos.y -= player->pos.h;

            // Si le Sprite est dans la phase ascendante du saut, on le laisse poursuivre
            // Sinon, on le stoppe et on l'indique comme étant au sol.
            if (player->dirY > 0)
            {
                player->dirY = 0;
                player->on_ground = 1;
            }

            player->wasOnSlope = isOnSlope;

            return 1;
        }

        // Si on est sur le sol, on vérifie si la Tile suivante, et en desssous, est une pente.
        // Dans ce cas, on déplace le Sprite sur la pente,
        else
        {
            if (goOnSlopeDown > 0)
            {
                double a, b;
                if (!slopeEquation(goOnSlopeDown, &a, &b))
                    return 0;
                int xPos = posEndX - xa * TILE_SIZE;

                //Ici, xPos étant sur la Tile suivante, on retranche une Tile pour avoir le bon yPos
                if (player->dirXmem > 0)
                    xPos -= TILE_SIZE;
                else
                    xPos += TILE_SIZE;

                int yPos = a * xPos + b;
                if (yPos > 31)
                    yPos = 31;
                diagOffSet = TILE_SIZE - yPos;
                yc = yb + 1;
                player->wasOnSlope = isOnSlope;
                checkWasOnSlope = 0;
            }

        // sinon on fait la transition en douceur avec "entity->wasOnSlope" ("checkWasOnSlope" restant à true)
        }

    }

    // Finalement, si on est pas sur une pente, qu'on ne va pas sur une pente
    // mais qu'on y était le tour d'avant, on force une sortie en douceur
    if (player->wasOnSlope > 0 && checkWasOnSlope)
    {
        // Si on quitte une montée
        if ((player->dirXmem > 0 && player->wasOnSlope == TILE_PENTE_BenH_2) || (player->dirXmem < 0 && player->wasOnSlope == TILE_PENTE_HenB_1))
            yc = ya;

        // Si on quitte une descente
        else
        {
            if ((player->dirXmem > 0 && player->wasOnSlope == TILE_PENTE_HenB_2) || (player->dirXmem < 0 && player->wasOnSlope == TILE_PENTE_BenH_1))
                yc = ya + 1;
        }

        resetWasOnSlope = 1;
    }

    // Si on "est" ou si on "quitte" une pente (donc que wasOnSlope > 0)
    if (player->wasOnSlope > 0)
    {
        // On calcul l'écart entre le sol de la pente et la position du Sprite
        // Si l'écart est plus grand que la vitesse de chute, on continue de laisser tomber le Sprite
        if (player->wasOnGround == 0)
        {
            int newPos = yc * TILE_SIZE + diagOffSet;
            int ecart = newPos - posIniY;

            if (ecart > player->dirYmem)
            {
                player->pos.y = player->posYmem;
                player->dirY = player->dirYmem;
                player->on_ground = 0;
                return 0;
            }
        }

        // On positionne le Sprite sur la pente
        player->pos.x = player->posXmem;
        player->dirX = player->dirXmem;
        player->pos.y = yc * TILE_SIZE + diagOffSet;
        player->pos.y -= player->pos.h;
        player->dirY = 0;
        player->on_ground = 1;
        player->can_jump = 1;

        // On n'oublie pas de remettre wasOnSlope à 0 si nécéssaire
        if (resetWasOnSlope)
            player->wasOnSlope = 0;

        return 1;
    }

    return 0;
}


void createBullet(Player *player, int player_num, Lvl *lvl, Pictures *pictures)
{
    BulletList *bullet = malloc(sizeof(BulletList));
    initBulletList(bullet);

    if(player->direction == RIGHT)
        bullet->dirX = BULLET_SPEED;
    else
        bullet->dirX = -BULLET_SPEED;

    SDL_QueryTexture(pictures->bullet_left, NULL, NULL, &bullet->pos.w, &bullet->pos.h);
    bullet->pos.x = player->pos.x + PLAYER_W / 2 - bullet->pos.w / 2;
    bullet->pos.y = player->pos.y + PLAYER_H / 2 - 15;

    bullet->player_num = player_num;

    insertBullet(lvl->bulletList, bullet);
}


void updateBullets(BulletList *bulletList, MonsterList *monsterList, Sounds *sounds, Lvl *lvl, Player *players[])
{
    int index = 0;
    BulletList *curBullet = bulletList->next;
    int bullet_removed = 0;

    while(curBullet != NULL)
    {
        int i, x1, x2, y1, y2;

        if(curBullet->pos.h > TILE_SIZE)
            i = TILE_SIZE;
        else
            i = curBullet->pos.h;

        for (;;)
        {
            x1 = (curBullet->pos.x + curBullet->dirX) / TILE_SIZE;
            x2 = (curBullet->pos.x + curBullet->dirX + curBullet->pos.w - 1) / TILE_SIZE;

            y1 = (curBullet->pos.y) / TILE_SIZE;
            y2 = (curBullet->pos.y + i - 1) / TILE_SIZE;

            if (x1 >= 0 && x2 < lvl->width && y1 >= 0 && y2 < lvl->height)
            {
                if (curBullet->dirX > 0)
                {
                    if(lvl->solid[lvl->map[x2][y1]] || lvl->solid[lvl->map[x2][y2]])
                    {
                        curBullet = curBullet->next;
                        removeBulletFromIndex(bulletList, index);
                        bullet_removed = 1;
                    }
                }
                else if(curBullet->dirX < 0)
                {
                    if (lvl->solid[lvl->map[x1][y1]] || lvl->solid[lvl->map[x1][y2]])
                    {
                        curBullet = curBullet->next;
                        removeBulletFromIndex(bulletList, index);
                        bullet_removed = 1;
                    }
                }
            }

            if(bullet_removed || i == curBullet->pos.h)
                break;

            i += TILE_SIZE;

            if(i > curBullet->pos.h)
                i = curBullet->pos.h;
        }

        if(!bullet_removed && (curBullet->pos.x + curBullet->pos.w < 0 || curBullet->pos.x >= lvl->width * TILE_SIZE || curBullet->pos.y > lvl->height * TILE_SIZE || curBullet->pos.y + curBullet->pos.h < 0))
        {
            curBullet = curBullet->next;
            removeBulletFromIndex(bulletList, index);
            bullet_removed = 1;
        }

        MonsterList *curMonster = monsterList->next;

        while(curMonster != NULL && !bullet_removed)
        {
            if(((curBullet->pos.x + curBullet->pos.w >= curMonster->pos.x && curBullet->pos.x + curBullet->pos.w < curMonster->pos.x + curMonster->pos.w) || (curBullet->pos.x <= curMonster->pos.x + curMonster->pos.w && curBullet->pos.x > curMonster->pos.x)) && curBullet->pos.y + curBullet->pos.h / 2 >= curMonster->pos.y && curBullet->pos.y + curBullet->pos.h / 2 < curMonster->pos.y + curMonster->pos.h && curMonster->lifes > 0)
            {
                curMonster->lifes--;
                curMonster->frame_explosion = 0;
                Mix_PlayChannel(-1, sounds->explosion, 0);
                players[curBullet->player_num]->killed++;

                curBullet = curBullet->next;
                removeBulletFromIndex(bulletList, index);
                bullet_removed = 1;
            }

            curMonster = curMonster->next;
        }

        if(!bullet_removed)
        {
            curBullet->pos.x += curBullet->dirX;
            curBullet = curBullet->next;
            index++;
        }

        bullet_removed = 0;
    }
}



void displayBullets(SDL_Renderer *renderer, Lvl *lvl, Player *player, int player_num, int num_player, Pictures *pictures)
{
    BulletList *curBullet = lvl->bulletList->next;

    while(curBullet != NULL)
    {
        SDL_Rect pos_dst;
        pos_dst.x = curBullet->pos.x - lvl->startX[player_num];
        pos_dst.y = curBullet->pos.y - lvl->startY[player_num] + player_num * WINDOW_H / 2;
        pos_dst.w = curBullet->pos.w;
        pos_dst.h = curBullet->pos.h;

        if(pos_dst.x + pos_dst.w >= 0 && pos_dst.x < WINDOW_W && pos_dst.y + pos_dst.h >= (WINDOW_H / num_player) * player_num && pos_dst.y < (WINDOW_H / num_player) * (player_num + 1))
            SDL_RenderCopy(renderer, curBullet->dirX < 0 ? pictures->bullet_left : pictures->bullet_right, NULL, &pos_dst);

        curBullet = curBullet->next;
    }
}


int receive_thread(void *data)
{
    unsigned long time1 = 0, time2 = 0;

    while(receive)
    {
        receivePos((Net*) data, &other_packet);
        waitGame(&time1, &time2, APPR_DELAY_GAME);
    }

    return 0;
}


void displayLabel(SDL_Renderer *renderer, Player *player)
{
    if(player->label.frame_num != -1)
    {
        if(player->label.pos.y < 10)
            player->label.pos.y = 10;

        SDL_RenderCopy(renderer, player->label.texture, NULL, &player->label.pos);
        player->label.frame_num++;
        player->label.pos.y--;

        if(player->label.frame_num >= 60)
        {
            SDL_DestroyTexture(player->label.texture);
            player->label.frame_num = -1;
        }
    }
}


void teleport(Lvl *lvl, Player *player, int x, int y)
{
    for(int i = 0; i < lvl->width; i++)
    {
        for(int j = 0; j < lvl->height; j++)
        {
            if(lvl->map[i][j] == TELEPORT_NUM && (i != x || j != y))
            {
                player->pos.x = i * TILE_SIZE;
                player->pos.y = j * TILE_SIZE;
                return;
            }
        }
    }
}

