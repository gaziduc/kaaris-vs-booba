#include <stdio.h>
#include <stdlib.h>
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
#include "utils.h"

/* Etat de la tentative de connexion en cours (partage avec connectingThread) */
static SDL_mutex *connect_mutex = NULL;
static int connect_attempt = 0;     /* identifiant de la tentative courante */
static TCPsocket connect_socket = NULL;
static IPaddress connect_ip;
int status;
char error[256];


/* ------------------------------------------------------------------------- */
/*  Couche reseau                                                            */
/* ------------------------------------------------------------------------- */

static void putInt32(Uint8 *p, Sint32 v)
{
    Uint32 u = (Uint32) v;
    p[0] = (Uint8) (u >> 24);
    p[1] = (Uint8) (u >> 16);
    p[2] = (Uint8) (u >> 8);
    p[3] = (Uint8) u;
}

static Sint32 getInt32(const Uint8 *p)
{
    return (Sint32) (((Uint32) p[0] << 24) | ((Uint32) p[1] << 16) | ((Uint32) p[2] << 8) | (Uint32) p[3]);
}

/* Format fil : magic, protocole, flags, accept, lvl_num, x, y, state, frame, pseudo */
void netEncode(const Packet *packet, Uint8 *buf)
{
    memset(buf, 0, NET_PACKET_SIZE);

    buf[0] = NET_MAGIC;
    buf[1] = NET_PROTOCOL;
    buf[2] = (Uint8) ((packet->quit ? 1 : 0) | (packet->choosing ? 2 : 0));
    buf[3] = (Uint8) (Sint8) packet->accept;
    buf[4] = (Uint8) (Sint8) packet->lvl_num;
    putInt32(buf + 5, packet->point.x);
    putInt32(buf + 9, packet->point.y);
    buf[13] = (Uint8) packet->state;
    buf[14] = (Uint8) (packet->frame >> 8);
    buf[15] = (Uint8) packet->frame;
    memcpy(buf + 16, packet->nickname, NICKNAME_LEN - 1);
    buf[16 + NICKNAME_LEN - 1] = '\0';
}

int netDecode(const Uint8 *buf, Packet *packet)
{
    if(buf[0] != NET_MAGIC || buf[1] != NET_PROTOCOL)
        return 0;

    memset(packet, 0, sizeof(Packet));
    packet->quit = (buf[2] & 1) ? 1 : 0;
    packet->choosing = (buf[2] & 2) ? 1 : 0;
    packet->accept = (char) (Sint8) buf[3];
    packet->lvl_num = (char) (Sint8) buf[4];
    packet->point.x = getInt32(buf + 5);
    packet->point.y = getInt32(buf + 9);
    packet->state = buf[13];
    packet->frame = (buf[14] << 8) | buf[15];
    memcpy(packet->nickname, buf + 16, NICKNAME_LEN - 1);
    packet->nickname[NICKNAME_LEN - 1] = '\0';

    if(packet->state >= NUM_STATE)
        packet->state = IDLE_RIGHT;

    return 1;
}

void netPacketInit(Packet *packet, const char *nickname)
{
    memset(packet, 0, sizeof(Packet));
    packet->accept = -1;
    packet->lvl_num = -1;
    packet->state = IDLE_RIGHT;
    packet->point.x = SPAWN_X;
    packet->point.y = SPAWN_Y;

    if(nickname != NULL)
    {
        strncpy(packet->nickname, nickname, NICKNAME_LEN - 1);
        packet->nickname[NICKNAME_LEN - 1] = '\0';
    }
}

void netInit(Net *net)
{
    memset(net, 0, sizeof(Net));
    net->pending_level = -1;
    net->accept_state = -1;
}

static void netPublish(Net *net, const Packet *packet)
{
    SDL_LockMutex(net->mutex);

    net->got_packet = 1;
    strcpy(net->peer_nickname, packet->nickname);

    if(packet->accept == 1)
        net->accept_state = 1;
    else if(packet->accept == 0 && net->accept_state != 1)
        net->accept_state = 0;

    if(packet->choosing)
    {
        if(packet->quit)
            net->peer_quit = 1;
        else
            net->pending_level = packet->lvl_num;
    }
    else
    {
        net->latest = *packet;
        net->has_latest = 1;
    }

    SDL_UnlockMutex(net->mutex);
}

