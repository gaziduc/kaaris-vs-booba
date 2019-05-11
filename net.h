#ifndef NET_H
#define NET_H

    #include <SDL2/SDL_net.h>
    #include "data.h"

    #define MAX_IP_LEN  16

    enum {HOST, JOIN, NUM_OPTIONS_NET};

    typedef struct
    {
        TCPsocket server;
        TCPsocket client;
    } Net;

    typedef struct
    {
        SDL_Point point;
        int state;
        int frame;
    } Packet;

    typedef struct
    {
        int level_num;
    } ChoosePacket;

    void createServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings);
    void connectToServer(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings);
    void hostOrJoin(SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings);
    void sendPos(Net *net, Packet *packet);
    void receivePos(Net *net, Packet *packet);
    void waitingForServer(SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, Net *net, Sounds *sounds, Mix_Music **music, Settings *settings);
    int waitingThread(void *data);

#endif // NET_H
