
#ifndef EVENT_H
#define EVENT_H

    #define DEAD_ZONE       8000
    #define KEY_ESCAPE      in->key[SDL_SCANCODE_ESCAPE] || in->controller.buttons[6]
    #define KEY_UP_MENU     (in->key[SDL_SCANCODE_UP] && !in->repeat) || in->controller.hat[0] == SDL_HAT_UP
    #define KEY_DOWN_MENU   (in->key[SDL_SCANCODE_DOWN] && !in->repeat) || in->controller.hat[0] == SDL_HAT_DOWN
    #define KEY_ENTER_MENU  ((in->key[SDL_SCANCODE_SPACE] || in->key[SDL_SCANCODE_RETURN] || in->key[SDL_SCANCODE_KP_ENTER]) && !in->repeat) || in->controller.buttons[0]
    #define KEY_UP_GAME     (in->key[SDL_SCANCODE_UP] && !in->repeat) || in->controller.buttons[0]
    #define KEY_LEFT_GAME   in->key[SDL_SCANCODE_LEFT] || (in->controller.hat[0] == SDL_HAT_LEFT || in->controller.hat[0] == SDL_HAT_LEFTUP || in->controller.hat[0] == SDL_HAT_LEFTDOWN || in->controller.axes[0] < -DEAD_ZONE)
    #define KEY_RIGHT_GAME  in->key[SDL_SCANCODE_RIGHT] || (in->controller.hat[0] == SDL_HAT_RIGHT || in->controller.hat[0] == SDL_HAT_RIGHTUP || in->controller.hat[0] == SDL_HAT_RIGHTDOWN || in->controller.axes[0] > DEAD_ZONE)

    typedef struct
    {
        SDL_Joystick *joystick;
        SDL_Haptic *haptic;
        int buttons[13];
        int axes[5];
        int hat[1];
    } Controller;

    typedef struct
    {
        char key[SDL_NUM_SCANCODES];
        char quit;
        char repeat;
        char mousebutton[5];
        int mouseX, mouseY;
        int wheelY;
        char text[4];
        Controller controller;
    } Input;

    void updateEvents(Input* in);
    void initController(Input *in);
    void freeController(Input *in);

#endif // EVENT_H
