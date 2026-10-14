#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "fx.h"

#define MAX_PARTICLES 768

enum {P_DOT, P_STAR, P_RING, P_HEART, P_SMOKE};

typedef struct
{
    int type, life, maxlife;
    float x, y, vx, vy, grav, drag, size, grow;
    Uint8 r, g, b;
} Particle;

typedef struct
{
    int money, lifes, power, dead, ground, invincible, init, step;
    float dirY;
    float squash;   /* >0 : ecrase (atterrissage), <0 : etire (saut) */
} PlayerFx;

static Particle particles[MAX_PARTICLES];
static int num_particles = 0;
static float shake_amount = 0;
static int shake_dx = 0, shake_dy = 0;
static SDL_Texture *vignette = NULL;
static PlayerFx pfx[2];

static float frand(float a, float b)
{
    return a + (b - a) * ((float) rand() / (float) RAND_MAX);
}

static Particle* add(int type, float x, float y, float vx, float vy, int life, float size, int r, int g, int b)
{
    if(num_particles >= MAX_PARTICLES)
        return NULL;

    Particle *p = &particles[num_particles++];
    p->type = type; p->x = x; p->y = y; p->vx = vx; p->vy = vy;
    p->life = p->maxlife = life; p->size = size; p->grow = 0;
    p->grav = 0; p->drag = 1.0f;
    p->r = r; p->g = g; p->b = b;
    return p;
}

void fxInit(SDL_Renderer *renderer)
{
    fxClear();
    memset(pfx, 0, sizeof(pfx));

    if(vignette == NULL)
    {
        const int w = 256, h = 144;
        SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);

        if(s != NULL)
        {
            Uint32 *px = (Uint32*) s->pixels;
            for(int y = 0; y < h; y++)
                for(int x = 0; x < w; x++)
                {
                    float dx = (x - w / 2.0f) / (w / 2.0f), dy = (y - h / 2.0f) / (h / 2.0f);
                    float d = sqrtf(dx * dx + dy * dy) / 1.414f;
                    float a = d > 0.55f ? (d - 0.55f) / 0.45f : 0;
                    if(a > 1) a = 1;
                    px[y * (s->pitch / 4) + x] = ((Uint32) (a * a * 120) << 24);
                }

            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // lissage pour l'agrandissement
            vignette = SDL_CreateTextureFromSurface(renderer, s);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
            if(vignette != NULL)
                SDL_SetTextureBlendMode(vignette, SDL_BLENDMODE_BLEND);
            SDL_FreeSurface(s);
        }
    }
}

void fxQuit(void)
{
    if(vignette != NULL)
    {
        SDL_DestroyTexture(vignette);
        vignette = NULL;
    }
    fxClear();
}

void fxClear(void)
{
    num_particles = 0;
    shake_amount = 0;
}

void fxUpdate(void)
{
    for(int i = 0; i < num_particles; i++)
    {
        Particle *p = &particles[i];
        p->vy += p->grav;
        p->vx *= p->drag;
        p->vy *= p->drag;
        p->x += p->vx;
        p->y += p->vy;
        p->size += p->grow;
        p->life--;

        if(p->life <= 0)
        {
            particles[i--] = particles[--num_particles];
        }
    }

    if(shake_amount > 0.3f)
    {
        shake_dx = (int) frand(-shake_amount, shake_amount);
        shake_dy = (int) frand(-shake_amount, shake_amount);
        shake_amount *= 0.86f;
    }
    else
    {
        shake_amount = 0;
        shake_dx = shake_dy = 0;
    }

    for(int i = 0; i < 2; i++)
        pfx[i].squash *= 0.80f;
}

void fxDust(float x, float y, int dir, int n)
{
    for(int i = 0; i < n; i++)
    {
        Particle *p = add(P_SMOKE, x + frand(-4, 4), y - frand(0, 3), frand(-1.2f, 1.2f) - dir * 0.6f, frand(-0.7f, -0.1f), (int) frand(16, 28), frand(2.5f, 4.5f), 235, 232, 225);
        if(p) { p->grow = 0.18f; p->drag = 0.94f; }
    }
}

