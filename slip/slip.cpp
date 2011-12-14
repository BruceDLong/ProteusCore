// The SDL SlipStream Engine. Copyright 2008-2011 by Bruce Long
/*  This file is part of the "SlipStream Engine"
    The SlipStream Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The SlipStream Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the SlipStream Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#define windowTitle "Turbulance"
#define MAX_PORTALS 24

// GraphicsEngine: OPENGL, GLES, DIRECTX
// CPU: Ix86, ARMEH, ARMEL
// OpSys: LINUX, MAC, WINDOWS, IOS, ANDROID
#define GraphicsEngine_NONE
#define CPU_ARMEL
#define OpSys_LINUX

#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftadvanc.h>
#include <freetype/ftsnames.h>
#include <freetype/tttables.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#include "sdl_common.h" // SDL state mgmt. (displays, windows, common events, etc.)

enum {LINUX, MACINTOSH, WINDOWS, IOS, ANDROID}

const QPLATFORM=LINUX;

int height, width, bpp;
static int doneYet = 0;
static int num = 0, numPortals=0;

//////////////////// Slip Code
#include <strstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../core/Proteus.h"

#define DEB(msg)  {std::cout<< msg;}
#define DEBl(msg) {std::cout<< msg << "\n";}
#define MSG(msg)  {std::cout<< msg;}
#define MSGl(msg) {std::cout<< msg << "\n";}
#define ERR(msg)  {std::cout<< msg;}
#define ERRl(msg) {std::cout<< msg << "\n";}

#define gINT gInt()
#define gZto1 (float)(((float)gInt()) / 1000.0)
#define gSTR gStr()

#define Z1 {a=gZto1;  DEB(a)}
#define Z2 {a=gZto1; b=gZto1; DEB(a<<", "<<b)}
#define Z3 {a=gZto1; b=gZto1; c=gZto1; DEB(a<<", "<<b<<", "<< c)}
#define Z4 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; DEB(a<<", "<<b<<", "<< c<<", "<< d)}
#define Z5 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; e=gZto1;}
#define Z6 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; e=gZto1; f=gZto1; DEB(a<<", "<<b<<", "<< c<<", "<< d<<", "<< e<<", "<< f)}
#define I2 {Ia=gINT; Ib=gINT;}
#define I3 {Ia=gINT; Ib=gINT; Ic=gINT;}
#define I4 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT;}
#define I5 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT;}
#define I6 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT; If=gINT;}
#define I7 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT; If=gINT; Ih=gINT;}
#define I8 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT; If=gINT; Ih=gINT; II=gINT;}
#define S1 {Sa=gSTR;}

infon* ItmPtr=0;
agent theAgent;
int EOT_d2,j,sign;
char textBuff[1024];
extern infon* Theme;
char* worldFile="World.pr";

int gInt(){
    getInt(ItmPtr,j,sign);  // DEB(j << ", ");
    theAgent.getNextTerm(&ItmPtr);
    return (sign)?-j:j;
}

char* gStr() {
    if((ItmPtr->pFlag&tType)==tString && !(ItmPtr->pFlag&fUnknown)) {
        memcpy(textBuff, ItmPtr->value, (uint)ItmPtr->size);
        textBuff[(uint)(ItmPtr->size)]=0;
    }
    theAgent.getNextTerm(&ItmPtr);
    return textBuff;
}

infon* gList(){
    infon* ret=ItmPtr;
    theAgent.getNextTerm(&ItmPtr);
    return ret;
}

const int maxVerts = 1024*20;
int vertexes[maxVerts];  // array to store vertexes.
float texCoords[maxVerts]; // array to store texture coordinates

int getArrayi(int* array){
    int size=gInt();
    for (int i=0; i<size; ++i){
        array[i]=gInt();
    }
    return size;
}
int getArrayz(float* array){ // gets an array of 0..1 values (Zero)
    int size=gInt();
    for (int i=0; i<size; ++i){
        array[i]=gZto1;
    }
    return size;
}

static inline uint32_t map_value(uint32_t val, uint32_t max, uint32_t tomax){
    return((uint32_t)((double)val * (double)tomax/(double)max));
}

static inline SDL_Surface *load_image(const char *filename){
    SDL_Surface* loadedImage = NULL;
    SDL_Surface* optimizedImage = NULL;

    if((loadedImage = IMG_Load(filename))) {
        optimizedImage = SDL_DisplayFormat(loadedImage);
        SDL_FreeSurface(loadedImage);
    }
    return optimizedImage;
}

void roundedRectangle(cairo_t *cr, double x, double y, double w, double h, double r){
    cairo_new_sub_path (cr);
    cairo_move_to(cr,x+r,y);                      //# Move to A
    cairo_line_to(cr,x+w-r,y);                    //# Straight line to B
    cairo_curve_to(cr,x+w,y,x+w,y,x+w,y+r);       //# Curve to C, Control points are both at Q
    cairo_line_to(cr,x+w,y+h-r);                  //# Move to D
    cairo_curve_to(cr,x+w,y+h,x+w,y+h,x+w-r,y+h); //# Curve to E
    cairo_line_to(cr,x+r,y+h);                    //# Line to F
    cairo_curve_to(cr,x,y+h,x,y+h,x,y+h-r);       //# Curve to G
    cairo_line_to(cr,x,y+r);                      //# Line to H
    cairo_curve_to(cr,x,y,x,y,x+r,y);             //# Curve to A;
    cairo_close_path(cr);
}

enum dTools{rectangle=1, curvedRect, circle, lineTo, lineRel, moveTo, moveRel, curveTo, curveRel, arcClockWise, arcCtrClockW, text,
            strokePath=16, fillPath, paintSurface, closePath,
            inkColor=20, inkGradient, inkImage, inkDrawing,
            lineWidth=25, lineStyle, fontFace, fontSize,
            drawToScrnN=30, drawToWindowN, drawToMemory, drawToPDF, drawToPS, drawToSVG, drawToPNG,
            shiftTo=40, scale, rotate,
            drawItem=100
};

void DrawProteusDescription(cairo_surface_t* surface, infon* ProteusDesc){
    if (surface==0 || ProteusDesc==0 || ProteusDesc->size==0) ERRl("Missing description.");
    //DEBl("DISPLAY:["<<printInfon(ProteusDesc));
    int count=0;
    infon *i, *OldItmPtr, *subItem;
    cairo_t *cr = cairo_create(surface); cairo_set_antialias(cr,CAIRO_ANTIALIAS_GRAY);
DEB("\n-----------------")
    int size, size2; float a,b,c,d,e,f; int Ia,Ib,Ic,Id,Ie,If,Ih,II; char  *Sa, *Sb;
    for(int EOL=theAgent.StartTerm(ProteusDesc, &i); !EOL; EOL=theAgent.getNextTerm(&i)){
        DEBl(count<<":[" << printInfon(i).c_str() << "]");
        EOT_d2=theAgent.StartTerm(i, &ItmPtr);
        int cmd=gINT;
        DEB("\n[CMD:"<< cmd << "]");
        switch(cmd){ // TODO: Pango, Audio, etc.
            case rectangle:DEB("rectangle:") Z4 cairo_rectangle (cr, a,b,c,d); break;
            case curvedRect:DEB("curvedRect:")  Z5 roundedRectangle(cr, a,b,c,d,e); break;
            case circle:  break;
            case lineTo: DEB("lineTo:")  Z2   cairo_line_to(cr, a,b);             break;
            case lineRel: DEB("lineRel:") Z2   cairo_rel_line_to(cr, a,b);         break;
            case moveTo: DEB("moveTo:")  Z2   cairo_move_to(cr, a,b);             break;
            case moveRel:  Z2   cairo_rel_move_to(cr, a,b);         break;
            case curveTo:  Z6   cairo_curve_to(cr, a,b,c,d,e,f);    break;
            case curveRel: Z6   cairo_rel_curve_to(cr, a,b,c,d,e,f);break;
            case arcClockWise:Z5 cairo_arc(cr, a,b,c,d,e);          break; //xc, yc, radius, angle1, angle2
            case arcCtrClockW:Z5 cairo_arc_negative(cr, a,b,c,d,e); break;
            case text: S1   cairo_show_text (cr, Sa); DEB("text")  break;

            case strokePath:   cairo_stroke(cr); DEB("strokePath")    break;
            case fillPath:     cairo_fill(cr); DEB("fillPath")      break;
            case paintSurface: cairo_paint(cr);      break;
            case closePath:    cairo_close_path(cr); break;

            case inkColor:DEB("inkColor:") Z3 cairo_set_source_rgba(cr, a, b, c, 1);  break;
            case inkGradient:  break;
            case inkImage:  break;
            case inkDrawing:  break;

            case lineWidth:DEB("lineWidth:")  Z1 cairo_set_line_width (cr, a); break;
            case lineStyle:  break;
            case fontFace:  break;
            case fontSize:  break;

            case drawToScrnN:  break;
            case drawToWindowN:  break;
            case drawToMemory:  break;
            case drawToPDF:  break;
            case drawToPS:  break;
            case drawToSVG:  break;
            case drawToPNG:  break;

            case shiftTo: Z2 cairo_translate(cr, a,b); break;
            case scale: Z2 cairo_scale(cr, a,b); break;
            case rotate: Z2 cairo_rotate(cr, a); break;

            case drawItem: DEB(">");
                subItem=gList();
             //   cairo_new_sub_path(cr); //cairo_push_group(cr);
                OldItmPtr=ItmPtr; DrawProteusDescription(surface, subItem); ItmPtr=OldItmPtr;
             //    cairo_close_path(cr); //cairo_pop_group_to_source (cr);
                //cairo_paint_with_alpha (cr, 1);
                DEB("<"); break;

            default: {ERRl("Invalid Drawing Command."); exit(2);}
        }
    if (++count==200) break;
    }
    cairo_destroy (cr);
//    DEBl("\nCount: " << count << "\n======================");
}
/////////////////// End of Slip Drawing, Begin Interface to Proteus Engine

struct InfonPortal {
    SDL_Window *window;
    SDL_Surface *surface;
    SDL_Renderer *renderer;
    cairo_surface_t *cairoSurf;
    infon *theme, *user, *topView, *crntView;
    char* title;
    bool needsToBeDrawn;
};

InfonPortal *portals[MAX_PORTALS];


int initModelsAndEventQueues(){ // TODO: Clean infon loading, use command line options, hard code some things
   // Load World
    MSGl("Loading world...");
    std::fstream InfonInW("world.pr");
//    std::string worldStr("<% eval=#+{{?{}, ?{}}, @\[?{}]: \[?{}, ?{}]} %> \n");
//    std::istringstream InfonInW(worldStr);
    QParser q(InfonInW);
extern infon *World; World=q.parse();
    if (World) {MSGl("["<<printInfon(World)<<"]");}
    else {ERRl("Error:"<<q.buf); exit(1);}
    theAgent.normalize(World);

    //Load Theme
    MSGl("Loading Theme...");
    std::fstream InfonInT("theme.pr");
    QParser T(InfonInT);
    Theme=T.parse();
    if (Theme) {MSGl("["<<printInfon(Theme)<<"]");}
    else {ERRl("Error:"<<T.buf); exit(1);}
   // theAgent.normalize(Theme);

// Load DispList
    MSGl("Loading display...");
    std::fstream InfonInD("display.pr");
//    std::string dispList("<% \n( {  @eval: { show_clock, +{*1+0 *15+0 *30+0} } /*{XXX}:<ref_to_myStuff>|...}*/ |...} ) %> \n");
//    std::string dispList("<% { @eval::{show_clock +{*1+0 *15+0 *30+0}} }  %> \n");
//    std::istringstream InfonInD(dispList);
    QParser D(InfonInD);
    infon* displayList=D.parse(); MSGl("parsed");
    if(displayList) {MSGl("["<<printInfon(displayList).c_str()<<"]");}
    else {ERRl("Error:"<<D.buf); exit(1);}

    theAgent.normalize(displayList);

  portals[0]->topView=displayList;
  portals[0]->needsToBeDrawn=true;

    MSGl("Normed");
    DEBl(printInfon(displayList));
}

