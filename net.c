#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "data.h"
#include "game.h"
#include "net.h"
#include "transition.h"
#include "file.h"

char level_num;
char quit;
char nickname[NICKNAME_LEN];
int status;
char error[256];
char accept_con;

void createServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    unsigned long frame_num = 0;
    int escape = 0;
    char str[100] = "";
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];
    SDL_Texture **ip_text = NULL;
    SDL_Color white = {255, 255, 255, 255};

    IPaddress ip;
    IPaddress *local_ips;
    SDLNet_ResolveHost(&ip, NULL, 3000);
    Net *net = malloc(sizeof(Net));
    net->server = SDLNet_TCP_Open(&ip);

    local_ips = malloc(sizeof(IPaddress) * MAX_IP_DISPLAYED);
    if(local_ips == NULL)
        exit(EXIT_FAILURE);

    int num_ips = SDLNet_GetLocalAddresses(local_ips, MAX_IP_DISPLAYED);

    if(num_ips > 0)
    {
        ip_text = malloc(sizeof(SDL_Texture*) * num_ips);

        for(int i = 0; i < num_ips; i++)
        {
            unsigned int a, b, c, d;
            getIP(&local_ips[i], &a, &b, &c, &d);
            sprintf(str, "%d.%d.%d.%d", a, b, c, d);
            ip_text[i] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
        }
    }

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_score, "En attente d'un joueur", white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_score, ".", white);
    texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, "Adresse(s) IP locale(s) :", white);

    for(int i = 0; i < 3; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        if(i < 2)
            pos_dst[i].y = 100 + i * 50;
        else
            pos_dst[i].y = 350;
    }

    transition(renderer, pictures->title, 3, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        net->client = SDLNet_TCP_Accept(net->server);
        if(net->client != NULL)
        {
            IPaddress *ip = SDLNet_TCP_GetPeerAddress(net->client);

            Packet packet;
            SDLNet_TCP_Recv(net->client, &packet, sizeof(Packet));

            if(acceptClient(renderer, fonts, sounds, pictures, fps, in, ip, packet.nickname))
            {
                Packet packet_to_send;
                packet_to_send.accept = 1;
                packet_to_send.choosing = 0;
                packet_to_send.frame = 0;
                packet_to_send.lvl_num = -1;
                packet_to_send.nickname[0] = '\0';
                packet_to_send.point.x = 0;
                packet_to_send.point.y = 0;
                packet_to_send.quit = 0;
                packet_to_send.state = 0;
                SDLNet_TCP_Send(net->client, &packet_to_send, sizeof(Packet));

                Mix_PlayChannel(-1, sounds->enter, 0);
                selectMode(renderer, pictures, fonts, in, sounds, music, settings, 1, net, fps);
            }
            else
            {
                Packet packet_to_send;
                packet_to_send.accept = 0;
                packet_to_send.quit = 1;
                SDLNet_TCP_Send(net->client, &packet_to_send, sizeof(Packet));
            }

            SDLNet_TCP_Close(net->client);
            net->client = NULL;
        }

        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        if(frame_num % 60 < 20)
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2 - 20;
        else if(frame_num % 60 < 40)
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
        else
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2 + 20;

        for(int i = 0; i < 3; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_Rect pos_dest;
        pos_dest.y = 440;

        for(int i = 0; i < num_ips; i++)
        {
            if(local_ips[i].host != 0) // si c'est pas 0.0.0.0 alors on l'affiche
            {
                SDL_QueryTexture(ip_text[i], NULL, NULL, &pos_dest.w, &pos_dest.h);
                pos_dest.x = WINDOW_W / 2 - pos_dest.w / 2;
                SDL_RenderCopy(renderer, ip_text[i], NULL, &pos_dest);

                pos_dest.y += 40;
            }
        }

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    SDLNet_TCP_Close(net->server);

    transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(texture[i]);

    for(int i = 0; i < num_ips; i++)
        SDL_DestroyTexture(ip_text[i]);

    if(num_ips > 0)
        free(ip_text);

    free(local_ips);

    free(net);
}



void connectToServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Settings *settings, FPSmanager *fps)
{
    int escape = 0, frame = 0;
    char str[MAX_IP_LEN] = "";
    SDL_Texture *texture[4];
    SDL_Rect pos_dst[4];
    SDL_Color white = {255, 255, 255, 255}, red = {255, 0, 0, 255};


    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Entrez l'adresse IP de l'hôte :", white);
    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = WINDOW_H / 2 - 150 - pos_dst[0].h / 2;

    texture[1] = RenderTextBlended(renderer, fonts->ocraext_score, "|", white);
    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = WINDOW_H / 2 + 65 - pos_dst[1].h / 2 + 2;

    texture[2] = RenderTextBlended(renderer, fonts->ocraext_commands, "(F2 pour coller depuis le presse-papier)", white);
    SDL_QueryTexture(texture[2], NULL, NULL, &pos_dst[2].w, &pos_dst[2].h);
    pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2;
    pos_dst[2].y = WINDOW_H / 2 - 60 - pos_dst[2].h / 2;

    transition(renderer, pictures->title, 3, texture, pos_dst, ENTERING, 1, fps);

    SDL_StartTextInput();

    Connect *connect = malloc(sizeof(Connect));
    connect->ip = malloc(sizeof(IPaddress));
    connect->net = malloc(sizeof(Net));
    strcpy(connect->nickname, settings->nickname);


    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if((status == USER_FOCUS || status == ERROR_NET) && (KEY_ESCAPE))
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }
        if((status == USER_FOCUS || status == ERROR_NET) && (in->key[SDL_SCANCODE_RETURN] || in->key[SDL_SCANCODE_KP_ENTER]))
        {
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;

            if(str[0] != '\0')
            {
                if(!isNullIP(str))
                {
                    strcpy(connect->str, str);
                    status = CONNECTING;
                    SDL_CreateThread(connectingThread, "connectingThread", connect);
                }
                else
                {
                    strcpy(error, "IP non valide");
                    status = ERROR_NET;
                }
            }
        }
        if((status == USER_FOCUS || status == ERROR_NET) && in->key[SDL_SCANCODE_BACKSPACE])
        {
            in->key[SDL_SCANCODE_BACKSPACE] = 0;

            int i = 0;
            while(str[i] != '\0')
                i++;

            if(i > 0)
                str[i - 1] = '\0';
        }
        if((status == USER_FOCUS || status == ERROR_NET) && in->key[SDL_SCANCODE_F2])
        {
            in->key[SDL_SCANCODE_F2] = 0;

            char *clipboard = SDL_GetClipboardText();
            if(clipboard != NULL)
            {
                if(isIP(clipboard))
                    strncat(str, clipboard, MAX_IP_LEN - strlen(str) - 1);

                SDL_free(clipboard);
            }
        }

        if(isIP(in->text))
            strncat(str, in->text, MAX_IP_LEN - strlen(str) - 1);

        if(str[0] != '\0')
        {
            texture[3] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
            SDL_QueryTexture(texture[3], NULL, NULL, &pos_dst[3].w, &pos_dst[3].h);
            pos_dst[3].x = WINDOW_W / 2 - pos_dst[3].w / 2;
            pos_dst[3].y = WINDOW_H / 2 + 65 - pos_dst[3].h / 2;

            pos_dst[1].x = WINDOW_W / 2 + pos_dst[3].w / 2 - pos_dst[1].w / 2 + 2;
        }
        else
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2 + 2;


        if(status == CONNECTED)
        {
            SDL_StopTextInput();
            Mix_PlayChannel(-1, sounds->enter, 0);

            transition(renderer, pictures->title, 3, texture, pos_dst, ENTERING, 0, fps);
            waitingForServer(renderer, fonts, pictures, in, connect->ip, connect->net, sounds, settings, fps);
            transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 1, fps);

            SDLNet_TCP_Close(connect->net->client);
            SDL_StartTextInput();

            status = USER_FOCUS;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        if(status == CONNECTING)
        {
            SDL_Rect pos;
            SDL_Texture *point = RenderTextBlended(renderer, fonts->ocraext_message, ".", white);
            SDL_QueryTexture(point, NULL, NULL, &pos.w, &pos.h);
            pos.x = WINDOW_W / 2 - pos.w / 2 - 20 + (frame / 20) * 20;
            pos.y = pos_dst[1].y + 55;
            SDL_RenderCopy(renderer, point, NULL, &pos);
            SDL_DestroyTexture(point);
        }
        else if(status == ERROR_NET)
        {
            SDL_Rect pos;
            SDL_Texture *text = RenderTextBlended(renderer, fonts->ocraext_message, error, red);
            SDL_QueryTexture(text, NULL, NULL, &pos.w, &pos.h);
            pos.x = WINDOW_W / 2 - pos.w / 2;
            pos.y = WINDOW_H / 2 + 160 - pos.h / 2;
            SDL_RenderCopy(renderer, text, NULL, &pos);
            SDL_DestroyTexture(text);
        }

        for(int i = 0; i < 4; i++)
            if(i == 0 || (i == 1 && frame < 30 && (status == USER_FOCUS || status == ERROR_NET)) || i == 2 || (i == 3 && str[0] != '\0'))
                SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        if(str[0] != '\0')
            SDL_DestroyTexture(texture[3]);

        frame++;
        frame = frame % 60;
    }

    transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(texture[i]);

    if(texture[3] != NULL)
        SDL_DestroyTexture(texture[3]);

    SDL_StopTextInput();
    status = USER_FOCUS;

    free(connect->ip);
    free(connect->net);
    free(connect);
}


