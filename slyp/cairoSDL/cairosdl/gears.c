/*
 * Copyright © 2004 David Reveman, Peter Nilsson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the names of
 * David Reveman and Peter Nilsson not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission. David Reveman and Peter Nilsson
 * makes no representations about the suitability of this software for
 * any purpose. It is provided "as is" without express or implied warranty.
 *
 * DAVID REVEMAN AND PETER NILSSON DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL DAVID REVEMAN AND
 * PETER NILSSON BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: David Reveman <c99drn@cs.umu.se>
 *          Peter Nilsson <c99pnn@cs.umu.se>
 */
#include <stdlib.h>
#include <cairo.h>
#include <math.h>

#define LINEWIDTH 3.0

#define FILL_R 0.1
#define FILL_G 0.1
#define FILL_B 0.75
#define FILL_OPACITY 0.5

#define STROKE_R 0.1
#define STROKE_G 0.75
#define STROKE_B 0.1
#define STROKE_OPACITY 1.0

#define NUMPTS 6

#define CHEAT_SHADOWS 1         /* 1: use opaque gear shadows,
                                 * 0: semitransparent shadows like qgears2 */

static double animpts[NUMPTS * 2];
static double deltas[NUMPTS * 2];

static int fill_gradient = 0;

static void
gear (cairo_t *cr,
	double inner_radius,
	double outer_radius,
	int teeth,
	double tooth_depth)
{
    int i;
    double r0, r1, r2;
    double angle, da;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0;
    r2 = outer_radius + tooth_depth / 2.0;

    da = 2.0 * M_PI / (double) teeth / 4.0;

    cairo_new_path (cr);

    angle = 0.0;
    cairo_move_to (cr, r1 * cos (angle + 3 * da), r1 * sin (angle + 3 * da));

    for (i = 1; i <= teeth; i++) {
	angle = i * 2.0 * M_PI / (double) teeth;

	cairo_line_to (cr, r1 * cos (angle), r1 * sin (angle));
	cairo_line_to (cr, r2 * cos (angle + da), r2 * sin (angle + da));
	cairo_line_to (cr, r2 * cos (angle + 2 * da), r2 * sin (angle + 2 * da));

	if (i < teeth)
	    cairo_line_to (cr, r1 * cos (angle + 3 * da),
		    r1 * sin (angle + 3 * da));
    }

    cairo_close_path (cr);

    cairo_move_to (cr, r0 * cos (angle + 3 * da), r0 * sin (angle + 3 * da));

    for (i = 1; i <= teeth; i++) {
	angle = i * 2.0 * M_PI / (double) teeth;

	cairo_line_to (cr, r0 * cos (angle), r0 * sin (angle));
    }

    cairo_close_path (cr);
}

void
trap_setup (cairo_surface_t *target, int w, int h)
{
    int i;

    (void)target;
    //cairo_scale (cr, 3.0, 1.0);

    for (i = 0; i < (NUMPTS * 2); i += 2) {
	animpts[i + 0] = (float) (drand48 () * w);
	animpts[i + 1] = (float) (drand48 () * h);
	deltas[i + 0] = (float) (drand48 () * 6.0 + 4.0);
	deltas[i + 1] = (float) (drand48 () * 6.0 + 4.0);
	if (animpts[i + 0] > w / 2.0) {
	    deltas[i + 0] = -deltas[i + 0];
	}
	if (animpts[i + 1] > h / 2.0) {
	    deltas[i + 1] = -deltas[i + 1];
	}
    }
}

static void
stroke_and_fill_animate (double *pts,
	double *deltas,
	int index,
	int limit)
{
    double newpt = pts[index] + deltas[index];

    if (newpt <= 0) {
	newpt = -newpt;
	deltas[index] = (double) (drand48 () * 4.0 + 2.0);
    } else if (newpt >= (double) limit) {
	newpt = 2.0 * limit - newpt;
	deltas[index] = - (double) (drand48 () * 4.0 + 2.0);
    }
    pts[index] = newpt;
}

static void
stroke_and_fill_step (int w, int h)
{
    int i;

    for (i = 0; i < (NUMPTS * 2); i += 2) {
	stroke_and_fill_animate (animpts, deltas, i + 0, w);
	stroke_and_fill_animate (animpts, deltas, i + 1, h);
    }
}

static double gear1_rotation = 0.35;
static double gear2_rotation = 0.33;
static double gear3_rotation = 0.50;

