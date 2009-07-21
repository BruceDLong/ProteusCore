#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "cairosdl.h"

#define dprintf(args)

#define BLIT_BOBS_USING_CAIRO 1

struct vector {
    double x, y;
};

#define MAX_BOBS 20

struct bob {
    struct vector pos;
    struct vector vel;
    struct vector accel;
    double mass;
    double radius;
    double focus;
    cairo_surface_t *surface;
};

static void
init_bobs (struct bob *bobs, size_t num_bobs)
{
    size_t i;
    double fill_ratio = 0.95;
    double n = sqrt(num_bobs);

    for (i=0; i<num_bobs; i++) {
        struct bob *bob = bobs + i;
        double theta = 1.0 - (i+0.5) / MAX_BOBS;
        double r = fill_ratio*(0.5 + theta*0.0)/n;
        r = r < 0.2 ? r : 0.2;
        bob->pos.x = rand () * 1.0 / RAND_MAX;
        bob->pos.y = rand () * 1.0 / RAND_MAX;
        bob->vel.x = bob->vel.y = 0;
        bob->accel.x = bob->accel.y = 0;
        bob->mass = (1+theta)*(1+theta);
        bob->mass = 1.0;      /* equal mass bobs have a smoother ride */
        bob->mass /= num_bobs;
        bob->radius = r;
        bob->focus = 1.0;
        bob->surface = NULL;
    }
}

static void
alloc_bobs (struct bob *bobs, size_t num_bobs)
{
    size_t i;
    SDL_Surface *screen = SDL_GetVideoSurface ();

    for (i=0; i<num_bobs; i++) {
        struct bob *bob = bobs + i;
        int width = screen->w * 2*bob->radius + 1;
        int height = screen->h * 2*bob->radius + 1;

        if (bob->surface) {
            cairo_surface_destroy (bob->surface);
            bob->surface = NULL;
        }

        if (BLIT_BOBS_USING_CAIRO) {
            bob->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                       width, height);
        }
        else {
            SDL_Surface *sdl_surface = SDL_CreateRGBSurface (
                SDL_SWSURFACE | SDL_SRCALPHA,
                width, height, 32,
                CAIROSDL_RMASK,
                CAIROSDL_GMASK,
                CAIROSDL_BMASK,
                CAIROSDL_AMASK);
            if (sdl_surface == NULL) {
                fprintf (stderr, "Failed allocating bob sdl surfaces: %s\n",
                         SDL_GetError ());
                exit (1);
            }
            assert (!SDL_MUSTLOCK (sdl_surface));
            bob->surface = cairosdl_surface_create (sdl_surface);
            SDL_FreeSurface (sdl_surface);
        }

        if (cairo_surface_status (bob->surface) != CAIRO_STATUS_SUCCESS) {
            cairo_status_t status = cairo_surface_status (bob->surface);
            fprintf (stderr, "Failed making a cairo surface for a bob: %s\n",
                     cairo_status_to_string (status));
            exit (1);
        }

    }
}

static void
render_bob (struct bob *bob, int i)
{
    int width = cairo_image_surface_get_width (bob->surface);
    int height = cairo_image_surface_get_height (bob->surface);
    cairo_t *cr = cairo_create (bob->surface);
    double theta = (i+0.5) / MAX_BOBS;
    double dx = bob->pos.x - 0.5;
    double dy = bob->pos.y - 0.5;

    cairo_scale (cr, 0.5*width, 0.5*height);
    cairo_translate (cr, 1.0, 1.0);

    cairo_set_tolerance (cr, 0.5);

    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    cairo_set_source_rgba (cr,
                           1-fabs(dx),
                           1-fabs(dy),
                           theta,
                           (1-2*fabs(dx))*(1-2*fabs(dy)));
    if (1) {
        cairo_pattern_t *pat;
        pat = cairo_pattern_create_radial (
            -dx, -dy, 0.0,
            0, 0, 1.0);
        cairo_pattern_add_color_stop_rgba (pat, 0.0,
                                           fabs(dy),
                                           0,
                                           fabs(dx),
                                           0);

        cairo_pattern_add_color_stop_rgba (pat, bob->focus,
                                           fabs(dx+dy),
                                           i&1 ? 1 : 1,
                                           i&1 ? fabs(dx) : 1-fabs(dx),
                                           0.6);

        cairo_pattern_add_color_stop_rgba (pat, 1.0,
                                           1 - fabs(dy),
                                           1 - fabs(dx),
                                           theta,
                                           0.0);

        cairo_set_source (cr, pat);
        cairo_pattern_destroy (pat);
    }

    cairo_arc (cr, 0,0, 1.0, 0, 6.29);
    cairo_close_path (cr);
    cairo_fill (cr);

    cairosdl_destroy (cr);
}