/////////////////////////////////////////////

bool CreateTurbulancePortal(char* title, int x, int y, int w, int h, int flags, char* userName, char* themeFilename, char* stuffName){
    if(numPortals >= MAX_PORTALS) return 1;
    char winTitle[1024] = windowTitle;
    InfonPortal* portal=new InfonPortal;
    if (numPortals>0){SDL_snprintf(winTitle, strlen(title), "%s %d", title, numPortals+1);}
    else {strcpy(winTitle, title);}
    portal->window=SDL_CreateWindow(winTitle, x,y,w,h, flags);
//TODO: Hardcode: Title, icon, bpp-depth/vid_mode, resizable
//    SDL_SetWindowDisplayMode(portal->window, ); // Mode for use during fullscreen mode
//    LoadIcon()
    portal->renderer = SDL_CreateRenderer(portal->window, -1, SDL_RENDERER_ACCELERATED);
    portal->surface = SDL_CreateRGBSurface (0, w, h, 32,0x00ff0000,0x0000ff00,0x000000ff,0);
    SDL_SetRenderDrawColor(portal->renderer, 0xA0, 0xA0, 0xc0, 0xFF);
    portal->cairoSurf = cairo_image_surface_create_for_data((unsigned char*)portal->surface->pixels, CAIRO_FORMAT_RGB24, w, h,portal->surface->pitch);
    portals[numPortals]=portal; ++numPortals;

// TODO: Load independant Stuff and Theme for this window., Detach Theme from Functions.cpp:draw()
}

