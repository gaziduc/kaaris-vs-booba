#ifndef NET_H
#define NET_H

    #include <SDL2/SDL_net.h>
    #include "data.h"

    #define MAX_IP_LEN          16
    #define MAX_IP_DISPLAYED    20

    enum {TITLE_NET, HOST, JOIN, NUM_OPTIONS_NET};
    enum {USER_FOCUS, CONNECTING, CONNECTED, ERROR_NET};
    enum {REQUEST1, REQUEST2, ACCEPT, DECLINE, NUM_TEXT_REQUEST};

    typedef struct
    {
        TCPsocket server;
        TCPsocket client;
    } Net;

    typedef struct
    {
        Net *net;
        IPaddress *ip;
        char str[MAX_IP_LEN];
        char nickname[NICKNAME_LEN];
    } Connect;

    typedef struct
    {
        SDL_Point point;
        int state;
        int frame;
        char quit;
        char lvl_num;
        char choosing;
        char nickname[NICKNAME_LEN];
        char accept;
    } Packet;

    void createServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps);
    void connectToServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Settings *settings, FPSmanager *fps);
    void hostOrJoin(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps);
    void sendPos(Net *net, Packet *packet);
    void receivePos(Net *net, Packet *packet);
    void waitingForServer(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, IPaddress *ip, Net *net, Sounds *sounds, Settings *settings, FPSmanager *fps);
    int isIP(char *str);
    int isNullIP(char *str);
    SDL_Texture* setErrorTexture(SDL_Renderer *renderer, Fonts *fonts, SDL_Rect *pos_dst, char *error);
    int waitingThread(void *data);
    int connectingThread(void *data);
    int acceptClient(SDL_Renderer *renderer, Fonts *fonts, Sounds *sounds, Pictures *pictures, FPSmanager *fps, Input *in, IPaddress *ip, char *nickname);
    void getIP(IPaddress *ip, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d);

#endif // NET_H