static void
render_bobs (struct bob *bobs, size_t num_bobs)
{
    size_t i;
    for (i=0; i<num_bobs; i++) {
        render_bob (bobs+i, i);
    }
}

static void
blit_bobs_using_sdl (struct bob *bobs, size_t num_bobs)
{
    size_t i;
    SDL_Surface *screen = SDL_GetVideoSurface ();

    for (i=0; i<num_bobs; i++) {
        struct bob *bob = bobs + i;
        SDL_Surface *sdl_surface = cairosdl_surface_get_target (bob->surface);
        SDL_Rect src_rect[1];
        SDL_Rect dst_rect[1];

        src_rect->x = 0;
        src_rect->y = 0;
        src_rect->w = sdl_surface->w;
        src_rect->h = sdl_surface->h;

        dst_rect->x = (bob->pos.x - bob->radius) * screen->w;
        dst_rect->y = (bob->pos.y - bob->radius) * screen->h;
        dst_rect->w = sdl_surface->w;
        dst_rect->h = sdl_surface->h;

        SDL_BlitSurface (sdl_surface, src_rect, screen, dst_rect);
    }
}

static void
blit_bobs_using_cairo (struct bob *bobs, size_t num_bobs)
{
    size_t i;
    SDL_Surface *screen = SDL_GetVideoSurface ();
    cairo_t *cr;

    while (SDL_LockSurface (screen) != 0) {
        SDL_Delay (1);
    }

    cr = cairosdl_create (screen);

    for (i=0; i<num_bobs; i++) {
        struct bob *bob = bobs + i;
        int x = (int)((bob->pos.x - bob->radius) * screen->w);
        int y = (int)((bob->pos.y - bob->radius) * screen->h);
        int width = cairo_image_surface_get_width (bob->surface);
        int height = cairo_image_surface_get_height (bob->surface);

        cairo_set_source_surface (cr, bob->surface, x,y);
        cairo_rectangle (cr,
                         x, y,
                         width, height);
        cairo_fill (cr);
    }

    cairo_destroy (cr);

    SDL_UnlockSurface (screen);
}

#define SQR(x) ((x)*(x))

static double
sim_bobs (struct bob *bobs, size_t num_bobs,
          double t0, double t1)
{
    double dt = 0.002;
    double G = 0.5;
    double t = t0;

    if (dt*10 < (t1 - t0) && 0)
        dt = (t1 - t0) / 10.0;

    while (t < t1) {
        size_t i, j;

        for (i=0; i<num_bobs; i++) {
            double theta = (i+0.5) / MAX_BOBS;
            double f = 0.3;
            bobs[i].focus = f + (0.96-f)*0.5*(1 + cos(3.141*t*(1-theta)));
            bobs[i].accel.x = (0.5-bobs[i].pos.x)*0.0;
            bobs[i].accel.y = (0.5-bobs[i].pos.y)*0.0;
        }

        /* Basic mass attraction forces. */
        for (i=0; i<num_bobs; i++) {
            struct bob *p = bobs + i;
            for (j=i+1; j<num_bobs; j++) {
                struct bob *q = bobs + j;
                double dx = q->pos.x - p->pos.x;
                double dy = q->pos.y - p->pos.y;
                double dist2 = SQR(dx) + SQR(dy);
                double f = G / dist2;

                if ((i^j) & 1) {
                    f *= -1;
                }

                p->accel.x += dx*f*q->mass;
                p->accel.y += dy*f*q->mass;
                q->accel.x -= dx*f*p->mass;
                q->accel.y -= dy*f*p->mass;
            }
        }


        /* Integrate one step forwards. */
        for (i=0; i<num_bobs; i++) {
            struct bob *p = bobs + i;

            p->accel.x = p->accel.x;
            p->accel.y = p->accel.y;

            p->vel.x += dt*p->accel.x;
            p->vel.y += dt*p->accel.y;

            p->vel.x = p->vel.x;
            p->vel.y = p->vel.y;

            p->pos.x = p->pos.x + dt*p->vel.x;
            p->pos.y = p->pos.y + dt*p->vel.y;

        }

        /* Apply position constraints. */
        for (i=0; i<num_bobs; i++) {
            double eps = 0.0;
            struct bob *p = bobs + i;

            /* Bounce off each other after allowed overlap. */
            for (j=i+1; j<num_bobs; j++) {
                struct bob *q = bobs + j;
                double dx = q->pos.x - p->pos.x;
                double dy = q->pos.y - p->pos.y;
                double dist = sqrt(SQR(dx) + SQR(dy));
                double overlap = p->radius*p->focus + q->radius*q->focus - dist - 0.02;

                if (overlap < 0.0)
                    continue;
                p->pos.x -= dx*overlap/dist;
                p->pos.y -= dy*overlap/dist;
                q->pos.x += dx*overlap/dist;
                q->pos.y += dy*overlap/dist;

                /* Swap velocity vectors, preserve momentum. */
                {
                    double scale;
                    struct vector tmp = p->vel;
                    p->vel = q->vel;
                    q->vel = tmp;

                    scale = q->mass/p->mass;
                    p->vel.x *= scale;
                    p->vel.y *= scale;
                    scale = p->mass/q->mass;
                    q->vel.x *= scale;
                    q->vel.y *= scale;
                }
            }

            /* Bounce off walls */
            if (p->pos.x > 1+eps - p->radius*p->focus) {
                p->pos.x = 1+eps - p->radius*p->focus;
                p->vel.x *= -1;
            }
            if (p->pos.x < 0-eps + p->radius*p->focus) {
                p->pos.x = 0-eps + p->radius*p->focus;
                p->vel.x *= -1;
            }
            if (p->pos.y > 1+eps - p->radius*p->focus) {
                p->pos.y = 1+eps - p->radius*p->focus;
                p->vel.y *= -1;
            }
            if (p->pos.y < 0-eps + p->radius*p->focus) {
                p->pos.y = 0-eps + p->radius*p->focus;
                p->vel.y *= -1;
            }

        }


        t += dt;
        if (t + dt >= t1)
            dt = t1 - t + 1e-6;
    }

    return t1;
}

