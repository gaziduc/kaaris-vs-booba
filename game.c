#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "event.h"
#include "game.h"
#include "transition.h"
#include "data.h"
#include "text.h"
#include "list.h"

void loadLevel(SDL_Renderer *renderer, const int lvl_num, Lvl *lvl, int mode, Settings *settings)
{
    char lvl_name[100] = "";
    char sky_filename[100] = "";
    char tileset_filename[100] = "";
    char solid_filename[100] = "";
    char music_filename[100] = "";
    char weather_filename[100] = "";
    char weather_sfx_filename[100] = "";
    char filename[200] = "";
    FILE *file = NULL;

    sprintf(filename, "./data/maps/map_%d.txt", lvl_num);
    file = fopen(filename, "r");
    if(file == NULL)
        exit(EXIT_FAILURE);

    lvl->number = lvl_num;

    lvl->weather = malloc(sizeof(Weather));
    if(lvl->weather == NULL)
        exit(EXIT_FAILURE);

    fgets(lvl_name, sizeof(lvl_name) / sizeof(lvl_name[0]), file);

    int size = strlen("name=");
    memset(lvl->name, '\0', sizeof(lvl->name));
    for(int i = size; lvl_name[i] != '\n'; i++)
        lvl->name[i - size] = lvl_name[i];

    fscanf(file, "width=%d\nheight=%d\nsky=%s\ntileset=%s\ncolision=%s\nmusic=%s\nweather=%s\nweather_num_elm=%d\nweather_dir_x=[%d,%d]\nweather_dir_y=[%d,%d]\nweather_sfx=%s\n", &lvl->width, &lvl->height, sky_filename, tileset_filename, solid_filename, music_filename, weather_filename, &lvl->weather->num_elm, &lvl->weather->dirXmin, &lvl->weather->dirXmax, &lvl->weather->dirYmin, &lvl->weather->dirYmax, weather_sfx_filename);

    sprintf(filename, "./data/background/%s", sky_filename);
    lvl->sky = IMG_LoadTexture(renderer, filename);
    if(lvl->sky == NULL)
        exit(EXIT_FAILURE);

    sprintf(filename, "./data/tilesets/%s", tileset_filename);
    lvl->tileset = IMG_LoadTexture(renderer, filename);
    if(lvl->tileset == NULL)
        exit(EXIT_FAILURE);

    sprintf(filename, "./data/weather/%s", weather_filename);
    lvl->weather->texture = IMG_LoadTexture(renderer, filename);
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

    lvl->startX = 0;
    lvl->startY = 0;
    lvl->maxX = lvl->width * TILE_SIZE;
    lvl->maxY = lvl->height * TILE_SIZE;


    sprintf(filename, "./data/tilesets/%s", solid_filename);
    file = fopen(filename, "r");
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

    sprintf(filename, "./data/sfx/%s", weather_sfx_filename);
    lvl->weather->sfx = Mix_LoadWAV(filename);
    if(lvl->weather->sfx == NULL)
        exit(EXIT_FAILURE);

    Mix_VolumeChunk(lvl->weather->sfx, settings->sfx_volume);

    lvl->weather->pos = malloc(lvl->weather->num_elm * sizeof(SDL_Rect));
    if(lvl->weather->pos == NULL)
        exit(EXIT_FAILURE);

    lvl->weather->dirX = malloc(lvl->weather->num_elm * sizeof(int));
    if(lvl->weather->dirX == NULL)
        exit(EXIT_FAILURE);

    lvl->weather->dirY = malloc(lvl->weather->num_elm * sizeof(int));
    if(lvl->weather->dirY == NULL)
        exit(EXIT_FAILURE);

    lvl->num_moving_plat = 0;

    if(mode == PLAY)
    {
        sprintf(filename, "./data/music/%s", music_filename);
        lvl->music = Mix_LoadMUS(filename);
        if(lvl->music == NULL)
            exit(EXIT_FAILURE);

        Mix_PlayMusic(lvl->music, -1);
        Mix_PlayChannel(-1, lvl->weather->sfx, -1);

        for(int i = 0; i < lvl->weather->num_elm; i++)
            setWeatherElement(lvl->weather, i, 0);
    }

}


void setWeatherElement(Weather *weather, int elm_num, int is_initted)
{
    weather->pos[elm_num].x = rand() % (int) WINDOW_W;
    if(is_initted)
        weather->pos[elm_num].y = 0;
    else
        weather->pos[elm_num].y = rand() % (int) WINDOW_H;
    SDL_QueryTexture(weather->texture, NULL, NULL, &weather->pos[elm_num].w, &weather->pos[elm_num].h);

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
        Mix_PauseMusic();
        Mix_HaltMusic();
        Mix_FreeMusic(lvl->music);
    }


    Mix_FreeChunk(lvl->weather->sfx);

    free(lvl->weather->pos);
    free(lvl->weather->dirX);
    free(lvl->weather->dirY);
    SDL_DestroyTexture(lvl->weather->texture);

    free(lvl->weather);
}