void DestroyTurbulancePortal(InfonPortal *p){
    //TODO slide portals items down
    cairo_surface_destroy(p->cairoSurf);
    SDL_FreeSurface(p->surface);
    SDL_DestroyRenderer(p->renderer);
    //SDL_DestroyTexture(p->tex);
}

//////////////////// End of Slip Specific Code, Begin Rendering Code
// TODO: Make resizing screens work. See example in testCairoSDL.cpp.
void ResizeScene(int Width,int Height){
   if (Height == 0) Height = 1;

   width  = Width;
   height = Height;
}

//////////////////// SDL Init and loop
int secondsToRun=10; // time until the program exits automatically. 0 = don't exit.
void EXIT(char* errmsg){ERRl(errmsg << "\n"); exit(1);}
void cleanup(void){SDL_Quit();}   //TODO: Add items to cleanup routine

static int SimThread(void *nothing){ // TODO: Make SimThread funtional
 //   initModelsAndEventQueues();
    SDL_Event ev;
    ev.type = SDL_USEREVENT;  ev.user.code = 0;  ev.user.data1 = 0;  ev.user.data2 = 0;
    int val, i = 0;
    while (!doneYet) {
        SDL_Delay(100);
        ev.user.data1 = (void *)i;
       // val = FE_PushEvent(&ev); // don't call this in the main thread
        ++num; ++i;
    }
  return 0;
}