void fxBurst(float x, float y, int n, int r, int g, int b)
{
    for(int i = 0; i < n; i++)
    {
        float a = frand(0, 6.2832f), s = frand(1.5f, 6.0f);
        Particle *p = add(P_DOT, x, y, cosf(a) * s, sinf(a) * s - 1.0f, (int) frand(18, 40), frand(2.0f, 4.5f), r, g + (int) frand(0, 80) > 255 ? 255 : g + (int) frand(0, 80), b);
        if(p) { p->grav = 0.22f; p->drag = 0.96f; p->grow = -0.05f; }
    }

    for(int i = 0; i < n / 4; i++)
    {
        Particle *p = add(P_SMOKE, x + frand(-8, 8), y + frand(-8, 8), frand(-1, 1), frand(-1.5f, -0.2f), (int) frand(25, 45), frand(5, 9), 90, 90, 96);
        if(p) { p->grow = 0.25f; p->drag = 0.95f; }
    }

    fxRing(x, y, r, g, b);
}

void fxSparkle(float x, float y, int n)
{
    for(int i = 0; i < n; i++)
    {
        float a = frand(0, 6.2832f), s = frand(0.8f, 3.2f);
        Particle *p = add(P_STAR, x, y, cosf(a) * s, sinf(a) * s - 1.2f, (int) frand(22, 40), frand(3, 5.5f), 255, (int) frand(190, 235), (int) frand(40, 120));
        if(p) { p->grav = 0.08f; p->drag = 0.96f; }
    }
}

void fxHearts(float x, float y)
{
    for(int i = 0; i < 5; i++)
    {
        Particle *p = add(P_HEART, x + frand(-10, 10), y + frand(-6, 6), frand(-0.5f, 0.5f), frand(-2.0f, -0.8f), (int) frand(40, 60), frand(4, 6), 255, 80, 110);
        if(p) p->drag = 0.98f;
    }
    fxSparkle(x, y, 6);
}

void fxRing(float x, float y, int r, int g, int b)
{
    Particle *p = add(P_RING, x, y, 0, 0, 22, 4, r, g, b);
    if(p) p->grow = 1.6f;
}

void fxFlame(float x, float y)
{
    for(int i = 0; i < 2; i++)
    {
        Particle *p = add(P_DOT, x + frand(-3, 3), y, frand(-0.6f, 0.6f), frand(1.5f, 3.2f), (int) frand(10, 18), frand(3, 5), 255, (int) frand(120, 210), 40);
        if(p) { p->grow = -0.2f; p->drag = 0.97f; }
    }
    Particle *s = add(P_SMOKE, x, y + 4, frand(-0.4f, 0.4f), frand(0.5f, 1.2f), 22, 3, 120, 120, 128);
    if(s) s->grow = 0.2f;
}

void fxShake(float amount)
{
    if(!FX_SHAKE)
        return;

    if(amount > shake_amount)
        shake_amount = amount;
}