void displayGame(SDL_Renderer *renderer, Pictures *pictures, Lvl *lvl, Player *player, unsigned long frame_num, int mode)
{
    int x, y, mapX, x1, x2, mapY, y1, y2;

    mapX = lvl->startX / TILE_SIZE;
    x1 = (lvl->startX % TILE_SIZE) * -1;
    x2 = x1 + WINDOW_W + (x1 == 0 ? 0 : TILE_SIZE);

    mapY = lvl->startY / TILE_SIZE;
    y1 = (lvl->startY % TILE_SIZE) * -1;
    y2 = y1 + WINDOW_H + (y1 == 0 ? 0 : TILE_SIZE);

    for (y = y1; y < y2; y += TILE_SIZE)
    {
        mapX = lvl->startX / TILE_SIZE;

        for (x = x1; x < x2; x += TILE_SIZE)
        {
            if(lvl->map[mapX][mapY] == MONSTER_NUM && mode == PLAY)
            {
                 createMonster(player, mapX * TILE_SIZE, mapY * TILE_SIZE, (lvl->number == NUM_LEVEL) ? 10 : 1);
                 lvl->map[mapX][mapY] = 0;
            }
            else if(lvl->map[mapX][mapY] == MOVING_PLAT_NUM && mode == PLAY)
            {
                createMovingPlat(lvl, mapX * TILE_SIZE, mapY * TILE_SIZE);
                lvl->map[mapX][mapY] = 0;
            }

            SDL_Rect pos_dst;
            pos_dst.x = x;
            pos_dst.y = y;
            pos_dst.w = TILE_SIZE;
            pos_dst.h = TILE_SIZE;

            SDL_Rect pos_src;
            pos_src.w = TILE_SIZE;
            pos_src.h = TILE_SIZE;
            pos_src.x = lvl->map[mapX][mapY] * TILE_SIZE;
            pos_src.y = ((frame_num % (NUM_ANIM_TILE * NUM_FRAME_ANIM)) / NUM_FRAME_ANIM) * TILE_SIZE;

            SDL_RenderCopy(renderer, lvl->tileset, &pos_src, &pos_dst);


            mapX++;
        }

        mapY++;
    }
}


void initPlayer(SDL_Renderer *renderer, Player *player)
{
    player->texture[IDLE_LEFT] = IMG_LoadTexture(renderer, "./data/tilesets/idleleft.png");
    player->texture[IDLE_RIGHT] = IMG_LoadTexture(renderer, "./data/tilesets/idleright.png");
    player->texture[WALK_LEFT] = IMG_LoadTexture(renderer, "./data/tilesets/walkleft.png");
    player->texture[WALK_RIGHT] = IMG_LoadTexture(renderer, "./data/tilesets/walkright.png");
    player->texture[JUMP_LEFT] = IMG_LoadTexture(renderer, "./data/tilesets/jumpleft.png");
    player->texture[JUMP_RIGHT] = IMG_LoadTexture(renderer, "./data/tilesets/jumpright.png");
    player->direction = RIGHT;
    player->state = IDLE_RIGHT;
    player->frame = 0;
    player->pos.w = PLAYER_W;
    player->pos.h = PLAYER_H;
    player->lifes = 3;
    player->money = 0;
    player->killed = 0;
    player->isCheckpointActive = 0;
    respawn(player);
    player->dead = 0;
    player->frame_explosion = 0;
    player->monsterList = malloc(sizeof(MonsterList));
    initMonsterList(player->monsterList);
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
}



void displayLevelName(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Lvl *lvl, Input *in)
{
    SDL_Texture *texture[2];
    SDL_Rect pos_dst[2];
    SDL_Color white = {255, 255, 255};
    char str[100] = "";
    unsigned long time1 = 0, time2 = 0;
    int escape = 0;

    sprintf(str, "Niveau %d", lvl->number);
    texture[0] = RenderTextBlended(renderer, fonts->ocraext_title, str, white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, lvl->name, white);

    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = WINDOW_H / 2 - pos_dst[0].h / 2 - 50;

    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = WINDOW_H / 2 - pos_dst[1].h / 2 + 50;

    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 1);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE || KEY_ENTER_MENU)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;
            in->key[SDL_SCANCODE_SPACE] = 0;
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;
            in->controller.buttons[0] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);
        SDL_RenderCopy(renderer, texture[0], NULL, &pos_dst[0]);
        SDL_RenderCopy(renderer, texture[1], NULL, &pos_dst[1]);
        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
    }

    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0);

    SDL_DestroyTexture(texture[0]);
    SDL_DestroyTexture(texture[1]);
}


