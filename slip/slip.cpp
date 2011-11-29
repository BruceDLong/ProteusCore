// The SDL SlipStream Engine. Copyright 2008-2011 by Bruce Long
/*  This file is part of the "SlipStream Engine"
    The SlipStream Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The SlipStream Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the SlipStream Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

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
static int num = 0;

static CommonState *SDL_state;
SDL_Surface *screen;
TTF_Font *font = NULL; // TODO: Are these 8 lines needed?
SDL_Color textColor = { 128, 128, 128 };
SDL_Color hiColor = { 255, 255, 255 };
SDL_Color loColor = { 0, 0, 0 };

int bg_red = 0;
int bg_green = 0;
int bg_blue = 0;

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
#define gZto1 (float)(((float)gInt()) / 1024.0)
#define gSTR gStr()

#define Z2 {a=gZto1; b=gZto1;}
#define Z3 {a=gZto1; b=gZto1; c=gZto1;}
#define Z4 {a=gZto1; b=gZto1; c=gZto1; d=gZto1;}
#define Z5 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; e=gZto1;}
#define Z6 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; e=gZto1; f=gZto1;}
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

int getColorComponents(infon* color, int* red, int* green, int* blue, int* alpha){
DEBl ("The Color is:" << printInfon(color));
    color=color->value; *red=(int)color->value;
    color=color->next; *green=(int)color->value;
    color=color->next; *blue=(int)color->value;
    color=color->next; *alpha=(int)color->value;
}

void DrawRects(SDL_Renderer * renderer)
{
    int i;
    SDL_Rect rect;
    SDL_Rect viewport;

    /* Query the sizes */
    SDL_RenderGetViewport(renderer, &viewport);

    for (i = 0; i < 8 / 4; ++i) {

        SDL_SetRenderDrawColor(renderer, rand() % 256, rand() % 256, rand() % 256, (Uint8) 255);

        rect.w = rand() % (viewport.w / 2);
        rect.h = rand() % (viewport.h / 2);
        rect.x = (rand() % (viewport.w*2) - viewport.w) - (rect.w / 2);
        rect.y = (rand() % (viewport.h*2) - viewport.h) - (rect.h / 2);
        SDL_RenderFillRect(renderer, &rect);
    }
}

static inline int show_message(SDL_Surface* scn, int x, int y, const char *msg){ // TODO: Integrate Pango
    SDL_Surface *message = NULL;
    SDL_Rect offset;
    offset.x = x - 1; // >
    offset.y = y - 1; // V

    message = TTF_RenderText_Solid(font, msg, hiColor);
    SDL_BlitSurface(message, NULL, scn, &offset );

    SDL_FreeSurface(message);
    message = TTF_RenderText_Solid(font, msg, loColor);
    offset.x += 2;
    offset.y += 2;
    SDL_BlitSurface(message, NULL, scn, &offset );

    SDL_FreeSurface(message);
    message = TTF_RenderText_Solid(font, msg, textColor);
    offset.x--;
    offset.y--;
    SDL_BlitSurface(message, NULL, scn, &offset );

    SDL_FreeSurface(message);

    return(1);
}

static inline void add_msg(SDL_Surface* scn, const char *msg){
    show_message(scn, 10, 50, msg);
}

enum dTools{drawItem=0, rectangle, curvedRect, circle, lineTo, lineRel, moveTo, moveRel, curveTo, curveRel, arcClockWise, arcCtrClockW, text,
            strokePath=16, fillPath, paintSurface, closePath,
            inkColor=20, inkGradient, inkImage, inkDrawing,
            lineWidth=25, lineStyle, fontFace, fontSize,
            drawToScrnN=30, drawToWindowN, drawToMemory, drawToPDF, drawToPS, drawToSVG, drawToPNG
};

