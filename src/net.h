#ifndef NET_H
#define NET_H

    #include <SDL2/SDL_net.h>
    #include <SDL2/SDL_thread.h>
    #include <SDL2/SDL_atomic.h>
    #include "data.h"

    #define MAX_IP_LEN          16
    #define MAX_IP_DISPLAYED    20
    #define NET_PORT            3000

    /* Taille fixe d'un paquet sur le reseau (voir netEncode / netDecode) */
    #define NET_PACKET_SIZE     (16 + NICKNAME_LEN)
    #define NET_MAGIC           0xB7
    #define NET_PROTOCOL        2

    enum {TITLE_NET, HOST, JOIN, NUM_OPTIONS_NET};
    enum {USER_FOCUS, CONNECTING, CONNECTED, ERROR_NET};
    enum {REQUEST1, REQUEST2, ACCEPT, DECLINE, NUM_TEXT_REQUEST};

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

    /*
     * Connexion TCP entre les deux joueurs.
     * Un thread unique (netReaderThread) lit le socket, reassemble les paquets
     * (TCP est un flux : un Recv peut renvoyer un paquet partiel) et publie
     * le resultat sous mutex. Le thread principal ne fait jamais de Recv.
     */
    typedef struct
    {
        TCPsocket server;
        TCPsocket client;

        SDLNet_SocketSet set;
        SDL_Thread *reader;
        SDL_mutex *mutex;
        SDL_atomic_t running;
        SDL_atomic_t connected;

        /* Donnees publiees par le thread de lecture (protegees par mutex) */
        int is_host;
        int got_packet;         /* au moins un paquet recu (hello)            */
        Packet latest;          /* dernier paquet de position recu            */
        int has_latest;
        int pending_level;      /* niveau choisi par l'autre joueur, -1 sinon */
        int peer_quit;          /* l'autre joueur a quitte le menu            */
        int accept_state;       /* -1 inconnu, 0 refuse, 1 accepte            */
        char peer_nickname[NICKNAME_LEN];
    } Net;

    typedef struct
    {
        Net *net;
        IPaddress *ip;
        char str[MAX_IP_LEN];
        char nickname[NICKNAME_LEN];
    } Connect;

    void createServer(SDL_Window *window, SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps);
    void connectToServer(SDL_Window *window, SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Settings *settings, FPSmanager *fps);
    void hostOrJoin(SDL_Window *window, SDL_Renderer *renderer, Pictures *pictures, Fonts *fonts, Input *in, Sounds *sounds, Mix_Music **music, Settings *settings, FPSmanager *fps);
    void waitingForServer(SDL_Window *window, SDL_Renderer *renderer, Fonts *fonts, Pictures *pictures, Input *in, IPaddress *ip, Net *net, Sounds *sounds, Settings *settings, FPSmanager *fps);
    int isIP(char *str);
    int isNullIP(char *str);
    SDL_Texture* setErrorTexture(SDL_Renderer *renderer, Fonts *fonts, SDL_Rect *pos_dst, char *error);
    int connectingThread(void *data);
    int acceptClient(SDL_Renderer *renderer, Fonts *fonts, Sounds *sounds, Pictures *pictures, FPSmanager *fps, Input *in, IPaddress *ip, char *nickname);
    void getIP(IPaddress *ip, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d);

    /* --- Couche reseau --- */
    void netInit(Net *net);
    int  netStart(Net *net, TCPsocket client, int is_host);
    void netStop(Net *net);
    void netPacketInit(Packet *packet, const char *nickname);
    int  netSend(Net *net, const Packet *packet);
    int  netIsConnected(Net *net);
    int  netPollPosition(Net *net, Packet *packet);
    int  netTakeLevel(Net *net);
    int  netPeerQuit(Net *net);
    int  netAcceptState(Net *net);
    int  netWaitHello(Net *net, Uint32 timeout_ms, char *nickname);
    void netResetGame(Net *net);
    void netEncode(const Packet *packet, Uint8 *buf);
    int  netDecode(const Uint8 *buf, Packet *packet);

#endif // NET_H