void playGame(SDL_Renderer *renderer, Input *in, Pictures *pictures, Fonts *fonts, Sounds *sounds, Settings *settings)
{
    intro(renderer, in, pictures, fonts, sounds);

    Lvl *lvl = malloc(sizeof(Lvl));
    if(lvl == NULL)
        exit(EXIT_FAILURE);

    loadLevel(renderer, 1, lvl, PLAY, settings);
    unsigned long time1 = 0, time2 = 0, frame_num = 0;
    int escape = 0, finished = 0;
    Player *player = malloc(sizeof(Player));
    initPlayer(renderer, player);

    displayLevelName(renderer, pictures, fonts, lvl, in);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;
            escape = 1;
        }
        if(KEY_LEFT_GAME)
        {
            player->direction = LEFT;
            player->dirX = -4;

            if(player->state != WALK_LEFT && player->on_ground)
            {
                player->state = WALK_LEFT;
                player->frame = 0;
            }
        }
        if(KEY_RIGHT_GAME)
        {
            player->direction = RIGHT;
            player->dirX = 4;

            if(player->state != WALK_RIGHT && player->on_ground)
            {
                player->state = WALK_RIGHT;
                player->frame = 0;
            }
        }
        if(KEY_UP_GAME)
        {
            if(player->on_ground)
            {
                player->dirY = -JUMP_HEIGHT;
                player->on_ground = 0;
                player->can_jump = 1;
                Mix_PlayChannel(-1, sounds->jump, 0);
            }
            else if(player->can_jump)
            {
                player->dirY = -JUMP_HEIGHT;
                player->can_jump = 0;
                Mix_PlayChannel(-1, sounds->jump, 0);
            }

            in->key[SDL_SCANCODE_UP] = 0;
            in->controller.buttons[0] = 0;
        }


        if(!player->on_ground)
        {
            if(player->direction == RIGHT && player->state != JUMP_RIGHT)
            {
                player->state = JUMP_RIGHT;
                player->frame = 0;
            }
            if(player->direction == LEFT && player->state != JUMP_LEFT)
            {
                player->state = JUMP_LEFT;
                player->frame = 0;
            }
        }
        else if(!(KEY_LEFT_GAME) && !(KEY_RIGHT_GAME))
        {
            if(player->state == JUMP_LEFT || player->state == WALK_LEFT || player->state == IDLE_LEFT)
                player->state = IDLE_LEFT;
            else
                player->state = IDLE_RIGHT;
        }



        updateMovingPlat(lvl, player);
        if(!player->dead)
            mapCollisionPlayer(renderer, lvl, player, sounds, in, pictures, fonts, settings);

        SDL_RenderClear(renderer); // put this line after mapCollisionPlayer(...), because of the animation of the end of a level.

        centerScrollingOnPlayer(lvl, player);
        SDL_RenderCopy(renderer, lvl->sky, NULL, NULL);
        displayWeather(renderer, player, lvl->weather, frame_num);
        displayGame(renderer, pictures, lvl, player, frame_num, PLAY);
        displayPlayer(renderer, lvl, player, pictures);
        if(updateMonsters(lvl, player, sounds, in)) // If the boss was killed
        {
            escape = 1;
            finished = 1;
        }
        displayMonsters(renderer, lvl, player, pictures, frame_num);
        displayMovingPlat(renderer, lvl, frame_num);
        displayHUD(renderer, player, pictures, fonts, lvl->number);
        SDL_RenderPresent(renderer);

        frame_num++;

        player->frame++;
        int w;
        SDL_QueryTexture(player->texture[player->state], NULL, NULL, &w, NULL);
        if(player->frame >= (w * 2) / PLAYER_W)
            player->frame = 0;

        player->dirX = 0;
        player->dirY += GRAVITY_SPEED;
        if(player->dirY >= MAX_FALL_SPEED)
            player->dirY = MAX_FALL_SPEED;

        wait(&time1, &time2, DELAY_GAME);

        if(player->lifes < 0 && !player->dead)
        {
            gameOver(renderer, pictures, fonts, in);
            escape = 1;
        }

        if(player->invicibleFramesLeft > 0)
            player->invicibleFramesLeft--;
    }

    freeLevel(lvl, PLAY);
    free(lvl);


    if(finished)
        displayScore(renderer, player, in, pictures, fonts);

    freePlayer(player);
}