void
trap_render (cairo_t *cr, int w, int h)
{
    double *ctrlpts = animpts;
    int len = (NUMPTS * 2);
    double prevx = ctrlpts[len - 2];
    double prevy = ctrlpts[len - 1];
    double curx = ctrlpts[0];
    double cury = ctrlpts[1];
    double midx = (curx + prevx) / 2.0;
    double midy = (cury + prevy) / 2.0;
    int i;
    int pass;

    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle (cr, 0, 0, w, h);
    cairo_fill (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba (cr, 0.75, 0.75, 0.75, 1.0);
    cairo_set_line_width (cr, 1.0);

    cairo_save (cr); {
        cairo_scale (cr, (double) w / 512.0, (double) h / 512.0);

        cairo_save (cr); {
            cairo_translate (cr, -10.0, -10.0);
            cairo_translate (cr, 170.0, 330.0);
            cairo_rotate (cr, gear1_rotation);
            gear (cr, 30.0, 120.0, 20, 20.0);
            cairo_set_source_rgba (cr, 0.70, 0.70, 0.70, 0.70 + CHEAT_SHADOWS);
            cairo_fill (cr);
            cairo_restore (cr);
        }
        cairo_save (cr); {
            cairo_translate (cr, -10.0, -10.0);
            cairo_translate (cr, 369.0, 330.0);
            cairo_rotate (cr, gear2_rotation);
            gear (cr, 15.0, 75.0, 12, 20.0);
            cairo_set_source_rgba (cr, 0.70, 0.70, 0.70, 0.70 + CHEAT_SHADOWS);
            cairo_fill (cr);
            cairo_restore (cr);
        }
        cairo_save (cr); {
            cairo_translate (cr, -10.0, -10.0);
            cairo_translate (cr, 170.0, 116.0);
            cairo_rotate (cr, gear3_rotation);
            gear (cr, 20.0, 90.0, 14, 20.0);
            cairo_set_source_rgba (cr, 0.70, 0.70, 0.70, 0.70 + CHEAT_SHADOWS);
            cairo_fill (cr);
            cairo_restore (cr);
        }

        cairo_save (cr); {
            cairo_translate (cr, 170.0, 330.0);
            cairo_rotate (cr, gear1_rotation);
            gear (cr, 30.0, 120.0, 20, 20.0);
            cairo_set_source_rgb (cr, 0.75, 0.75, 0.75);
            cairo_fill_preserve (cr);
            cairo_set_source_rgb (cr, 0.25, 0.25, 0.25);
            cairo_stroke (cr);
            cairo_restore (cr);
        }
        cairo_save (cr); {
            cairo_translate (cr, 369.0, 330.0);
            cairo_rotate (cr, gear2_rotation);
            gear (cr, 15.0, 75.0, 12, 20.0);
            cairo_set_source_rgb (cr, 0.75, 0.75, 0.75);
            cairo_fill_preserve (cr);
            cairo_set_source_rgb (cr, 0.25, 0.25, 0.25);
            cairo_stroke (cr);
            cairo_restore (cr);
        }
        cairo_save (cr); {
            cairo_translate (cr, 170.0, 116.0);
            cairo_rotate (cr, gear3_rotation);
            gear (cr, 20.0, 90.0, 14, 20.0);
            cairo_set_source_rgb (cr, 0.75, 0.75, 0.75);
            cairo_fill_preserve (cr);
            cairo_set_source_rgb (cr, 0.25, 0.25, 0.25);
            cairo_stroke (cr);
            cairo_restore (cr);
        }

        cairo_restore (cr);
    }

    gear1_rotation += 0.01;
    gear2_rotation -= (0.01 * (20.0 / 12.0));
    gear3_rotation -= (0.01 * (20.0 / 14.0));

    stroke_and_fill_step (w, h);

    cairo_translate (cr, -10, -10);
    for (pass = 1; pass <= 2; pass++) {
        cairo_new_path (cr);
        cairo_move_to (cr, midx, midy);

        for (i = 2; i <= (NUMPTS * 2); i += 2) {
            double x2, x1 = (midx + curx) / 2.0;
            double y2, y1 = (midy + cury) / 2.0;

            prevx = curx;
            prevy = cury;
            if (i < (NUMPTS * 2)) {
                curx = ctrlpts[i + 0];
                cury = ctrlpts[i + 1];
            } else {
                curx = ctrlpts[0];
                cury = ctrlpts[1];
            }
            midx = (curx + prevx) / 2.0;
            midy = (cury + prevy) / 2.0;
            x2 = (prevx + midx) / 2.0;
            y2 = (prevy + midy) / 2.0;
            cairo_curve_to (cr, x1, y1, x2, y2, midx, midy);
        }
        cairo_close_path (cr);

        if (pass == 1) {
            cairo_set_source_rgba (cr, 0,0,0,77/255.0);
            cairo_fill (cr);
            cairo_translate (cr, 10, 10);
        }
    }

    if (fill_gradient) {
	double x1, y1, x2, y2;
	cairo_pattern_t *pattern;

	cairo_fill_extents (cr, &x1, &y1, &x2, &y2);

	pattern = cairo_pattern_create_linear (x1, y1, x2, y2);
	cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 1.0, 0.75);
	cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1.0, 0.0, 0.0, 1.0);
	cairo_pattern_set_filter (pattern, CAIRO_FILTER_BILINEAR);

	cairo_move_to (cr, 0, 0);
	cairo_set_source (cr, pattern);
	cairo_pattern_destroy (pattern);
    } else {
	cairo_set_source_rgba (cr, FILL_R, FILL_G, FILL_B, FILL_OPACITY);
    }

    cairo_fill_preserve (cr);
    cairo_set_source_rgba (cr, STROKE_R, STROKE_G, STROKE_B, STROKE_OPACITY);
    cairo_set_line_width (cr, LINEWIDTH);
    cairo_stroke (cr);
}


