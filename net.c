#include <stdio.h>

#ifdef WIN64
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
#endif // WIN64

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_net.h>
#include "data.h"
#include "game.h"
#include "net.h"
#include "transition.h"
#include "file.h"

int level_num;


void createServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    unsigned long frame_num = 0;
    int escape = 0;
    char my_ip[50] = "";
    char str[100] = "";
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];
    SDL_Color white = {255, 255, 255};

    IPaddress ip;
    SDLNet_ResolveHost(&ip, NULL, 3000);
    Net *net = malloc(sizeof(Net));
    net->server = SDLNet_TCP_Open(&ip);

    #ifdef WIN64
        PMIB_IPADDRTABLE pIPAddrTable = (MIB_IPADDRTABLE *) malloc(sizeof(MIB_IPADDRTABLE));
        DWORD dwSize = 0;
        GetIpAddrTable(pIPAddrTable, &dwSize, 0);
        pIPAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
        GetIpAddrTable(pIPAddrTable, &dwSize, 0);

        for(int i = 0; i < pIPAddrTable->dwNumEntries; i++)
        {
            IN_ADDR IPAddr;
            IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwAddr;
            strcpy(my_ip, inet_ntoa(IPAddr));
        }

        free(pIPAddrTable);
    #else
        FILE *f = popen("/bin/hostname -I", "r");
        if(f == NULL)
            strcpy(my_ip, "Inconnue");
        else
        {
            fgets(my_ip, sizeof(my_ip) - 1, f);
            pclose(f);

            my_ip[sizeof(my_ip) - 1] = '\0';

            int i;
            for(i = 0; i < sizeof(my_ip) - 1 && my_ip[i] != ' '; i++);
            if(my_ip[i] == ' ')
                my_ip[i] = '\0';
        }
    #endif

    sprintf(str, "Votre adresse IP : %s", my_ip);
    texture[0] = RenderTextBlended(renderer, fonts->ocraext_score, "En attente d'un joueur", white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_score, ".", white);
    texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);

    for(int i = 0; i < 3; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        if(i < 2)
            pos_dst[i].y = 240 + i * 50;
        else
            pos_dst[i].y = 420;
    }

    transition(renderer, pictures->title, 3, texture, pos_dst, ENTERING, 1, fps);

    while(!escape)
    {
        net->client = SDLNet_TCP_Accept(net->server);
        if(net->client != NULL)
        {
            Mix_PlayChannel(-1, sounds->enter, 0);
            selectMode(renderer, pictures, fonts, in, sounds, music, settings, 1, net, fps);
            SDLNet_TCP_Close(net->client);
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

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    SDLNet_TCP_Close(net->server);

    transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(texture[i]);

    free(net);
}



void connectToServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    int escape = 0, frame = 0;
    char str[MAX_IP_LEN] = "";
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];
    SDL_Color white = {255, 255, 255};

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Entrez l'adresse IP de l'hote :", white);
    SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
    pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;
    pos_dst[0].y = WINDOW_H / 2 - 50 - pos_dst[0].h / 2;

    texture[1] = RenderTextBlended(renderer, fonts->ocraext_score, "|", white);
    SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
    pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
    pos_dst[1].y = WINDOW_H / 2 + 50 - pos_dst[1].h / 2 + 2;

    transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 1, fps);

    SDL_StartTextInput();

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
        if(in->key[SDL_SCANCODE_RETURN] || in->key[SDL_SCANCODE_KP_ENTER])
        {
            in->key[SDL_SCANCODE_RETURN] = 0;
            in->key[SDL_SCANCODE_KP_ENTER] = 0;

            IPaddress ip;
            if(str[0] != '\0' && SDLNet_ResolveHost(&ip, str, 3000) == 0) // Success
            {
                Net *net = malloc(sizeof(Net));
                net->client = SDLNet_TCP_Open(&ip);
                SDL_StopTextInput();

                Mix_PlayChannel(-1, sounds->enter, 0);

                transition(renderer, pictures->title, 3, texture, pos_dst, ENTERING, 0, fps);
                waitingForServer(renderer, fonts, pictures, in, net, sounds, music, settings, fps);
                transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 1, fps);

                SDLNet_TCP_Close(net->client);
                SDL_StartTextInput();
            }


        }
        if(in->key[SDL_SCANCODE_BACKSPACE])
        {
            in->key[SDL_SCANCODE_BACKSPACE] = 0;

            int i = 0;
            while(str[i] != '\0')
                i++;

            if(i > 0)
                str[i - 1] = '\0';
        }

        if(strlen(str) + strlen(in->text) + 1 <= MAX_IP_LEN)
            strcat(str, in->text);

        if(str[0] != '\0')
        {
            texture[2] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
            SDL_QueryTexture(texture[2], NULL, NULL, &pos_dst[2].w, &pos_dst[2].h);
            pos_dst[2].x = WINDOW_W / 2 - pos_dst[2].w / 2;
            pos_dst[2].y = WINDOW_H / 2 + 50 - pos_dst[2].h / 2;

            pos_dst[1].x = WINDOW_W / 2 + pos_dst[2].w / 2 - pos_dst[1].w / 2 + 2;
        }
        else
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2 + 2;

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);

        for(int i = 0; i < 3; i++)
            if(i == 0 || (i == 2 && str[0] != '\0') || (i == 1 && frame < 30))
                SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        if(str[0] != '\0')
            SDL_DestroyTexture(texture[2]);


        frame++;
        frame = frame % 60;
    }

    transition(renderer, pictures->title, 2, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 2; i++)
        SDL_DestroyTexture(texture[i]);

    SDL_StopTextInput();
}