void InitializePortalSystem(int argc, char** argv){
    srand(time(NULL));
    numPortals=0;
    char* userName="Demo_user"; char* theme="default"; char* portalContent="MyStuff";
    for (int i=1; i<argc;) { // Handle command line arguments
        int consumed = 0; //CommonArg(*SDL_state, i); // Handle common arguments
        if (consumed == 0) {
            consumed = -1;
            if (SDL_strcasecmp(argv[i], "--world") == 0) if (argv[i + 1]) {worldFile=argv[i]; consumed = 2;}
            if (SDL_strcasecmp(argv[i], "--theme") == 0) if (argv[i + 1]) {/*argv[i+1] is filename; consumed = 2; */ }
            if (SDL_strcasecmp(argv[i], "--user") == 0) if (argv[i + 1]) {/*argv[i+1] is username; consumed = 2; */ }
            if (SDL_strcasecmp(argv[i], "--pass") == 0) if (argv[i + 1]) {/*argv[i+1] is password; consumed = 2; */ }
            else if (SDL_isdigit(*argv[i])) {secondsToRun = SDL_atoi(argv[i]); consumed = 1;}
        }
        if (consumed < 0) {
            fprintf(stderr, "Usage: %s <seconds to run> [--world <filename>] [--theme <filename>] [--user <username>]\n", argv[0]);
            exit(1);
        }
        i += consumed;
    }
    if (SDL_VideoInit(NULL) < 0) {MSGl("Couldn't initialize Video Driver: "<<SDL_GetError()); exit(2);}
    if (SDL_AudioInit(NULL) < 0) {MSGl("Couldn't initialize Audio Driver: "<<SDL_GetError()); exit(2);}
 //   if (SDL_TimerInit() < 0) {MSGl("Couldn't initialize Timer Driver: "<<SDL_GetError()); exit(2);}

    //TODO: if (DebugMode) PrintConfiguration from common.c Set debugmode via command-line argument
    CreateTurbulancePortal(windowTitle, 100,120,800,480, 0, userName, theme, portalContent);

    //TODO: Enable Key Repeat and Unicode support in SDL. (See Harfbuzz.c) : SDL_EnableKeyRepeat(300, 130);  SDL_EnableUNICODE(1)
    atexit(cleanup);
}
enum userActions {TURB_UPDATE_SURFACE=1, TURB_ADD_SCREEN, TURB_DEL_SCREEN, TURB_ADD_WINDOW, TURB_DEL_WINDOW};