static int netReaderThread(void *data)
{
    Net *net = (Net*) data;
    Uint8 buf[NET_PACKET_SIZE];
    int have = 0;

    while(SDL_AtomicGet(&net->running))
    {
        int ready = SDLNet_CheckSockets(net->set, 50);

        if(ready < 0)
            break;
        if(ready == 0 || !SDLNet_SocketReady(net->client))
            continue;

        int received = SDLNet_TCP_Recv(net->client, buf + have, NET_PACKET_SIZE - have);

        if(received <= 0) // connexion fermee ou erreur
            break;

        have += received;

        while(have == NET_PACKET_SIZE)
        {
            Packet packet;

            if(netDecode(buf, &packet))
            {
                netPublish(net, &packet);
                have = 0;
            }
            else
            {
                // flux desynchronise : on jette un octet et on recommence
                memmove(buf, buf + 1, NET_PACKET_SIZE - 1);
                have--;
            }
        }
    }

    SDL_AtomicSet(&net->connected, 0);

    return 0;
}

int netStart(Net *net, TCPsocket client, int is_host)
{
    net->client = client;
    net->is_host = is_host;
    net->got_packet = 0;
    net->has_latest = 0;
    net->pending_level = -1;
    net->peer_quit = 0;
    net->accept_state = -1;
    net->peer_nickname[0] = '\0';
    net->mutex = SDL_CreateMutex();
    net->set = SDLNet_AllocSocketSet(1);

    if(net->mutex == NULL || net->set == NULL || SDLNet_TCP_AddSocket(net->set, client) < 0)
    {
        netStop(net);
        return 0;
    }

    SDL_AtomicSet(&net->running, 1);
    SDL_AtomicSet(&net->connected, 1);
    net->reader = SDL_CreateThread(netReaderThread, "netReader", net);

    if(net->reader == NULL)
    {
        netStop(net);
        return 0;
    }

    return 1;
}

void netStop(Net *net)
{
    SDL_AtomicSet(&net->running, 0);

    if(net->reader != NULL)
    {
        SDL_WaitThread(net->reader, NULL);
        net->reader = NULL;
    }

    if(net->set != NULL)
    {
        if(net->client != NULL)
            SDLNet_TCP_DelSocket(net->set, net->client);

        SDLNet_FreeSocketSet(net->set);
        net->set = NULL;
    }

    if(net->client != NULL)
    {
        SDLNet_TCP_Close(net->client);
        net->client = NULL;
    }

    if(net->mutex != NULL)
    {
        SDL_DestroyMutex(net->mutex);
        net->mutex = NULL;
    }

    SDL_AtomicSet(&net->connected, 0);
}

int netSend(Net *net, const Packet *packet)
{
    Uint8 buf[NET_PACKET_SIZE];

    if(net == NULL || net->client == NULL || !SDL_AtomicGet(&net->connected))
        return 0;

    netEncode(packet, buf);

    if(SDLNet_TCP_Send(net->client, buf, NET_PACKET_SIZE) < NET_PACKET_SIZE)
    {
        SDL_AtomicSet(&net->connected, 0);
        return 0;
    }

    return 1;
}

int netIsConnected(Net *net)
{
    return net != NULL && SDL_AtomicGet(&net->connected);
}

int netPollPosition(Net *net, Packet *packet)
{
    int ok;

    SDL_LockMutex(net->mutex);
    ok = net->has_latest;
    if(ok)
        *packet = net->latest;
    SDL_UnlockMutex(net->mutex);

    return ok;
}

int netTakeLevel(Net *net)
{
    int level;

    SDL_LockMutex(net->mutex);
    level = net->pending_level;
    net->pending_level = -1;
    SDL_UnlockMutex(net->mutex);

    return level;
}

int netPeerQuit(Net *net)
{
    int quit;

    SDL_LockMutex(net->mutex);
    quit = net->peer_quit;
    SDL_UnlockMutex(net->mutex);

    return quit;
}

int netAcceptState(Net *net)
{
    int state;

    SDL_LockMutex(net->mutex);
    state = net->accept_state;
    SDL_UnlockMutex(net->mutex);

    return state;
}