void hostOrJoin(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    unsigned long frame_num = 0;
    int escape = 0, selected = HOST;
    char str[100] = "";
    SDL_Texture *texture[NUM_OPTIONS_NET];
    SDL_Rect pos_dst[NUM_OPTIONS_NET];
    SDL_Color white = {255, 255, 255, 255}, green = {0, 255, 0, 255};

    sprintf(str, "Content de vous revoir, %s", settings->nickname);
    char temp = str[0];

    texture[TITLE_NET] = RenderTextBlended(renderer, fonts->ocraext_message, "_", green);
    texture[HOST] = RenderTextBlended(renderer, fonts->ocraext_message, "Accueillir une partie", white);
    texture[JOIN] = RenderTextBlended(renderer, fonts->ocraext_message, "Joindre une partie", white);

    for(int i = 0; i < NUM_OPTIONS_NET; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        if(i == TITLE_NET)
            pos_dst[i].y = 200;
        else
            pos_dst[i].y = 310 + i * 80;
    }


    transition(renderer, pictures->title, NUM_OPTIONS_NET, texture, pos_dst, ENTERING, 1, fps);


    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }
        if(KEY_UP_MENU)
        {
            in->key[SDL_SCANCODE_UP] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected > HOST)
            {
                selected--;
                Mix_PlayChannel(-1, sounds->select, 0);
            }
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected < JOIN)
            {
                selected++;
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


            Mix_PlayChannel(-1, sounds->enter, 0);

            transition(renderer, pictures->title, NUM_OPTIONS_NET, texture, pos_dst, ENTERING, 0, fps);

            if(selected == HOST)
                createServer(renderer, pictures, fonts, in, sounds, music, settings, fps);
            else if(selected == JOIN)
                connectToServer(renderer, pictures, fonts, in, sounds, settings, fps);

            transition(renderer, pictures->title, NUM_OPTIONS_NET, texture, pos_dst, EXITING, 1, fps);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        if(frame_num <= strlen(str))
        {
            temp = str[frame_num];
            str[frame_num] = '\0';

            SDL_DestroyTexture(texture[TITLE_NET]);
            texture[TITLE_NET] = RenderTextBlended(renderer, fonts->ocraext_message, str, green);
            SDL_QueryTexture(texture[TITLE_NET], NULL, NULL, &pos_dst[TITLE_NET].w, &pos_dst[TITLE_NET].h);
            pos_dst[TITLE_NET].x = WINDOW_W / 2 - pos_dst[TITLE_NET].w / 2;

            str[frame_num] = temp;
        }

        for(int i = 0; i < NUM_OPTIONS_NET; i++)
        {
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);
            if(selected == i)
            {
                SDL_Rect pos_arrow;
                SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_arrow.w, &pos_arrow.h);
                pos_arrow.x = pos_dst[i].x - pos_arrow.w - 40 + (frame_num % 60 < 30 ? frame_num % 30 : 30 - frame_num % 30);
                pos_arrow.y = pos_dst[i].y + pos_dst[i].h / 2 - pos_arrow.h / 2;
                SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_arrow);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    transition(renderer, pictures->title, NUM_OPTIONS_NET, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < NUM_OPTIONS_NET; i++)
        SDL_DestroyTexture(texture[i]);
}