void fxPlayerEvents(int idx, Player *pl)
{
    if(idx < 0 || idx > 1)
        return;

    PlayerFx *f = &pfx[idx];
    float fx_x = pl->pos.x + PLAYER_W / 2.0f, feet = pl->pos.y + PLAYER_H;
    int on_ground = pl->on_ground;

    if(!f->init)
    {
        f->money = pl->money; f->lifes = pl->lifes; f->power = pl->power_up;
        f->dead = pl->dead; f->ground = on_ground; f->dirY = pl->dirY; f->invincible = pl->invicibleFramesLeft;
        f->init = 1;
        return;
    }

    f->step++;

    if(pl->dead)
    {
        if(!f->dead)
        {
            fxBurst(fx_x, pl->pos.y + PLAYER_H / 2.0f, 46, 255, 120, 30);
            fxShake(11);
        }
    }
    else
    {
        if(pl->money > f->money)
            fxSparkle(fx_x, pl->pos.y + 30, 10);
        if(pl->lifes > f->lifes)
            fxHearts(fx_x, pl->pos.y + 20);
        if(pl->power_up != NO_POWER_UP && pl->power_up != f->power)
        {
            fxRing(fx_x, pl->pos.y + 40, 120, 220, 255);
            fxSparkle(fx_x, pl->pos.y + 40, 14);
        }

        if(f->ground && !on_ground && pl->dirY < 0)
        {
            fxDust(fx_x, feet, 0, 7);
            f->squash = -0.28f;
        }
        else if(!f->ground && !on_ground && pl->dirY <= -JUMP_HEIGHT + 1 && f->dirY > -JUMP_HEIGHT + 1 && pl->power_up != JETPACK)
        {
            fxRing(fx_x, feet - 6, 255, 255, 255);   // double saut
            fxDust(fx_x, feet, 0, 5);
            f->squash = -0.22f;
        }

        if(!f->ground && on_ground && f->dirY > 2.5f)
        {
            fxDust(fx_x, feet, -1, 5);
            fxDust(fx_x, feet, 1, 5);
            f->squash = f->dirY >= MAX_FALL_SPEED - 0.5f ? 0.34f : 0.2f;
            if(f->dirY >= MAX_FALL_SPEED - 0.5f)
                fxShake(3);
        }

        if(on_ground && (pl->state == WALK_LEFT || pl->state == WALK_RIGHT) && f->step % 6 == 0)
            fxDust(fx_x + (pl->state == WALK_LEFT ? 8 : -8), feet, pl->state == WALK_LEFT ? -1 : 1, 1);

        if(pl->invicibleFramesLeft > 0 && f->step % 2 == 0)
        {
            Particle *p = add(P_STAR, pl->pos.x + frand(0, PLAYER_W), pl->pos.y + frand(4, PLAYER_H), 0, frand(-0.8f, -0.2f), 24, frand(2.5f, 4.5f), (int) frand(120, 255), (int) frand(120, 255), (int) frand(120, 255));
            if(p) p->drag = 0.97f;
        }

        if(pl->power_up == JETPACK && pl->dirY < f->dirY - 0.4f)
            fxFlame(fx_x, feet - 8);
    }

    f->money = pl->money; f->lifes = pl->lifes; f->power = pl->power_up;
    f->dead = pl->dead; f->ground = on_ground; f->dirY = pl->dirY; f->invincible = pl->invicibleFramesLeft;
}

void fxPlayerScale(int idx, float *sx, float *sy)
{
    *sx = *sy = 1.0f;

    if(idx < 0 || idx > 1)
        return;

    float s = pfx[idx].squash;   /* >0 ecrase, <0 etire */
    *sy = 1.0f - s;
    *sx = 1.0f + s * 0.9f;
}

void fxShadow(SDL_Renderer *renderer, Lvl *lvl, Player *pl, int view, int num_players)
{
    int tx = (pl->pos.x + PLAYER_W / 2) / TILE_SIZE;
    int ty = (pl->pos.y + PLAYER_H) / TILE_SIZE;

    if(tx < 0 || tx >= lvl->width)
        return;

    for(int k = 0; k < 8 && ty + k < lvl->height; k++)
    {
        if(ty + k >= 0 && lvl->solid[(int) lvl->map[tx][ty + k]])
        {
            int ground = (ty + k) * TILE_SIZE;
            int dist = ground - (pl->pos.y + PLAYER_H);
            if(dist < 0 || dist > 200)
                return;

            int sx = pl->pos.x + PLAYER_W / 2 - lvl->startX[view];
            int sy = ground - lvl->startY[view] + (int) (WINDOW_H / num_players) * view;
            int w = 13 - dist / 22;
            if(w < 4) w = 4;
            filledEllipseRGBA(renderer, sx, sy - 1, w, 3, 0, 0, 0, dist > 120 ? 40 : 75);
            return;
        }
    }
}