/* Attend le premier paquet de l'autre joueur (hello). Renvoie 0 en cas de timeout ou de deconnexion. */
int netWaitHello(Net *net, Uint32 timeout_ms, char *nickname)
{
    Uint32 start = SDL_GetTicks();

    while(SDL_GetTicks() - start < timeout_ms)
    {
        int got;

        SDL_LockMutex(net->mutex);
        got = net->got_packet;
        if(got)
            strcpy(nickname, net->peer_nickname);
        SDL_UnlockMutex(net->mutex);

        if(got)
            return 1;
        if(!SDL_AtomicGet(&net->connected))
            return 0;

        SDL_Delay(5);
    }

    return 0;
}

/* A appeler avant de lancer une partie : oublie les paquets d'une partie precedente (ex : quit=1 perime). */
void netResetGame(Net *net)
{
    SDL_LockMutex(net->mutex);
    net->has_latest = 0;
    net->pending_level = -1;
    SDL_UnlockMutex(net->mutex);
}

void createServer(SDL_Window *window, SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    unsigned long frame_num = 0;
    int escape = 0;
    char str[100] = "";
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];
    SDL_Texture **ip_text = NULL;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;
    IPaddress ip;
    IPaddress *local_ips;
    SDLNet_ResolveHost(&ip, NULL, NET_PORT);
    Net *net = xmalloc(sizeof(Net), window);
    netInit(net);
    net->server = SDLNet_TCP_Open(&ip);

    local_ips = xmalloc(sizeof(IPaddress) * MAX_IP_DISPLAYED, window);
    int num_ips = SDLNet_GetLocalAddresses(local_ips, MAX_IP_DISPLAYED);

    if(num_ips > 0)
    {
        ip_text = xmalloc(sizeof(SDL_Texture*) * num_ips, window);

        for(int i = 0; i < num_ips; i++)
        {
            unsigned int a, b, c, d;
            getIP(&local_ips[i], &a, &b, &c, &d);
            sprintf(str, "%d.%d.%d.%d", a, b, c, d);
            ip_text[i] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
        }
    }

    if(net->server != NULL)
        texture[0] = RenderTextBlended(renderer, fonts->ocraext_score, "En attente d'un joueur", white);
    else
    {
        // port deja utilise (autre instance du jeu ?) : on l'affiche au lieu d'attendre indefiniment
        SDL_Color red = {255, 80, 80, 255};
        texture[0] = RenderTextBlended(renderer, fonts->ocraext_score, "Port 3000 indisponible", red);
    }
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
        TCPsocket new_client = (net->server != NULL) ? SDLNet_TCP_Accept(net->server) : NULL;

        if(new_client != NULL)
        {
            char peer_nickname[NICKNAME_LEN] = "";
            IPaddress *peer_ip = SDLNet_TCP_GetPeerAddress(new_client);
            IPaddress peer_ip_copy;
            memset(&peer_ip_copy, 0, sizeof(peer_ip_copy));
            if(peer_ip != NULL)
                peer_ip_copy = *peer_ip;

            // le thread de lecture recupere le "hello" du joueur ; on n'attend plus indefiniment un client muet
            if(netStart(net, new_client, 1) && netWaitHello(net, 3000, peer_nickname))
            {
                Packet packet_to_send;
                netPacketInit(&packet_to_send, settings->nickname);

                if(acceptClient(renderer, fonts, sounds, pictures, fps, in, &peer_ip_copy, peer_nickname) && netIsConnected(net))
                {
                    packet_to_send.accept = 1;
                    netSend(net, &packet_to_send);

                    Mix_PlayChannel(-1, sounds->enter, 0);
                    selectMode(window, renderer, pictures, fonts, in, sounds, music, settings, 1, net, fps);
                }
                else
                {
                    packet_to_send.accept = 0;
                    packet_to_send.quit = 1;
                    netSend(net, &packet_to_send);
                    SDL_Delay(100); // laisse partir le paquet avant la fermeture
                }
            }

            netStop(net); // ferme aussi le socket client
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
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);

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

    if(net->server != NULL)
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



typedef struct
{
    int id;
    char host[MAX_IP_LEN];
} ConnectJob;