void DrawProteusDescription(SDL_Surface* scn, infon* ProteusDesc){
    if (scn==0 || ProteusDesc==0 || ProteusDesc->size==0) ERRl("Missing description.");
    DEBl("DISPLAY:["<<printInfon(ProteusDesc));
    int count=0;
    infon *i, *OldItmPtr;
    int size, size2; float a,b,c,d,e,f; int Ia,Ib,Ic,Id,Ie,If,Ih,II; char  *Sa, *Sb;
    for(int EOL=theAgent.StartTerm(ProteusDesc, &i); !EOL; EOL=theAgent.getNextTerm(&i)){
        DEBl(count<<":[" << printInfon(i).c_str() << "]");
        EOT_d2=theAgent.StartTerm(i, &ItmPtr);
        //infon* args=gList();
        int cmd=gINT;
        DEB("\n[CMD:"<< cmd << "]");
        switch(cmd){ // TODO: Add more commands: Cairo, Pango, Audio, etc.
            case drawItem: cairo_push_group(cr); OldItmPtr=ItmPtr; DrawProteusDescription(scn, OldItmPtr); ItmPtr=OldItmPtr; cairo_pop_group_to_source (cr); break;
            case rectangle: cairo_rectangle (cr, 0, 0, 1, 1); break;
            case curvedRect:  break;
            case circle:  break;
            case lineTo:   Z2   cairo_line_to(cr, a,b);             break;
            case lineRel:  Z2   cairo_rel_line_to(cr, a,b);         break;
            case moveTo:   Z2   cairo_move_to(cr, a,b);             break;
            case moveRel:  Z2   cairo_rel_move_to(cr, a,b);         break;
            case curveTo:  Z6   cairo_curve_to(cr, a,b,c,d,e,f);    break;
            case curveRel: Z6   cairo_rel_curve_to(cr, a,b,c,d,e,f);break;
            case arcClockWise:Z5 cairo_arc(cr, a,b,c,d,e);          break; //xc, yc, radius, angle1, angle2
            case arcCtrClockW:Z5 cairo_arc_negative(cr, a,b,c,d,e); break;
            case text: I2 S1  show_message(scn, Ia, Ib, Sa); break;

            case strokePath:   cairo_stroke(cr);     break;
            case fillPath:     cairo_fill(cr);       break;
            case paintSurface: cairo_paint(cr);      break;
            case closePath:    cairo_close_path(cr); break;

            case inkColor: cairo_set_source_rgb(cr, 0, 0, 0); break;
            case inkGradient:  break;
            case inkImage:  break;
            case inkDrawing:  break;

            case lineWidth: cairo_set_line_width (cr, 0.1); break;
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

            default: ERRl("Invalid Drawing Command.");
        }
    if (++count==90) break;
    }
//    DEBl("\nCount: " << count << "\n======================");
}
/////////////////// End of Slip Drawing, Begin Interface to Proteus Engine
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

//  protScreens[0].SurfaceForThisScreen=&protSurfaces[0];
//  protScreens[0].screen=screen;
//  protSurfaces[0].frameInfon=displayList;

    MSGl("Normed");
    DEBl(printInfon(displayList));
}

/////////////////////////////////////////////
// TODO: Is this (DisplayItem) needed?
class DisplayItem {
    uint surfaceID;
    uint itemID;
    uint locX, locY, rotate, scale, alpha;
    bool hidden;
    uint maxRect;
    uint ItemList;
    bool changedFlag;

    void ProcessStateChanges(infon* delta);
    // commands: Select Item or Primative by it's itemID; created if the itemID is new. Delete
    //        Set: loc, rot, scale, alpha, hidden. Set screen
    //         In ItemList, go: home, end. Insert before, after, move: to-front, to-back, up, down. Clear list
};

