#include <string.h>
#include <SDL2/SDL.h>
#include "event.h"

void updateEvents(Input* in)
{
    in->quit = 0;
    in->wheelY = 0;
    memset(in->text, '\0', sizeof(in->text));
    SDL_Event event;

	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
		    case SDL_QUIT:
                in->quit = 1;
                break;
            case SDL_TEXTINPUT:
                strncat(in->text, event.text.text, sizeof(in->text) - 1);
                break;
            case SDL_KEYDOWN:
                in->key[event.key.keysym.scancode] = 1;
                in->repeat = event.key.repeat;
                break;
            case SDL_KEYUP:
                in->key[event.key.keysym.scancode] = 0;
                break;
            case SDL_MOUSEBUTTONDOWN:
                in->mousebutton[event.button.button] = 1;
                break;
            case SDL_MOUSEBUTTONUP:
                in->mousebutton[event.button.button] = 0;
                break;
            case SDL_MOUSEMOTION:
                in->mouseX = event.motion.x;
                in->mouseY = event.motion.y;
                break;
            case SDL_MOUSEWHEEL:
                in->wheelY = event.wheel.y;
                break;
            case SDL_JOYDEVICEADDED:
                if(in->num_controller < 2)
                {
                    freeController(in, in->num_controller);
                    in->num_controller++;
                    initController(in, in->num_controller);
                }
                break;
            case SDL_JOYBUTTONDOWN:
                for(int i = 0; i < in->num_controller; i++)
                    if(in->controller_num[i] == event.jdevice.which)
                        in->controller[i].buttons[event.jbutton.button] = 1;
                break;
            case SDL_JOYBUTTONUP:
                for(int i = 0; i < in->num_controller; i++)
                    if(in->controller_num[i] == event.jdevice.which)
                        in->controller[i].buttons[event.jbutton.button] = 0;
                break;
            case SDL_JOYAXISMOTION:
                for(int i = 0; i < in->num_controller; i++)
                    if(in->controller_num[i] == event.jdevice.which)
                    {
                        in->controller[i].axes[event.jaxis.axis] = event.jaxis.value;

                        if(in->controller[i].was_axes_centered[event.jaxis.axis] && (in->controller[i].axes[event.jaxis.axis] < -DEAD_ZONE || in->controller[i].axes[event.jaxis.axis] > DEAD_ZONE))
                            in->controller[i].was_axes_centered[event.jaxis.axis] = 0;
                        else if(in->controller[i].axes[event.jaxis.axis] < -DEAD_ZONE || in->controller[i].axes[event.jaxis.axis] > DEAD_ZONE)
                            in->controller[i].is_axes_centered[event.jaxis.axis] = 0;
                        else
                        {
                            in->controller[i].is_axes_centered[event.jaxis.axis] = 1;
                            in->controller[i].was_axes_centered[event.jaxis.axis] = 1;
                        }
                    }
                break;
            case SDL_JOYHATMOTION:
                for(int i = 0; i < in->num_controller; i++)
                    if(in->controller_num[i] == event.jdevice.which)
                        in->controller[i].hat[event.jhat.hat] = event.jhat.value;
                break;
            case SDL_JOYDEVICEREMOVED:
                for(int i = 0; i < in->num_controller; i++)
                    if(in->controller_num[i] == event.jdevice.which)
                    {
                        freeController(in, in->num_controller);
                        in->num_controller--;
                        initController(in, in->num_controller);
                    }

                break;
            default:
                break;
		}
	}
}


int getKey()
{
    SDL_Event event;
    int escape = 0;
    int key;

    while(SDL_PollEvent(&event) || !escape)
	{
	    switch(event.type)
	    {
	        case SDL_KEYDOWN:
                key = event.key.keysym.scancode;
                escape = 1;
                break;
	    }
	}

	return key;
}




void initController(Input *in, int num_controller)
{
    for(int i = 0; i < num_controller; i++)
    {
        in->controller[i].joystick = SDL_JoystickOpen(i);

        if(in->controller[i].joystick != NULL)
        {
            in->controller_num[i] = SDL_JoystickInstanceID(in->controller[i].joystick);

            in->controller[i].haptic = SDL_HapticOpenFromJoystick(in->controller[i].joystick);

            if(in->controller[i].haptic != NULL)
                SDL_HapticRumbleInit(in->controller[i].haptic);
        }
    }
}



void freeController(Input *in, int num_controller)
{
    for(int i = 0; i < num_controller; i++)
    {
        if(in->controller[i].haptic != NULL)
        {
            SDL_HapticClose(in->controller[i].haptic);
            in->controller[i].haptic = NULL;
        }

        SDL_JoystickClose(in->controller[i].joystick);
        in->controller[i].joystick = NULL;
    }
}