void displayScore(SDL_Renderer *renderer, Player *player, Input *in, Pictures *pictures, Fonts *fonts)
{
    long score = player->lifes * 1000 + player->killed * 100 + player->money * 10;
    int escape = 0;
    SDL_Texture *texture[5];
    SDL_Rect pos_dst[5];
    SDL_Color white = {255, 255, 255};
    unsigned long time1 = 0, time2 = 0;
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


    transition(renderer, pictures->title, 5, texture, pos_dst, ENTERING, 1);

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
            in->controller.buttons[0] = 0;
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller.buttons[6] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < 5; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
    }

    transition(renderer, pictures->title, 5, texture, pos_dst, EXITING, 0);

    for(int i = 0; i < 5; i++)
        SDL_DestroyTexture(texture[i]);



    unsigned long scores[NUM_SCORES];
    char names[NUM_SCORES][NAME_LEN];
    loadScores(scores, names);

    memset(str, '\0', sizeof(str));
    enterName(renderer, fonts, pictures, in, str);

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
    displayScoreList(renderer, pictures, fonts, in, scores, names);
}


void displayWeather(SDL_Renderer *renderer, Player *player, Weather *weather, unsigned long frame_num)
{
    int w;
    SDL_QueryTexture(weather->texture, NULL, NULL, &w, NULL);

    for(int i = 0; i < weather->num_elm; i++)
    {
        if(weather->pos[i].y >= WINDOW_H || weather->pos[i].x >= WINDOW_W || weather->pos[i].x + w < 0)
            setWeatherElement(weather, i, 1);
        else
        {
            weather->pos[i].x += weather->dirX[i];
            weather->pos[i].y += weather->dirY[i];
        }

        SDL_RenderCopy(renderer, weather->texture, NULL, &weather->pos[i]);
    }

}




void gameOver(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in)
{
    SDL_Color white = {255, 255, 255};
    SDL_Texture *texture = RenderTextBlended(renderer, fonts->ocraext_message, "Appuyez sur ENTREE...", white);
    Mix_Music *music = Mix_LoadMUS("./data/music/zoo.mp3");
    Mix_PlayMusic(music, -1);
    SDL_Rect pos_dst;
    unsigned long time1 = 0, time2 = 0;
    int escape = 0, frame = 0;

    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W / 2 - pos_dst.w / 2;
    pos_dst.y = WINDOW_H - pos_dst.h - 60;

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
            in->controller.buttons[0] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->gameover, NULL, NULL);

        frame = frame % 60;
        if(frame < 30)
            SDL_RenderCopy(renderer, texture, NULL, &pos_dst);

        SDL_RenderPresent(renderer);

        wait(&time1, &time2, DELAY_GAME);
        frame++;
    }

    SDL_DestroyTexture(texture);
    Mix_HaltMusic();
    Mix_FreeMusic(music);
}


void freePlayer(Player *player)
{
    for(int i = 0; i < NUM_STATE; i++)
        SDL_DestroyTexture(player->texture[i]);

    removeAllMonsters(player->monsterList);
    free(player->monsterList);

    free(player);
}


void displayHUD(SDL_Renderer *renderer, Player *player, Pictures *pictures, Fonts *fonts, int level_num)
{
    SDL_Color color = {255, 0, 0};
    char str[100] = "";
    SDL_Rect pos_dst;
    pos_dst.x = 10;
    pos_dst.y = 10;
    SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_dst.w, &pos_dst.h);
    SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_dst);

    sprintf(str, "x %d", player->lifes);
    SDL_Texture *texture = RenderTextBlended(renderer, fonts->ocraext_message, str, color);
    pos_dst.x += pos_dst.w + 10;
    pos_dst.y += pos_dst.h / 2;
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.y -= pos_dst.h / 2;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);

    pos_dst.x += pos_dst.w + 20;
    pos_dst.y = 10;
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


    sprintf(str, "Mini-Booba tués : %d", player->killed);
    texture = RenderTextBlended(renderer, fonts->ocraext_message, str, color);
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst.w, &pos_dst.h);
    pos_dst.x = WINDOW_W - pos_dst.w - 20;
    SDL_RenderCopy(renderer, texture, NULL, &pos_dst);
    SDL_DestroyTexture(texture);

    if(level_num == NUM_LEVEL)
        displayBossLife(renderer, player, fonts);
}


void displayPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, Pictures *pictures)
{
    SDL_Rect pos_src;
    SDL_Rect pos_dst;

    if(!player->dead)
    {
        pos_src.x = (player->frame / 2) * PLAYER_W;
        pos_src.y = 0;
        pos_src.w = PLAYER_W;
        pos_src.h = PLAYER_H;

        pos_dst.x = player->pos.x - lvl->startX;
        pos_dst.y = player->pos.y - lvl->startY;
        pos_dst.w = player->pos.w;
        pos_dst.h = player->pos.h;

        SDL_RenderCopy(renderer, player->texture[player->state], &pos_src, &pos_dst);
    }
    else
    {
        pos_src.x = (player->frame_explosion / 8) * EXPLOSION_SIZE;
        pos_src.y = 0;
        pos_src.w = EXPLOSION_SIZE;
        pos_src.h = EXPLOSION_SIZE;

        pos_dst.x = player->pos.x + TILE_SIZE / 2 - EXPLOSION_SIZE / 2 - lvl->startX;
        pos_dst.y = player->pos.y + TILE_SIZE / 2 - EXPLOSION_SIZE / 2 - lvl->startY;
        pos_dst.w = EXPLOSION_SIZE;
        pos_dst.h = EXPLOSION_SIZE;

        SDL_RenderCopy(renderer, pictures->explosion, &pos_src, &pos_dst);

        player->frame_explosion++;
        if(player->frame_explosion >= 56)
        {
            player->dead = 0;
            respawn(player);
        }
    }

}



void displayMovingPlat(SDL_Renderer *renderer, Lvl *lvl, unsigned long frame_num)
{
    for(int i = 0; i < lvl->num_moving_plat; i++)
    {
        SDL_Rect pos_src;
        pos_src.x = MOVING_PLAT_NUM * TILE_SIZE;
        pos_src.y = ((frame_num % (NUM_ANIM_TILE * NUM_FRAME_ANIM)) / NUM_FRAME_ANIM) * TILE_SIZE;
        pos_src.w = TILE_SIZE;
        pos_src.h = TILE_SIZE;

        SDL_Rect pos_dst;
        pos_dst.x = lvl->moving_plat[i].pos.x - lvl->startX;
        pos_dst.y = lvl->moving_plat[i].pos.y - lvl->startY;
        pos_dst.w = TILE_SIZE;
        pos_dst.h = TILE_SIZE;

        SDL_RenderCopy(renderer, lvl->tileset, &pos_src, &pos_dst);
    }
}



void displayMonsters(SDL_Renderer *renderer, Lvl *lvl, Player *player, Pictures *pictures, unsigned long frame_num)
{
    MonsterList *current = player->monsterList->next;

    while(current != NULL)
    {
        SDL_Rect pos_src;
        SDL_Rect pos_dst;

        if(current->monster.lifes > 0)
        {
            pos_src.x = MONSTER_NUM * TILE_SIZE;
            pos_src.y = ((frame_num % (NUM_ANIM_TILE * NUM_FRAME_ANIM)) / NUM_FRAME_ANIM) * TILE_SIZE;
            pos_src.w = TILE_SIZE;
            pos_src.h = TILE_SIZE;

            pos_dst.x = current->monster.pos.x - lvl->startX;
            pos_dst.y = current->monster.pos.y - lvl->startY;
            pos_dst.w = TILE_SIZE;
            pos_dst.h = TILE_SIZE;

            SDL_RenderCopy(renderer, lvl->tileset, &pos_src, &pos_dst);
        }

        if(current->monster.frame_explosion >= 0)
        {
            pos_src.x = (current->monster.frame_explosion / 8) * EXPLOSION_SIZE;
            pos_src.y = 0;
            pos_src.w = EXPLOSION_SIZE;
            pos_src.h = EXPLOSION_SIZE;

            pos_dst.x = current->monster.pos.x + TILE_SIZE / 2 - EXPLOSION_SIZE / 2 - lvl->startX;
            pos_dst.y = current->monster.pos.y + TILE_SIZE / 2 - EXPLOSION_SIZE / 2 - lvl->startY;
            pos_dst.w = EXPLOSION_SIZE;
            pos_dst.h = EXPLOSION_SIZE;

            SDL_RenderCopy(renderer, pictures->explosion, &pos_src, &pos_dst);
        }

        current = current->next;
    }
}


void updateMovingPlat(Lvl *lvl, Player *player)
{
    for(int i = 0; i < lvl->num_moving_plat; i++)
    {
        if(lvl->moving_plat[i].direction == RIGHT)
        {
            lvl->moving_plat[i].pos.x += 2;
            if(lvl->moving_plat[i].is_player_on)
                player->dirX += 2;
        }
        else
        {
             lvl->moving_plat[i].pos.x -= 2;
             if(lvl->moving_plat[i].is_player_on)
                player->dirX -= 2;
        }


        if(lvl->moving_plat[i].pos.x == lvl->moving_plat[i].endX)
            lvl->moving_plat[i].direction = LEFT;
        else if(lvl->moving_plat[i].pos.x == lvl->moving_plat[i].beginX)
            lvl->moving_plat[i].direction = RIGHT;
    }
}