static void startConnect(const char *host)
{
    if(connect_mutex == NULL)
        connect_mutex = SDL_CreateMutex();

    ConnectJob *job = malloc(sizeof(ConnectJob));
    if(job == NULL)
        return;

    SDL_LockMutex(connect_mutex);
    job->id = ++connect_attempt;
    status = CONNECTING;
    SDL_UnlockMutex(connect_mutex);

    strncpy(job->host, host, MAX_IP_LEN - 1);
    job->host[MAX_IP_LEN - 1] = '\0';

    SDL_Thread *thread = SDL_CreateThread(connectingThread, "connectingThread", job);
    if(thread != NULL)
        SDL_DetachThread(thread);
    else
    {
        free(job);
        strcpy(error, "Impossible de lancer la connexion");
        status = ERROR_NET;
    }
}

static void cancelConnect(void)
{
    SDL_LockMutex(connect_mutex);
    connect_attempt++; // la tentative en cours sera ignoree (et son socket ferme) quand elle se terminera
    if(connect_socket != NULL)
    {
        SDLNet_TCP_Close(connect_socket);
        connect_socket = NULL;
    }
    status = USER_FOCUS;
    SDL_UnlockMutex(connect_mutex);
}

void connectToServer(SDL_Window *window, SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Settings *settings, FPSmanager *fps)
{
    int escape = 0, frame = 0;
    char str[MAX_IP_LEN] = "";
    SDL_Texture *texture[4];
    SDL_Rect pos_dst[4];
    SDL_Color white = {255, 255, 255, 255}, red = {255, 0, 0, 255};
    texture[3] = NULL;
    memset(pos_dst, 0, sizeof(pos_dst));
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;


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

    Connect *connect = xmalloc(sizeof(Connect), window);
    connect->ip = xmalloc(sizeof(IPaddress), window);
    connect->net = xmalloc(sizeof(Net), window);
    netInit(connect->net);

    if(connect_mutex == NULL)
        connect_mutex = SDL_CreateMutex();
    status = USER_FOCUS;

    strcpy(connect->nickname, settings->nickname);


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

            if(status == CONNECTING)
                cancelConnect(); // un hote injoignable ne bloque plus l'ecran pendant le timeout systeme
            else
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
                    startConnect(connect->str);
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


        if (status == CONNECTED)
        {
            SDL_LockMutex(connect_mutex);
            TCPsocket sock = connect_socket;
            connect_socket = NULL;
            *connect->ip = connect_ip;
            status = USER_FOCUS;
            SDL_UnlockMutex(connect_mutex);

            if(sock != NULL && netStart(connect->net, sock, 0))
            {
                Packet hello;
                netPacketInit(&hello, connect->nickname);
                netSend(connect->net, &hello);

                Mix_PlayChannel(-1, sounds->enter, 0);

                transition(renderer, pictures->title, 3, texture, pos_dst, ENTERING, 0, fps);
                waitingForServer(window, renderer, fonts, pictures, in, connect->ip, connect->net, sounds, settings, fps);
                transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 1, fps);

                netStop(connect->net);
            }
            else
            {
                strcpy(error, "Connexion perdue");
                status = ERROR_NET;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);

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

        if(texture[3] != NULL)
        {
            SDL_DestroyTexture(texture[3]); // evite aussi une double destruction apres la boucle
            texture[3] = NULL;
        }

        frame++;
        frame = frame % 60;
    }

    transition(renderer, pictures->title, 3, texture, pos_dst, EXITING, 0, fps);

    for(int i = 0; i < 3; i++)
        SDL_DestroyTexture(texture[i]);

    if(texture[3] != NULL)
        SDL_DestroyTexture(texture[3]);

    SDL_StopTextInput();
    cancelConnect();

    free(connect->ip);
    free(connect->net);
    free(connect);
}


void hostOrJoin(SDL_Window *window, SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps)
{
    unsigned long frame_num = 0;
    int escape = 0, selected = HOST;
    char str[100] = "";
    SDL_Texture *texture[NUM_OPTIONS_NET];
    SDL_Rect pos_dst[NUM_OPTIONS_NET];
    SDL_Color white = {255, 255, 255, 255}, green = {0, 255, 0, 255};
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;
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
                createServer(window, renderer, pictures, fonts, in, sounds, music, settings, fps);
            else if(selected == JOIN)
                connectToServer(window, renderer, pictures, fonts, in, sounds, settings, fps);

            transition(renderer, pictures->title, NUM_OPTIONS_NET, texture, pos_dst, EXITING, 1, fps);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);

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