void DisplayItem::ProcessStateChanges(infon* delta){

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
    initModelsAndEventQueues();
    // TODO: Load fonts etc. Init freetype, cairo, pango, etc.
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

void setupSDL(CommonState** SDL_state, int argc, char** argv){

    if(!(*SDL_state = CommonCreateState(argv, /*SDL_INIT_AUDIO|*/SDL_INIT_VIDEO))) EXIT((char*)"Couldn't Create SDL State"); //TODO: InitAudio
    for (int i=1; i<argc;) { // Handle command line arguments
        int consumed = CommonArg(*SDL_state, i); // Handle common arguments
        if (consumed == 0) {
            consumed = -1;
            if (SDL_strcasecmp(argv[i], "--world") == 0) if (argv[i + 1]) {/*argv[i+1] is filename; consumed = 2; */ }
            if (SDL_strcasecmp(argv[i], "--theme") == 0) if (argv[i + 1]) {/*argv[i+1] is filename; consumed = 2; */ }
            if (SDL_strcasecmp(argv[i], "--user") == 0) if (argv[i + 1]) {/*argv[i+1] is username; consumed = 2; */ }
            if (SDL_strcasecmp(argv[i], "--pass") == 0) if (argv[i + 1]) {/*argv[i+1] is password; consumed = 2; */ }
            else if (SDL_isdigit(*argv[i])) {secondsToRun = SDL_atoi(argv[i]); consumed = 1;}
        }
        if (consumed < 0) {
            fprintf(stderr, "Usage: %s %s [--blend none|blend|add|mod] [--cyclecolor] [--cyclealpha]\n", argv[0], CommonUsage(*SDL_state));
            exit(1);
        }
        i += consumed;
    }

    //TODO: Hardcode: Title, icon, bpp-depth/vid_mode, resizable
    if (!CommonInit(*SDL_state)) {exit(2);}

    for (i = 0; i < (*SDL_state)->num_windows; ++i) {  // Create the windows and initialize the renderers
        SDL_Renderer *renderer = (*SDL_state)->renderers[i];
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
        SDL_RenderClear(renderer);
    }

    //TODO: Enable Key Repeat and Unicode support in SDL. (See Harfbuzz.c) : SDL_EnableKeyRepeat(300, 130);  SDL_EnableUNICODE(1)
    // SDL_InitTimer(); TODO: Init and Quit for SDL_Timer.
    TTF_Init();
    atexit(cleanup);
    srand((unsigned int)time(NULL));
}
enum userActions {TURB_UPDATE_SURFACE=1, TURB_ADD_SCREEN, TURB_DEL_SCREEN, TURB_ADD_WINDOW, TURB_DEL_WINDOW};

void eventLoop(CommonState* SDL_state){
    Uint32 frames=0, then=SDL_GetTicks(), now=0, limit = secondsToRun*1000;
    SDL_Thread *simulationThread = SDL_CreateThread(SimThread, "Proteus", NULL);
    SDL_Event ev;
    while (secondsToRun && (SDL_GetTicks() < (then+limit)) && !doneYet){
        --num; ++frames;
        while (SDL_PollEvent(&ev)) {
            CommonEvent(SDL_state, &ev, &doneYet);
            switch (ev.type) {
            case SDL_USEREVENT:
                switch(ev.user.code){
                    case TURB_UPDATE_SURFACE:
                //      protSurfaces[(uint)ev.user.data1].frameInfon=(infon*)ev.user.data2;
                //      protSurfaces[(uint)ev.user.data1].needsToBeDrawn=true;
                        break;
                    case TURB_ADD_SCREEN: break;
                    case TURB_DEL_SCREEN: break;
                    case TURB_ADD_WINDOW: break;
                    case TURB_DEL_WINDOW: break;
                }
                break;
            }
        }
        for (int i = 0; i < SDL_state->num_windows; ++i) {
            SDL_Renderer *renderer = SDL_state->renderers[i];
            SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xc0, 0xFF);

            SDL_RendererInfo RenInfo;
            SDL_GetRendererInfo(renderer, &RenInfo);
            int width=RenInfo.max_texture_width; int height=RenInfo.max_texture_height;

            SDL_Surface *sdl_surface = SDL_CreateRGBSurface (0, width, height, 32,0x00ff0000,0x0000ff00,0x000000ff,0);
            SDL_FillRect(sdl_surface,NULL,SDL_MapRGB(sdl_surface->format, 30,250,30));
            SDL_Texture *tex = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB888,SDL_TEXTUREACCESS_STREAMING,width,height);
            SDL_UpdateTexture(tex, NULL, sdl_surface->pixels, sdl_surface->pitch);
            SDL_FreeSurface(sdl_surface);
            //DrawProteusDescription(scn->screen, scn->SurfaceForThisScreen->frameInfon);
            // TODO: Here is where to blit from surface to texture if needed
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, tex, NULL, NULL);
         //   DrawRects(renderer);
            SDL_RenderPresent(renderer);
        }
    }

    now = SDL_GetTicks();  // Print out some timing information
    if (now > then) printf("%2.2f frames per second\n", (((double) frames * 1000) / (now - then)));

    SDL_WaitThread(simulationThread, NULL); // wait for it to die
}

int main(int argc, char *argv[]){
    setupSDL(&SDL_state, argc, argv);
    eventLoop(SDL_state);
    return (0);
}