int updateMonsters(Lvl *lvl, Player *player, Sounds *sounds, Input *in)
{
    int index = 0;
    MonsterList *current = player->monsterList->next;

    while(current != NULL)
    {
        current->monster.dirX = 0;
        current->monster.dirY += GRAVITY_SPEED;

        if(current->monster.frame_explosion >= 0)
        {
            current->monster.frame_explosion++;
            if(current->monster.frame_explosion >= 56)
            {
                current->monster.frame_explosion = -1;
                if(current->monster.lifes <= 0)
                {
                    current = current->next;
                    removeMonsterFromIndex(player->monsterList, index);

                    if(lvl->number == NUM_LEVEL)
                        return 1;

                    continue;
                }
            }
        }


        if(current->monster.lifes > 0)
        {
            if(current->monster.dirY >= MAX_FALL_SPEED)
                current->monster.dirY = MAX_FALL_SPEED;

            if(current->monster.pos.x == current->monster.saveX || checkFall(lvl, player, current))
            {
                if(current->monster.direction == LEFT)
                    current->monster.direction = RIGHT;
                else
                    current->monster.direction = LEFT;
            }

            if(current->monster.direction == LEFT)
                current->monster.dirX -= (lvl->number == NUM_LEVEL) ? 12 : 2;
            else
                current->monster.dirX += (lvl->number == NUM_LEVEL) ? 12 : 2;


            current->monster.saveX = current->monster.pos.x;

            mapCollisionMonster(lvl, player, current, index);

            if(!player->dead)
            {
                int collision = collide(player, current);

                if(collision == 1 || (collision == 2 && player->invicibleFramesLeft > 0))
                {
                    if(lvl->number < NUM_LEVEL)
                        player->killed++;

                    if(player->invicibleFramesLeft == 0)
                    {
                        player->dirY = -JUMP_HEIGHT;
                        player->can_jump = 1;
                    }

                    current->monster.lifes--;
                    current->monster.frame_explosion = 0;
                    Mix_PlayChannel(-1, sounds->explosion, 0);
                }
                else if(collision == 2)
                    death(player, sounds, in);
            }
        }

        current = current->next;
        index++;
    }


    return 0;
}


int collide(Player *player, MonsterList *currentMonster)
{
    if(player->pos.x >= currentMonster->monster.pos.x + currentMonster->monster.pos.w || player->pos.x + player->pos.w <= currentMonster->monster.pos.x || player->pos.y >= currentMonster->monster.pos.y + currentMonster->monster.pos.h || player->pos.y + player->pos.h <= currentMonster->monster.pos.y)
        return 0;
    if(player->pos.y + player->pos.h <= currentMonster->monster.pos.y + 10)
        return 1;

    return 2;
}


void displayBossLife(SDL_Renderer *renderer, Player *player, Fonts *fonts)
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

    if(player->monsterList->next != NULL && player->monsterList->next->monster.lifes > 0)
    {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, player->monsterList->next->monster.lifes * 114, 5);
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


int checkFall(Lvl *lvl, Player *player, MonsterList *currentMonster)
 {
    int x, y;

    if (currentMonster->monster.direction == LEFT)
    {
        x = (int) (currentMonster->monster.pos.x + currentMonster->monster.dirX) / TILE_SIZE;
        y = (int) (currentMonster->monster.pos.y + currentMonster->monster.pos.h - 1) /  TILE_SIZE;
        if (y < 0)
            y = 1;
        if (y > lvl->height)
            y = lvl->height;
        if (x < 0)
            x = 1;
        if (x > lvl->width)
            x = lvl->width;

        if (!lvl->solid[lvl->map[x][y + 1]])
            return 1;
        else
            return 0;
    }
    else
    {
        x = (int)(currentMonster->monster.pos.x + currentMonster->monster.pos.w + currentMonster->monster.dirX) / TILE_SIZE;
        y = (int)(currentMonster->monster.pos.y + currentMonster->monster.pos.h - 1) / TILE_SIZE;
        if (y <= 0)
            y = 1;
        if (y >= lvl->height)
            y = lvl->height - 1;
        if (x <= 0)
            x = 1;
        if (x >= lvl->width)
            x = lvl->width - 1;

        if (!lvl->solid[lvl->map[x][y + 1]])
            return 1;
        else
            return 0;
    }
}






void wait(unsigned long *time1, unsigned long *time2, int delay)
{
    *time2 = SDL_GetTicks();
    if(*time2 - *time1 < delay)
        SDL_Delay(delay - (*time2 - *time1));
    *time1 = SDL_GetTicks();
}