void StreamEvents(){
    Uint32 frames=0, then=SDL_GetTicks(), now=0, limit = secondsToRun*1000;
//    SDL_Thread *simulationThread = SDL_CreateThread(SimThread, "Proteus", NULL);
    initModelsAndEventQueues();
    SDL_Event ev;
    while (secondsToRun && (SDL_GetTicks() < (then+limit)) && !doneYet){
        --num; ++frames;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_USEREVENT:
                switch(ev.user.code){
                    case TURB_UPDATE_SURFACE:
                      portals[(uint)ev.user.data1]->topView=(infon*)ev.user.data2;
                      portals[(uint)ev.user.data1]->needsToBeDrawn=true;
                      break;
                    case TURB_ADD_SCREEN: break;
                    case TURB_DEL_SCREEN: break;
                    case TURB_ADD_WINDOW: break;
                    case TURB_DEL_WINDOW: break;
                }
                break;
            case SDL_WINDOWEVENT: break; // e.g., close
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_PRINTSCREEN: break; // see sdl_common.c
                case SDLK_c: if (ev.key.keysym.mod & KMOD_CTRL) {SDL_SetClipboardText("SDL rocks!\nYou know it!");} break;
                case SDLK_v: break;       // paste. see sdl_common.c
                case SDLK_g: break;       // Grab Input. see sdl_common.c
                case SDLK_RETURN: break;  // Toggle fullscreen. see sdl_common.c
                case SDLK_n: break;      // New Portal
                }
            case SDL_QUIT: doneYet=true; break;

            }
        }
        for (int i = 0; i < numPortals; ++i) {
            InfonPortal* portal=portals[i];
            if(! portal->needsToBeDrawn) continue;
            SDL_Renderer *renderer = portal->renderer;
            SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xc0, 0xFF);

            DrawProteusDescription(portal->cairoSurf, portal->topView);

            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer,portal->surface);
        //   SDL_Texture *tex = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB888,SDL_TEXTUREACCESS_STREAMING,width,height);
        //   SDL_LockTexture(tex,NULL,sdl_surface->pixels,sdl_surface->pitch);
        //   SDL_UpdateTexture(tex, NULL, sdl_surface->pixels, sdl_surface->pitch);
        //   SDL_UnlockTexture(tex);
        //   SDL_RenderClear(renderer);

            SDL_RenderCopy(renderer, tex, NULL, NULL);
            SDL_RenderPresent(renderer);
            SDL_DestroyTexture(tex);
        }
    }
    doneYet=true;
    if ((now = SDL_GetTicks()) > then) printf("\nFrames per second: %2.2f\n", (((double) frames * 1000) / (now - then)));

 //   SDL_WaitThread(simulationThread, NULL);
}

int main(int argc, char *argv[]){
    InitializePortalSystem(argc, argv);
    StreamEvents();
    return (0);
}
