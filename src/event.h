
#ifndef EVENT_H
#define EVENT_H

    #define DEAD_ZONE           20000
    #define KEY_PAUSE           in->key[SDL_SCANCODE_P] || in->key[SDL_SCANCODE_ESCAPE] || in->controller[0].buttons[7] || in->controller[1].buttons[6]
    #define KEY_ESCAPE          in->key[SDL_SCANCODE_ESCAPE] || in->controller[0].buttons[6] || in->controller[1].buttons[6]
    #define KEY_UP_MENU         (in->key[SDL_SCANCODE_UP] && !in->repeat) || in->controller[0].hat[0] == SDL_HAT_UP || in->controller[1].hat[0] == SDL_HAT_UP || (in->controller[0].axes[1] < -DEAD_ZONE && in->controller[0].is_axes_centered[1]) || (in->controller[1].axes[1] < -DEAD_ZONE && in->controller[1].is_axes_centered[1])
    #define KEY_DOWN_MENU       (in->key[SDL_SCANCODE_DOWN] && !in->repeat) || in->controller[0].hat[0] == SDL_HAT_DOWN || in->controller[1].hat[0] == SDL_HAT_DOWN || (in->controller[0].axes[1] > DEAD_ZONE && in->controller[0].is_axes_centered[1]) || (in->controller[1].axes[1] > DEAD_ZONE && in->controller[1].is_axes_centered[1])
    #define KEY_ENTER_MENU      ((in->key[SDL_SCANCODE_SPACE] || in->key[SDL_SCANCODE_RETURN] || in->key[SDL_SCANCODE_KP_ENTER]) && !in->repeat) || in->controller[0].buttons[0] || in->controller[1].buttons[0]
    #define KEY_UP_GAME         ((in->key[settings->controls[i].jump] || (num_player == 1 && in->key[settings->controls[i + 1].jump])) && !in->repeat) || in->controller[i].buttons[0]
    #define KEY_LEFT_GAME       in->key[settings->controls[i].left] || (num_player == 1 && in->key[settings->controls[i + 1].left]) || (in->controller[i].hat[0] == SDL_HAT_LEFT || in->controller[i].hat[0] == SDL_HAT_LEFTUP || in->controller[i].hat[0] == SDL_HAT_LEFTDOWN || in->controller[i].axes[0] < -DEAD_ZONE)
    #define KEY_RIGHT_GAME      in->key[settings->controls[i].right] || (num_player == 1 && in->key[settings->controls[i + 1].right]) || (in->controller[i].hat[0] == SDL_HAT_RIGHT || in->controller[i].hat[0] == SDL_HAT_RIGHTUP || in->controller[i].hat[0] == SDL_HAT_RIGHTDOWN || in->controller[i].axes[0] > DEAD_ZONE)
    #define KEY_POWER_UP_GAME   (in->key[settings->controls[i].power_up] || (num_player == 1 && in->key[settings->controls[i + 1].power_up])) || in->controller[i].buttons[2]
    #define KEY_LEFT_MENU       (in->key[SDL_SCANCODE_LEFT] && !in->repeat) || in->controller[0].hat[0] == SDL_HAT_LEFT || in->controller[1].hat[0] == SDL_HAT_LEFT || (in->controller[0].axes[0] < -DEAD_ZONE && in->controller[0].is_axes_centered[0]) || (in->controller[1].axes[0] < -DEAD_ZONE && in->controller[1].is_axes_centered[0])
    #define KEY_RIGHT_MENU      (in->key[SDL_SCANCODE_RIGHT] && !in->repeat) || in->controller[0].hat[0] == SDL_HAT_RIGHT || in->controller[1].hat[0] == SDL_HAT_RIGHT || (in->controller[0].axes[0] > DEAD_ZONE && in->controller[0].is_axes_centered[0]) || (in->controller[1].axes[0] > DEAD_ZONE && in->controller[1].is_axes_centered[0])
    #define KEY_UP_EDITOR       in->key[SDL_SCANCODE_UP] || in->controller[0].hat[0] == SDL_HAT_UP || in->controller[1].hat[0] == SDL_HAT_UP || in->controller[0].axes[1] < -DEAD_ZONE || in->controller[1].axes[1] < -DEAD_ZONE
    #define KEY_DOWN_EDITOR     in->key[SDL_SCANCODE_DOWN] || in->controller[0].hat[0] == SDL_HAT_DOWN || in->controller[1].hat[0] == SDL_HAT_DOWN || in->controller[0].axes[1] > DEAD_ZONE || in->controller[1].axes[1] > DEAD_ZONE
    #define KEY_LEFT_EDITOR     in->key[SDL_SCANCODE_LEFT] || in->controller[0].hat[0] == SDL_HAT_LEFT || in->controller[1].hat[0] == SDL_HAT_LEFT || in->controller[0].axes[0] < -DEAD_ZONE || in->controller[1].axes[0] < -DEAD_ZONE
    #define KEY_RIGHT_EDITOR    in->key[SDL_SCANCODE_RIGHT] || in->controller[0].hat[0] == SDL_HAT_RIGHT || in->controller[1].hat[0] == SDL_HAT_RIGHT || in->controller[0].axes[0] > DEAD_ZONE || in->controller[1].axes[0] > DEAD_ZONE

    typedef struct
    {
        SDL_Joystick *joystick;
        SDL_Haptic *haptic;
        int buttons[13];
        int axes[5];
        int is_axes_centered[5];
        int was_axes_centered[5];
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
        char text[8];
        Controller controller[2];
        int controller_num[2];
        int num_controller;
    } Input;

    void updateEvents(Input* in);
    void initController(Input *in, int index);
    void freeController(Input *in, int index);
    int getKey();

#endif // EVENT_H