void centerScrollingOnPlayer(Lvl *lvl, Player *player)
{
    lvl->startX = player->pos.x - (WINDOW_W / 2) + player->pos.w / 2;

    if(lvl->startX < 0)
        lvl->startX = 0;
    if(lvl->startX + WINDOW_W >= lvl->maxX)
        lvl->startX = lvl->maxX - WINDOW_W;

    lvl->startY = player->pos.y - (WINDOW_H / 2) + player->pos.h / 2;

    if(lvl->startY < 0)
        lvl->startY = 0;
    if(lvl->startY + WINDOW_H >= lvl->maxY)
        lvl->startY = lvl->maxY - WINDOW_H;
 }


void mapCollisionPlayer(SDL_Renderer *renderer, Lvl *lvl, Player *player, Sounds *sounds, Input *in, Pictures *pictures, Fonts *fonts, Settings *settings)
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

                if(lvl->map[x2][y1] == INVICIBLE_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x2][y1] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x2][y2] == INVICIBLE_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x2][y2] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x2][y1] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    lvl->map[x2][y1]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                }

                if(lvl->map[x2][y2] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    lvl->map[x2][y2]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
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

                if(lvl->map[x1][y1] == INVICIBLE_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x1][y1] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x1][y2] == INVICIBLE_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x1][y2] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x1][y1] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    lvl->map[x1][y1]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                }

                if(lvl->map[x1][y2] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    lvl->map[x1][y2]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
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

                if(lvl->map[x1][y2] == INVICIBLE_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x1][y2] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x2][y2] == INVICIBLE_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x2][y2] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x1][y2] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    lvl->map[x1][y2]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                }

                if(lvl->map[x2][y2] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y2 * TILE_SIZE - player->pos.h;
                    lvl->map[x2][y2]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
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
                        lvl->moving_plat[j].is_player_on = 1;
                    }
                    else
                        lvl->moving_plat[j].is_player_on = 0;
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

                if(lvl->map[x1][y1] == INVICIBLE_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x1][y1] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x2][y1] == MONEY_NUM)
                {
                    player->invicibleFramesLeft = 360;
                    lvl->map[x2][y1] = 0;
                    Mix_PlayChannel(-1, sounds->invicible, 0);
                }

                if(lvl->map[x1][y1] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x1 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    lvl->map[x1][y1]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
                }

                if(lvl->map[x2][y1] == CHECKPOINT_NUM)
                {
                    player->isCheckpointActive = 1;
                    player->respawnX = x2 * TILE_SIZE;
                    player->respawnY = y1 * TILE_SIZE - player->pos.h;
                    lvl->map[x2][y1]++;
                    Mix_PlayChannel(-1, sounds->checkpoint, 0);
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
        death(player, sounds, in);

    if(player->pos.x + player->pos.w >= lvl->maxX)
    {
        int lvl_num = lvl->number + 1;

        if(lvl_num <= NUM_LEVEL)
        {
            SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_W, WINDOW_H, 32, SDL_PIXELFORMAT_ABGR8888);
            SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ABGR8888, surface->pixels, surface->pitch);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);

            levelFinished(renderer, sounds, fonts, texture);
            SDL_DestroyTexture(texture);

            freeLevel(lvl, PLAY);
            loadLevel(renderer, lvl_num, lvl, PLAY, settings);
            displayLevelName(renderer, pictures, fonts, lvl, in);
            player->isCheckpointActive = 0;
            respawn(player);
            removeAllMonsters(player->monsterList);
        }
        else
            player->pos.x = lvl->maxX - player->pos.w - 1;
    }
}