static void
push_expose ()
{
    SDL_Event event[1];
    event->type = SDL_VIDEOEXPOSE;
    if (SDL_PushEvent (event) != 0) {
        fprintf (stderr, "Failed to push an expose event: %s\n",
                 SDL_GetError ());
    }
}

static void
on_expose (struct bob *bobs, size_t num_bobs)
{
    SDL_Surface *screen = SDL_GetVideoSurface ();

    SDL_FillRect (screen, NULL,
                  SDL_MapRGBA (screen->format,
                               0,0,0,SDL_ALPHA_OPAQUE));

    if (BLIT_BOBS_USING_CAIRO)
        blit_bobs_using_cairo (bobs, num_bobs);
    else
        blit_bobs_using_sdl (bobs, num_bobs);

    SDL_Flip (screen);

    push_expose ();             /* Schedule another expose soon, */
}

static void
event_loop (unsigned flags, int width, int height)
{
    struct bob bobs[MAX_BOBS];
    size_t num_bobs = MAX_BOBS;

    double t0 = SDL_GetTicks () / 1000.0; /* Current simulation time.. */
    double t1;                  /* Next simulation time. */
    SDL_Event event[1];

    init_bobs (bobs, num_bobs);

    event->resize.type = SDL_VIDEORESIZE;
    event->resize.w = width;
    event->resize.h = height;
    SDL_PushEvent (event);

    while (SDL_WaitEvent (event)) {
        switch (event->type) {
        case SDL_VIDEORESIZE:
            if (SDL_SetVideoMode (event->resize.w,
                                  event->resize.h,
                                  32, flags) == NULL)
            {
                fprintf (stderr, "Failed to set video mode: %s\n",
                         SDL_GetError ());
                exit (1);
            }
            alloc_bobs (bobs, num_bobs);
            render_bobs (bobs, num_bobs);
            /* fallthrough  */

        case SDL_VIDEOEXPOSE:
            t1 = SDL_GetTicks () / 1000.0;
            render_bobs (bobs, num_bobs);
            t0 = sim_bobs (bobs, num_bobs, t0, t1);
            on_expose (bobs, num_bobs);
            break;

        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_q)
                return;
        }
    }
    fprintf (stderr, "WaitEvent failed: %s\n", SDL_GetError ());
}

int
main()
{
    int width = 600;
    int height = 600;
    int flags = SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;

    if (SDL_Init (flags) < 0) {
        fprintf (stderr, "Failed to initialise SDL: %s\n",
                 SDL_GetError ());
        exit (1);
    }
    atexit (SDL_Quit);

    if (1) {
        event_loop (
            SDL_SWSURFACE | SDL_RESIZABLE,
            width, height);
    }
    else {
        event_loop (
            SDL_HWSURFACE |
            SDL_FULLSCREEN |
            SDL_DOUBLEBUF,
            width, height);
    }
    return 0;
}