void fxDraw(SDL_Renderer *renderer, int startX, int startY, int view_y, int view_h)
{
    SDL_Rect clip = {0, view_y, (int) WINDOW_W, view_h};
    int split = view_h < (int) WINDOW_H;

    if(split)
        SDL_RenderSetClipRect(renderer, &clip);

    for(int i = 0; i < num_particles; i++)
    {
        Particle *p = &particles[i];
        float t = (float) p->life / p->maxlife;
        int x = (int) (p->x - startX), y = (int) (p->y - startY) + view_y;
        int a = (int) (255 * (t > 0.5f ? 1.0f : t * 2.0f));

        if(x < -40 || x > WINDOW_W + 40 || y < view_y - 40 || y > view_y + view_h + 40)
            continue;

        switch(p->type)
        {
            case P_RING:
                aacircleRGBA(renderer, x, y, (int) p->size, p->r, p->g, p->b, a);
                aacircleRGBA(renderer, x, y, (int) p->size + 1, p->r, p->g, p->b, a / 2);
                break;
            case P_SMOKE:
                filledCircleRGBA(renderer, x, y, (int) p->size, p->r, p->g, p->b, a * 6 / 10);
                break;
            case P_STAR:
            {
                int s = (int) p->size;
                filledTrigonRGBA(renderer, x, y - s, x - s / 2, y, x + s / 2, y, p->r, p->g, p->b, a);
                filledTrigonRGBA(renderer, x, y + s, x - s / 2, y, x + s / 2, y, p->r, p->g, p->b, a);
                filledTrigonRGBA(renderer, x - s, y, x, y - s / 2, x, y + s / 2, p->r, p->g, p->b, a);
                filledTrigonRGBA(renderer, x + s, y, x, y - s / 2, x, y + s / 2, p->r, p->g, p->b, a);
                break;
            }
            case P_HEART:
            {
                int s = (int) p->size;
                filledCircleRGBA(renderer, x - s / 2, y - s / 3, s / 2 + 1, p->r, p->g, p->b, a);
                filledCircleRGBA(renderer, x + s / 2, y - s / 3, s / 2 + 1, p->r, p->g, p->b, a);
                filledTrigonRGBA(renderer, x - s, y - s / 4, x + s, y - s / 4, x, y + s, p->r, p->g, p->b, a);
                break;
            }
            default:
                filledCircleRGBA(renderer, x, y, p->size < 1 ? 1 : (int) p->size, p->r, p->g, p->b, a);
        }
    }

    if(split)
        SDL_RenderSetClipRect(renderer, NULL);
}

void fxVignette(SDL_Renderer *renderer)
{
    if(FX_VIGNETTE && vignette != NULL)
        SDL_RenderCopy(renderer, vignette, NULL, NULL);
}

/* Le tremblement decale temporairement le scrolling (au lieu d'un viewport SDL, source de problemes selon le moteur de rendu). */
static int saved_startX[2], saved_startY[2];

void fxShakeBegin(Lvl *lvl, int num_players)
{
    for(int i = 0; i < num_players && i < 2; i++)
    {
        saved_startX[i] = lvl->startX[i];
        saved_startY[i] = lvl->startY[i];

        int nx = lvl->startX[i] + shake_dx, ny = lvl->startY[i] + shake_dy;

        if(nx >= 0 && nx + WINDOW_W < lvl->maxX)
            lvl->startX[i] = nx;
        if(ny >= 0 && ny + WINDOW_H / num_players < lvl->maxY)
            lvl->startY[i] = ny;
    }
}

void fxShakeEnd(Lvl *lvl, int num_players)
{
    for(int i = 0; i < num_players && i < 2; i++)
    {
        lvl->startX[i] = saved_startX[i];
        lvl->startY[i] = saved_startY[i];
    }
}
