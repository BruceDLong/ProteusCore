#include <assert.h>
#include <string.h>
#include "cairosdl.h"

static int
sdl_surface_eq(SDL_Surface *a, SDL_Surface *b)
{
    if (a->format->BitsPerPixel != 32) return 0;
    if (a->format->BytesPerPixel != 4) return 0;
    if (SDL_MUSTLOCK(a)) return 0;
    if (SDL_MUSTLOCK(b)) return 0;

#define check_field(name) if (a->name != b->name) return 0
    check_field(format->BitsPerPixel);
    check_field(format->BytesPerPixel);
    check_field(format->Rmask);
    check_field(format->Gmask);
    check_field(format->Bmask);
    check_field(format->Amask);
    check_field(format->colorkey);
    check_field(format->alpha);
    check_field(w);
    check_field(h);

    {
        unsigned char *a_bytes = a->pixels;
        unsigned char *b_bytes = b->pixels;
        size_t row_size = a->format->BytesPerPixel * a->w;
        int i;
        for (i=0; i<a->h; i++) {
            if (memcmp(a_bytes, b_bytes, row_size))
                return 0;
            a_bytes += a->pitch;
            b_bytes += b->pitch;
        }
        return 1;
    }
}

static SDL_Surface *
dup_sdl_surface(SDL_Surface *a)
{
    SDL_Surface *b = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                          a->w, a->h,
                                          a->format->BitsPerPixel,
                                          a->format->Rmask,
                                          a->format->Gmask,
                                          a->format->Bmask,
                                          a->format->Amask);
    unsigned char *b_bytes = b->pixels;
    unsigned char *a_bytes = a->pixels;
    size_t row_size = a->format->BytesPerPixel * a->w;
    int i;
    assert(! SDL_MUSTLOCK(a) );
    assert(! SDL_MUSTLOCK(b) );

    for (i=0; i<a->h; i++) {
        memcpy(b_bytes, a_bytes, row_size);
        a_bytes += a->pitch;
        b_bytes += b->pitch;
    }
    return b;
}

static int
test_argb32()
{
    SDL_Surface *ref;
    SDL_Surface *sdlsurf = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        100, 100, 32,
        CAIROSDL_RMASK,
        CAIROSDL_GMASK,
        CAIROSDL_BMASK,
        CAIROSDL_AMASK);

    /* Prefilling the surface tests the mark_dirty functions. */
    SDL_FillRect(sdlsurf, NULL,
                 SDL_MapRGBA(sdlsurf->format,255,0,0,128));

    /* Make the reference image. */
    {
        SDL_Rect r;
        ref = dup_sdl_surface (sdlsurf);
        r.x = r.y = 25;
        r.w = r.h = 50;
        SDL_FillRect(ref, &r,
                     SDL_MapRGBA(ref->format,255,170,0,192));
    }

    /* Draw with cairo.  cairosdl_destroy() calls the cairosdl surface
     * flush. */
    {
        cairo_t *cr = cairosdl_create(sdlsurf);

        cairo_set_source_rgba(cr, 1,1,0,0.5);
        cairo_rectangle(cr, 25,25,50,50);
        cairo_fill(cr);

        cairosdl_destroy(cr);
    }

    /* Check vs the reference. */
    {
        int ok = sdl_surface_eq(ref,sdlsurf);
        SDL_FreeSurface(ref);
        SDL_FreeSurface(sdlsurf);
        return ok;
    }
}

int
main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
    atexit(SDL_Quit);

    return test_argb32() ? 0 : 1;
}