void waitingForServer(SDL_Window *window, SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, IPaddress *ip, Net *net, Sounds *sounds, Settings *settings, FPSmanager *fps)
{
    int escape = 0, accepted = 0;
    char str[128] = "";
    unsigned long frame_num = 0;
    SDL_Color white = {255, 255, 255, 255}, green = {0, 255, 0, 255};
    SDL_Texture *texture[3];
    SDL_Rect pos_dst[3];
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;

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

    char host_nickname[NICKNAME_LEN] = "";
    netResetGame(net);

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


        // tout vient du thread de lecture : plus de Recv concurrents ni de variables globales partagees
        int accept_state = netAcceptState(net);
        int level_num = netTakeLevel(net);

        SDL_LockMutex(net->mutex);
        strcpy(host_nickname, net->peer_nickname);
        SDL_UnlockMutex(net->mutex);

        if(accept_state == 1)
            accepted = 1;

        if(netPeerQuit(net) || !netIsConnected(net))
            escape = 1; // l'hote a quitte, a refuse, ou la connexion est tombee
        else if(level_num == 0)
        {
            transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0, fps);
            playGame(window, renderer, in, pictures, fonts, sounds, settings, 1, ALL_LEVELS, 1, net, fps);

            netResetGame(net);
            transition(renderer, pictures->title, 2, texture, pos_dst, EXITING, 1, fps);
        }
        else if(level_num > 0 && level_num <= NUM_LEVEL)
        {
            transition(renderer, pictures->title, 2, texture, pos_dst, ENTERING, 0, fps);
            playGame(window, renderer, in, pictures, fonts, sounds, settings, level_num, ONE_LEVEL, 1, net, fps);

            netResetGame(net);
            transition(renderer, pictures->title, 2, texture, pos_dst, EXITING, 1, fps);
        }

        SDL_DestroyTexture(texture[0]);
        sprintf(str, "En attente de l'hôte %d.%d.%d.%d%s%s", a, b, c, d, host_nickname[0] != '\0' ? " -> " : "", host_nickname);
        texture[0] = RenderTextBlended(renderer, fonts->ocraext_message, str, white);
        SDL_QueryTexture(texture[0], NULL, NULL, &pos_dst[0].w, &pos_dst[0].h);
        pos_dst[0].x = WINDOW_W / 2 - pos_dst[0].w / 2;

        if(accepted && accept_state != 0)
        {
            SDL_DestroyTexture(texture[1]);
            texture[1] = RenderTextBlended(renderer, fonts->ocraext_message, "L'hôte a accepté la demande !", green);
            SDL_QueryTexture(texture[1], NULL, NULL, &pos_dst[1].w, &pos_dst[1].h);
            pos_dst[1].x = WINDOW_W / 2 - pos_dst[1].w / 2;
        }


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);
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

SDL_Texture* setErrorTexture(SDL_Renderer *renderer, Fonts *fonts, SDL_Rect *pos_dst, char *error)
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
    SDL_Rect pos_fs;
    pos_fs.x = 0;
    pos_fs.y = 0;
    pos_fs.w = WINDOW_W;
    pos_fs.h = WINDOW_H;

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
        SDL_RenderCopy(renderer, pictures->title, NULL, &pos_fs);

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



int connectingThread(void *data)
{
    ConnectJob *job = (ConnectJob*) data;
    IPaddress ip;
    TCPsocket sock = NULL;
    char message[256] = "";

    if(SDLNet_ResolveHost(&ip, job->host, NET_PORT) == 0)
    {
        sock = SDLNet_TCP_Open(&ip); // bloquant jusqu'au timeout systeme : d'ou le thread dedie

        if(sock == NULL)
            snprintf(message, sizeof(message), "Connexion impossible (%s)", SDLNet_GetError());
    }
    else
        snprintf(message, sizeof(message), "Adresse introuvable (%s)", SDLNet_GetError());

    SDL_LockMutex(connect_mutex);

    if(job->id == connect_attempt) // sinon la tentative a ete annulee entre-temps
    {
        if(sock != NULL)
        {
            connect_socket = sock;
            connect_ip = ip;
            status = CONNECTED;
        }
        else
        {
            strncpy(error, message, sizeof(error) - 1);
            error[sizeof(error) - 1] = '\0';
            status = ERROR_NET;
        }
    }
    else if(sock != NULL)
        SDLNet_TCP_Close(sock);

    SDL_UnlockMutex(connect_mutex);

    free(job);

    return 0;
}
