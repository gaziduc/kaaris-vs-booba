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
                strcat(in->text, event.text.text);
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
                if(event.jdevice.which == 0)
                    initController(in);
                break;
            case SDL_JOYBUTTONDOWN:
                in->controller.buttons[event.jbutton.button] = 1;
                break;
            case SDL_JOYBUTTONUP:
                in->controller.buttons[event.jbutton.button] = 0;
                break;
            case SDL_JOYAXISMOTION:
                in->controller.axes[event.jaxis.axis] = event.jaxis.value;
                break;
            case SDL_JOYHATMOTION:
                in->controller.hat[event.jhat.hat] = event.jhat.value;
                break;
            case SDL_JOYDEVICEREMOVED:
                if(event.jdevice.which == 0)
                    freeController(in);
                break;
            default:
                break;
		}
	}
}




void initController(Input *in)
{
    in->controller.joystick = SDL_JoystickOpen(0);

    if(in->controller.joystick != NULL)
    {
        in->controller.haptic = SDL_HapticOpenFromJoystick(in->controller.joystick);

        if(in->controller.haptic != NULL)
            SDL_HapticRumbleInit(in->controller.haptic);
    }
}



void freeController(Input *in)
{
    if(in->controller.haptic != NULL)
    {
        SDL_HapticClose(in->controller.haptic);
        in->controller.haptic = NULL;
    }

    SDL_JoystickClose(in->controller.joystick);
    in->controller.joystick = NULL;
}