void sendPos(Net *net, Packet *packet)
{
    SDLNet_TCP_Send(net->client, packet, sizeof(Packet));
}


void receivePos(Net *net, Packet *packet)
{
    SDLNet_TCP_Recv(net->client, packet, sizeof(Packet));
}


void waitingForServer(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, IPaddress *ip, Net *net, Sounds *sounds, Settings *settings, FPSmanager *fps)
{
    int escape = 0, accepted = 0;
    char str[128] = "";
    unsigned long frame_num = 0;
    SDL_Color white = {255, 255, 255, 255}, green = {0, 255, 0, 255};
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];

    unsigned int a, b, c, d;
    getIP(ip, &a, &b, &c, &d);
    sprintf(str, "En attente de l'hôte %d.%d.%d.%d", a, b, c, d);
    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, "L'hôte doit accepter la demande...", white);
    texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, ".", white);

    for(int i = 0; i < 3; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = WINDOW_H / 2 - 80 + i * 80;
    }

    level_num = -1;
    nickname[0] = '\0';
    SDL_CreateThread(waitingThread, "waitingThread", net);

    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_ESCAPE)
        {
            in->key[SDL_SCANCODE_ESCAPE] = 0;
            in->controller[0].buttons[6] = 0;
            in->controller[1].buttons[6] = 0;
            escape = 1;
        }

        if(frame_num % 60 < 20)
            pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2 - 20;
        else if(frame_num % 60 < 40)
            pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2;
        else
            pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2 + 20;


        if(quit)
            escape = 1;
        else if(level_num == 0)
        {
            transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0, fps);
            playGame(renderer, in, pictures, fonts, sounds, settings, 1, ALL_LEVELS, 1, net, fps);

            SDL_CreateThread(waitingThread, "waitingThread", net);
            transition(renderer, pictures->title, 2, texture, pos_dst, EXITING, 1, fps);
        }
        else if(level_num > 0 && level_num <= NUM_LEVEL)
        {
            transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0, fps);
            playGame(renderer, in, pictures, fonts, sounds, settings, level_num, ONE_LEVEL, 1, net, fps);


            SDL_CreateThread(waitingThread, "waitingThread", net);
            transition(renderer, pictures->title, 2, texture, pos_dst, EXITING, 1, fps);
        }

        SDL_DestroyTexture(texture[0]);
        sprintf(str, "En attente de l'hôte %d.%d.%d.%d%s%s", a, b, c, d, nickname[0] != '\0' ? " -> " : "", nickname);
        texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
        SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
        pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;

        if(accept_con == 1)
        {
            SDL_DestroyTexture(texture[1]);
            texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, "L'hôte a accepté la demande !", green);
            SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
            accepted = 1;
        }
        else if(accept_con == 0 && !accepted)
        {
            escape = 1;
            accept_con = -1;
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);
        for(int i = 0; i < 3; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(texture[i]);
}