void mapCollisionMonster(Lvl *lvl, Player *player, MonsterList *currentMonster, int monsterIndex)
{
    int i, x1, x2, y1, y2;

    currentMonster->monster.on_ground = 0;

    if(currentMonster->monster.pos.h > TILE_SIZE)
        i = TILE_SIZE;
    else
        i = currentMonster->monster.pos.h;

    for (;;)
    {
        x1 = (currentMonster->monster.pos.x + currentMonster->monster.dirX) / TILE_SIZE;
        x2 = (currentMonster->monster.pos.x + currentMonster->monster.dirX + currentMonster->monster.pos.w - 1) / TILE_SIZE;

        y1 = (currentMonster->monster.pos.y) / TILE_SIZE;
        y2 = (currentMonster->monster.pos.y + i - 1) / TILE_SIZE;


        if (x1 >= 0 && x2 < lvl->width && y1 >= 0 && y2 < lvl->height)
        {
            if (currentMonster->monster.dirX > 0)
            {
                if(lvl->solid[lvl->map[x2][y1]] || lvl->solid[lvl->map[x2][y2]])
                {
                    currentMonster->monster.pos.x = x2 * TILE_SIZE;
                    currentMonster->monster.pos.x -= currentMonster->monster.pos.w + 1;
                    currentMonster->monster.dirX = 0;
                }
            }

            else if(currentMonster->monster.dirX < 0)
            {
                if (lvl->solid[lvl->map[x1][y1]] || lvl->solid[lvl->map[x1][y2]])
                {
                    currentMonster->monster.pos.x = (x1 + 1) * TILE_SIZE;
                    player->dirX = 0;
                }
            }
        }

        if(i == currentMonster->monster.pos.h)
            break;

        i += TILE_SIZE;

        if(i > currentMonster->monster.pos.h)
            i = currentMonster->monster.pos.h;
    }


    if(currentMonster->monster.pos.w > TILE_SIZE)
        i = TILE_SIZE;
    else
        i = currentMonster->monster.pos.w;


    for (;;)
    {
        x1 = (currentMonster->monster.pos.x) / TILE_SIZE;
        x2 = (currentMonster->monster.pos.x + i) / TILE_SIZE;

        y1 = (currentMonster->monster.pos.y + currentMonster->monster.dirY) / TILE_SIZE;
        y2 = (currentMonster->monster.pos.y + currentMonster->monster.dirY + currentMonster->monster.pos.h) / TILE_SIZE;

        if (x1 >= 0 && x2 < lvl->width && y1 >= 0 && y2 < lvl->height)
        {
            if(currentMonster->monster.dirY > 0)
            {
                if(lvl->solid[lvl->map[x1][y2]] || lvl->solid[lvl->map[x2][y2]])
                {
                    currentMonster->monster.pos.y = y2 * TILE_SIZE;
                    currentMonster->monster.pos.y -= currentMonster->monster.pos.h;
                    currentMonster->monster.dirY = 0;
                    currentMonster->monster.on_ground = 1;
                }
            }
            else if (player->dirY < 0)
            {
                if (lvl->solid[lvl->map[x1][y1]] || lvl->solid[lvl->map[x2][y1]])
                {
                    currentMonster->monster.pos.y = (y1 + 1) * TILE_SIZE;
                    player->dirY = 0;
                }
            }
        }

        if(i == currentMonster->monster.pos.w)
            break;

        i += TILE_SIZE;

        if(i > currentMonster->monster.pos.w)
            i = currentMonster->monster.pos.w;
    }


    currentMonster->monster.pos.x += currentMonster->monster.dirX;
    currentMonster->monster.pos.y += currentMonster->monster.dirY;


    if(currentMonster->monster.pos.x < 0)
        currentMonster->monster.pos.x = 0;

    if(currentMonster->monster.pos.y > lvl->maxY)
        removeMonsterFromIndex(player->monsterList, monsterIndex);

    if(currentMonster->monster.pos.x + currentMonster->monster.pos.w >= lvl->maxX)
        currentMonster->monster.pos.x = lvl->maxX - currentMonster->monster.pos.w - 1;
}






void death(Player *player, Sounds *sounds, Input *in)
{
    player->lifes--;
    player->dead = 1;
    player->frame_explosion = 0;
    Mix_PlayChannel(-1, sounds->death, 0);
    Mix_PlayChannel(-1, sounds->explosion, 0);
    if(in->controller.haptic != NULL)
        SDL_HapticRumblePlay(in->controller.haptic, 0.25, 500);
}


void createMonster(Player *player, const int x, const int y, const int lifes)
{
    MonsterList *monsterList = malloc(sizeof(MonsterList));
    initMonsterList(monsterList);

    monsterList->monster.direction = LEFT;
    monsterList->monster.pos.x = x;
    monsterList->monster.pos.y = y;
    monsterList->monster.pos.w = TILE_SIZE;
    monsterList->monster.pos.h = TILE_SIZE;
    monsterList->monster.on_ground = 0;
    monsterList->monster.frame_explosion = -1;
    monsterList->monster.lifes = lifes;

    insertMonster(player->monsterList, monsterList);
}


void createMovingPlat(Lvl *lvl, int x, int y)
{
    if(lvl->num_moving_plat < MAX_MOVING_PLAT)
    {
        lvl->moving_plat[lvl->num_moving_plat].pos.x = x;
        lvl->moving_plat[lvl->num_moving_plat].pos.y = y;
        lvl->moving_plat[lvl->num_moving_plat].pos.w = TILE_SIZE;
        lvl->moving_plat[lvl->num_moving_plat].pos.h = TILE_SIZE;

        lvl->moving_plat[lvl->num_moving_plat].beginX = x;
        lvl->moving_plat[lvl->num_moving_plat].beginY = y;
        lvl->moving_plat[lvl->num_moving_plat].is_player_on = 0;
        lvl->moving_plat[lvl->num_moving_plat].direction = RIGHT;
        lvl->moving_plat[lvl->num_moving_plat].endX = -1;
        lvl->moving_plat[lvl->num_moving_plat].endY = y;

        while(lvl->moving_plat[lvl->num_moving_plat].endX == -1)
        {
            x++;
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