void hostOrJoin(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    unsigned long frame_num = 0;
    int escape = 0, selected = HOST;
    SDL_Texture *texture[NUM_OPTIONS_NET];
    SDL_Rect pos_dst[NUM_OPTIONS_NET];
    SDL_Color white = {255, 255, 255};

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "Accueillir une partie", white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, "Joindre une partie", white);

    for(int i = 0; i < NUM_OPTIONS_NET; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = 300 + i * 80;
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
                connectToServer(renderer, pictures, fonts, in, sounds, music, settings, fps);

            transition(renderer, pictures->title, NUM_OPTIONS_NET, texture, pos_dst, EXITING, 1, fps);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);
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


void waitingForServer(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, Net *net, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    int escape = 0;
    SDL_Color white = {255, 255, 255};
    unsigned long frame_num = 0;
    SDL_Texture *texture[2];
    SDL_Rect pos_dst[2];

    texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, "En attente de l'hote", white);
    texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, ".", white);

    for(int i = 0; i < 2; i++)
    {
        SDL_QueryTexture(texture[i], NULL, NULL, &pos_dst[i].w, &pos_dst[i].h);
        pos_dst[i].x = WINDOW_W / 2 - pos_dst[i].w / 2;
        pos_dst[i].y = WINDOW_H / 2 - 50 + i * 80;
    }

    level_num = -2;
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
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2 - 20;
        else if(frame_num % 60 < 40)
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
        else
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2 + 20;


        if(level_num == -1)
            escape = 1;
        else if(level_num == 0)
        {
            transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0, fps);
            playGame(renderer, in, pictures, fonts, sounds, settings, 1, ALL_LEVELS, 1, net, fps);
            transition(renderer, pictures->title, 2, texture, pos_dst, EXITING, 1, fps);
            level_num = -2;
            SDL_CreateThread(waitingThread, "waitingThread", net);
        }
        else if(level_num > 0 && level_num <= NUM_LEVEL)
        {
            transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0, fps);
            playGame(renderer, in, pictures, fonts, sounds, settings, level_num, ONE_LEVEL, 1, net, fps);
            transition(renderer, pictures->title, 2, texture, pos_dst, EXITING, 1, fps);
            level_num = -2;
            SDL_CreateThread(waitingThread, "waitingThread", net);
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, NULL);
        for(int i = 0; i < 2; i++)
            SDL_RenderCopy(renderer, texture[i], NULL, &pos_dst[i]);

        SDL_RenderPresent(renderer);
        SDL_framerateDelay(fps);

        frame_num++;
    }

    for(int i = 0; i < 2; i++)
        SDL_DestroyTexture(texture[i]);
}


int waitingThread(void *data)
{
    ChoosePacket packet;
    SDLNet_TCP_Recv(((Net*) data)->client, &packet, sizeof(ChoosePacket));
    level_num = packet.level_num;
    return 0;
}