/* SDL code */
#include "cairosdl.h"

static Uint32
print_fps_timer (Uint32 interval, void *param)
{
    unsigned *frame_counter = (unsigned *)param;
    unsigned num_frames = *frame_counter;
    *frame_counter = 0;

    printf("%u frames in %u ms = %.3f fps\n",
           num_frames, interval, 1000.0*num_frames / interval);

    return interval;
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
event_loop (unsigned flags, int width, int height)
{
    unsigned num_frames_rendered = 0;
    SDL_Event event[1];
    event->resize.type = SDL_VIDEORESIZE;
    event->resize.w = width;
    event->resize.h = height;
    SDL_PushEvent (event);

    SDL_AddTimer (5000, print_fps_timer, &num_frames_rendered);

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
            width = event->resize.w;
            height = event->resize.h;
            /* fallthrough  */

        case SDL_VIDEOEXPOSE: {
            SDL_Surface *screen = SDL_GetVideoSurface ();
            cairo_t *cr;
            cairo_status_t status;

            while (SDL_LockSurface (screen) != 0)
                SDL_Delay (1);

            cr = cairosdl_create (screen);
            trap_render (cr, width, height);
            status = cairo_status (cr);
            cairosdl_destroy (cr);

            SDL_UnlockSurface (screen);
            SDL_Flip (screen);

            ++num_frames_rendered;

            if (status != CAIRO_STATUS_SUCCESS) {
                fprintf (stderr, "Failed to render: %s\n",
                         cairo_status_to_string (status));
                exit (1);
            }

            push_expose ();             /* Schedule another expose soon, */
            break;
        }
        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_q)
                return;
        }
    }
    fprintf (stderr, "WaitEvent failed: %s\n", SDL_GetError ());
}

int
main (int argc, char **argv)
{
    int width = 512;
    int height = 512;
    int flags = SDL_SWSURFACE;
    int init_flags = SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
    int i;

    if (SDL_Init (init_flags) < 0) {
        fprintf (stderr, "Failed to initialise SDL: %s\n",
                 SDL_GetError ());
        exit (1);
    }
    atexit (SDL_Quit);

    for (i=1; i<argc; i++) {
        if (0 == strcmp(argv[i], "-gradient")) {
            fill_gradient = 1;
        }
        else if (0 == strcmp(argv[i], "-fullscreen")) {
            flags |= SDL_HWSURFACE | SDL_FULLSCREEN;
        }
        else if (0 == strcmp(argv[i], "-hwsurface")) {
            flags |= SDL_HWSURFACE;
        }
        else if (0 == strcmp(argv[i], "-swsurface")) {
            flags |= SDL_SWSURFACE;
        }
        else if (0 == strcmp(argv[i], "-doublebuf")) {
            flags |= SDL_DOUBLEBUF;
        }
        else if (0 == strcmp(argv[i], "-resizable")) {
            flags |= SDL_RESIZABLE;
        }
        else {
            fprintf(stderr, "usage: [-gradient] [-fullscreen] [-resizable]\n");
        }
    }

    event_loop (flags, width, height);
    return 0;
}