int isIP(char *str)
{
    for(size_t i = 0; i < strlen(str); i++)
        if((str[i] < '0' || str[i] > '9') && str[i] != '.')
            return 0;

    return 1;
}

int isNullIP(char *str)
{
    for(size_t i = 0; i < strlen(str); i++)
        if(str[i] != '0' && str[i] != '.')
            return 0;

    return 1;
}

SDL_Texture* setErrorTexture(SDL_Renderer *renderer, Fonts *fonts, SDL_Rect *pos_dst, const char *error)
{
    SDL_Color red = {255, 0, 0, 255};

    SDL_Texture *texture = RenderTextBlended(renderer, fonts->ocraext_commands, error, red);
    SDL_QueryTexture(texture, NULL, NULL, &pos_dst->w, &pos_dst->h);
    pos_dst->x = WINDOW_W / 2 - pos_dst->w / 2;

    return texture;
}


int acceptClient(SDL_Renderer *renderer, Fonts *fonts, Sounds *sounds, Pictures *pictures, FPSmanager *fps, Input *in, IPaddress *ip, char *nickname)
{
    SDL_Color white = {255, 255, 255, 255};
    int selected = ACCEPT;
    int escape = 0;
    char str[256] = "";
    SDL_Texture *texture[NUM_TEXT_REQUEST];
    SDL_Rect pos_dst[NUM_TEXT_REQUEST];
    unsigned long frame_num = 0;

    unsigned int a, b, c, d;
    getIP(ip, &a, &b, &c, &d);
    sprintf(str, "%s à l'adresse %d.%d.%d.%d", nickname, a, b, c, d);
    texture[REQUEST1] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
    texture[REQUEST2] = RenderTextBlended(renderer, fonts->ocraext_message, "veut jouer avec vous.", white);
    texture[ACCEPT] = RenderTextBlended(renderer, fonts->ocraext_message, "Accepter", white);
    texture[DECLINE] = RenderTextBlended(renderer, fonts->ocraext_message, "Refuser", white);

    for(int i = 0; i < NUM_TEXT_REQUEST; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;

        if(i < ACCEPT)
            pos_dst[i].y = 220 + i * 60;
        else
            pos_dst[i].y = 390 + (i - ACCEPT) * 70;
    }



    while(!escape)
    {
        updateEvents(in);

        if(in->quit)
            exit(EXIT_SUCCESS);
        if(KEY_UP_MENU)
        {
            in->key[SDL_SCANCODE_UP] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected > ACCEPT)
            {
                selected--;
                Mix_PlayChannel(-1, sounds->select, 0);
            }
        }
        if(KEY_DOWN_MENU)
        {
            in->key[SDL_SCANCODE_DOWN] = 0;
            in->controller[0].hat[0] = SDL_HAT_CENTERED;
            in->controller[0].axes[1] = 0;
            in->controller[1].hat[0] = SDL_HAT_CENTERED;
            in->controller[1].axes[1] = 0;

            if(selected < DECLINE)
            {
                selected++;
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

            escape = 1;
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        roundedBoxRGBA(renderer, WINDOW_W / 8, WINDOW_H / 4, (WINDOW_W / 8) * 7, (WINDOW_H / 4) * 3, 10, 64, 64, 64, 192);
        roundedRectangleRGBA(renderer, WINDOW_W / 8, WINDOW_H / 4, (WINDOW_W / 8) * 7, (WINDOW_H / 4) * 3, 10, 255, 255, 255, 192);

        for(int i = 0; i < NUM_TEXT_REQUEST; i++)
        {
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);
            if(selected == i)
            {
                SDL_Rect pos_arrow;
                SDL_QueryTexture(pictures->HUDlife, NULL, NULL, &pos_arrow.w, &pos_arrow.h);
                pos_arrow.x = pos_dst[i].x - pos_arrow.w - 40 + (frame_num % 60 < 30 ? frame_num % 30 : 30 - frame_num % 30);
                pos_arrow.y = pos_dst[i].y + pos_dst[i].h / 2 - pos_arrow.h / 2;
                SDL_RenderCopy(renderer, pictures->HUDlife, NULL, &pos_arrow);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    for(int i = 0; i < NUM_TEXT_REQUEST; i++)
        SDL_DestroyTexture(texture[i]);

    if(selected == ACCEPT)
        return 1;

    return 0;
}


void getIP(IPaddress *ip, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d)
{
    *a = ip->host % 256;
    *b = (ip->host % 65536 - *a) / 256;
    *c = (ip->host % 16777216 - (*a + *b)) / 65536;
    *d = (ip->host % 4294967296 - (*a + *b + *c)) / 16777216;
}



int waitingThread(void *data)
{
    Packet packet;
    packet.choosing = 0;
    accept_con = -1;
    quit = 0;
    level_num = -1;

    while(!packet.choosing)
    {
        SDLNet_TCP_Recv(((Net*) data)->client, &packet, sizeof(Packet));
        strcpy(nickname, packet.nickname);
        accept_con = packet.accept;

        SDL_Delay(10);
    }

    level_num = packet.lvl_num;
    quit = packet.quit;

    return 0;
}


int connectingThread(void *data)
{
    if(SDLNet_ResolveHost(((Connect*) data)->ip, ((Connect*) data)->str, 3000) == 0) // Success
    {
        ((Connect*) data)->net->client = SDLNet_TCP_Open(((Connect*) data)->ip);

        if(((Connect*) data)->net->client != NULL)
        {
            status = CONNECTED;

            Packet packet;
            packet.choosing = 0;
            packet.quit = 0;
            packet.lvl_num = -1;
            strcpy(packet.nickname, ((Connect*) data)->nickname);
            packet.point.x = 0;
            packet.point.y = 0;
            packet.frame = 0;
            packet.state = 0;
            SDLNet_TCP_Send(((Connect*) data)->net->client, &packet, sizeof(Packet));
        }
        else
        {
            strcpy(error, SDLNet_GetError());
            status = ERROR_NET;
        }
    }
    else
    {
        strcpy(error, SDLNet_GetError());
        status = ERROR_NET;
    }


    return 0;
}

