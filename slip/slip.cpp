// The SDL SlipStream Engine. Copyright 2008 by Bruce Long
/*    This file is part of the "SlipStream Engine"

    The SlipStream Engine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The SlipStream Engine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the SlipStream Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
//#define OUT(msg) {std::cout<< msg;}
//#define DEB(msg) {std::cout<< msg << "\n";}

// GraphicsEngine: OPENGL, GLES, DIRECTX
// CPU: Ix86, ARMEH, ARMEL
// OpSys: LINUX, MAC, WINDOWS
#define GraphicsEngine_NONE
#define CPU_ARMEL
#define OpSys_LINUX

#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "fastevents.h"
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>

#ifdef GraphicsEngine_OPENGL
#include "SDLUtils.h"
#include <GL/gl.h>
#include <GL/glu.h>
#elif defined GraphicsEngine_GLES
#include <SDL/SDL_gles.h>
#include <GLES2/gl2.h>
#elif GraphicsEngine_DIRECTX

#endif


enum {LINUX, MACINTOSH, WINDOWS}

const QPLATFORM=LINUX;

int height, width, bpp;
static int doneYet = 0;
static int num = 0;

SDL_Surface *screen = NULL;
TTF_Font *font = NULL;
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

#define gINT gInt()
#define gZto1 (float)(((float)gInt()) / 1024.0)
#define gSTR gStr()

infon* ProteusDesc=0;

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
    getInt(ItmPtr,j,sign);  // std::cout << j << ", ";
    theAgent.getNextTerm(&ItmPtr);
    return (sign)?-j:j;
}

char* gStr() {
	if((ItmPtr->flags&tType)==tString && !(ItmPtr->flags&fUnknown)) {
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

static inline uint32_t map_value(uint32_t val, uint32_t max, uint32_t tomax)
{
	return((uint32_t)((double)val * (double)tomax/(double)max));
}

static inline SDL_Surface *load_image(const char *filename)
{
	SDL_Surface* loadedImage = NULL;
	SDL_Surface* optimizedImage = NULL;

	if((loadedImage = IMG_Load(filename))) {
		optimizedImage = SDL_DisplayFormat(loadedImage);
		SDL_FreeSurface(loadedImage);
	}
	return optimizedImage;
}

int getColorComponents(infon* color, int* red, int* green, int* blue, int* alpha){
std::cout << "The Color is:" << printInfon(color) << "\n";
    color=color->value; *red=(int)color->value;
    color=color->next; *green=(int)color->value;
    color=color->next; *blue=(int)color->value;
    color=color->next; *alpha=(int)color->value;
}

static inline int show_message(int x, int y, const char *msg)
{
	SDL_Surface *message = NULL;
	SDL_Rect offset;
	offset.x = x - 1; // >
	offset.y = y - 1; // V

	message = TTF_RenderText_Solid(font, msg, hiColor);
	SDL_BlitSurface(message, NULL, screen, &offset );

	SDL_FreeSurface(message);
	message = TTF_RenderText_Solid(font, msg, loColor);
	offset.x += 2;
	offset.y += 2;
	SDL_BlitSurface(message, NULL, screen, &offset );

	SDL_FreeSurface(message);
	message = TTF_RenderText_Solid(font, msg, textColor);
	offset.x--;
	offset.y--;
	SDL_BlitSurface(message, NULL, screen, &offset );

	SDL_FreeSurface(message);

	return(1);
}

static inline void add_msg(const char *msg)
{
	show_message(10, 50, msg);
}

static inline void draw_rect(infon* color, int x, int y, int size_x, int size_y)
{
    	int red,green,blue,alpha;
    	getColorComponents(color, &red, &green, &blue, &alpha);
	boxRGBA(screen, x, y, size_x+x, size_y+y, red, green, blue, alpha);
}

static inline void draw_RelLine(infon* color, int x, int y, int size_x, int size_y)
{
    	int red,green,blue,alpha;
    	getColorComponents(color, &red, &green, &blue, &alpha);
	lineRGBA(screen, x, y, size_x/15+x, size_y/15+x, red, green, blue, alpha);
}
static inline void draw_line(infon* color, int x, int y, int size_x, int size_y)
{
    	int red,green,blue,alpha;
    	getColorComponents(color, &red, &green, &blue, &alpha);
	lineRGBA(screen, x, y, size_x, size_y, red, green, blue, alpha);
}

static inline void draw_round(infon* color, int x, int y, int size_x, int size_y, int corner)
{
        int red,green,blue,alpha;
       getColorComponents(color, &red, &green, &blue, &alpha);
	roundedBoxRGBA(screen, x, y, size_x+x, size_y+y, corner, red, green, blue, alpha);
}

static inline void draw_circle(infon* color, int x, int y, int radius)
{
    	int red,green,blue,alpha;
       getColorComponents(color, &red, &green, &blue, &alpha);
	if(radius < x && radius < y) {
		filledCircleRGBA(screen, x, y, radius, red, green, blue, alpha);
	}
}

void DrawProteusDescription(infon* ProteusDesc){
if (ProteusDesc==0) std::cout << "Err1\n";
if (ProteusDesc->size==0) std::cout << "Err2\n";
std::cout<<"DISPLAY:["<<printInfon(ProteusDesc)<<"]\n"; 
    int count=0;
    infon* OldItmPtr;
int size, size2; float a,b,c,d; int Ia,Ib,Ic,Id,Ie,If,Ih,II; char  *Sa, *Sb;
int EOT_d1=0; infon* i=0;
EOT_d1=theAgent.StartTerm(ProteusDesc, &i);
while(!EOT_d1){
  std::cout << count<<":[" << printInfon(i).c_str() << "]\n";
        EOT_d2=theAgent.StartTerm(i, &ItmPtr);
        //std::cout<<"ival["<<ItmPtr<<"]\n";
        infon* args=gList();
        int cmd=gINT;
std::cout<<"\n[CMD:"<< cmd << "]";
        switch(cmd){
            case 0:  		OldItmPtr=ItmPtr; DrawProteusDescription(OldItmPtr); ItmPtr=OldItmPtr;
            case 1: I4 		draw_rect(args, Ia, Ib, Ic, Id);  break;
            case 2: I5 		draw_round(args, Ia, Ib, Ic, Id, Ie);  break;
            case 3: I3		draw_circle(args, Ia, Ib, Ic);  break;
            case 4: I4 		draw_RelLine(args, Ia, Ib, Ic, Id);  break;
            case 5: I4 		draw_line(args, Ia, Ib, Ic, Id);  break;
            case 10: I2 S1	show_message(Ia, Ib, Sa);  break;
        }
    if (++count==90) break;
    EOT_d1=theAgent.getNextTerm(&i);
    }
//    std::cout << "\nCount: " << count << "\n======================\n";
}
//////////////////// End of Slip Drawing, Begin Interface to Proteus Engine
int initWorldAndEventQueues(){
    // Load World
    std::cout << "Loading world\n";
    std::fstream InfonInW("world.pr");
//    std::string worldStr("<% eval=#+{{?{}, ?{}}, @\[?{}]: \[?{}, ?{}]} %> \n");
//    std::istringstream InfonInW(worldStr);
    QParser q(InfonInW);
extern infon *World; World=q.parse();
    if (World) std::cout<<"["<<printInfon(World)<<"]\n";
    else {std::cout<<"Error:"<<q.buf<<"\n"; exit(1);}
    theAgent.normalize(World);

    //Load Theme
    std::cout << "Loading theme\n";
    std::fstream InfonInT("theme.pr");
    QParser T(InfonInT);
    Theme=T.parse();
    if (Theme) std::cout<<"["<<printInfon(Theme)<<"]\n";
    else {std::cout<<"Error:"<<T.buf<<"\n"; exit(1);}
   // theAgent.normalize(Theme);

// Load DispList
    std::cout << "Loading display\n";
    std::fstream InfonInD("display.pr");
//    std::string dispList("<% \n( {  @eval: { show_clock, +{*1+0 *15+0 *30+0} } /*{XXX}:<ref_to_myStuff>|...}*/ |...} ) %> \n");
//    std::string dispList("<% { @eval::{show_clock +{*1+0 *15+0 *30+0}} }  %> \n");
//    std::istringstream InfonInD(dispList);
    QParser D(InfonInD);
    infon* displayList=D.parse(); std::cout << "parsed\n";
    if(displayList) std::cout<<"["<<printInfon(displayList).c_str()<<"]\n";
    else {std::cout<<"Error:"<<D.buf<<"\n"; exit(1);}

    theAgent.normalize(displayList);
    DEB("Normed.");
    std::cout<< printInfon(displayList) << "\n";
    ProteusDesc=displayList;
}

//////////////////// End of Slip Specific Code, Begin OpenGL Code
#define FRAME_RATE_SAMPLES 50
int FrameCount=0;
float FrameRate=0;

static void ourDoFPS(){
   static clock_t last=0;
   clock_t now;
   float delta;

   if (++FrameCount >= FRAME_RATE_SAMPLES) {
      now  = clock();
      delta= (now - last) / (float) CLOCKS_PER_SEC;
      last = now;

      FrameRate = FRAME_RATE_SAMPLES / delta;
      FrameCount = 0;
   }
}

void DrawScreen(){
   DrawProteusDescription(ProteusDesc);
    SDL_Flip(screen); // SDL_GL_SwapBuffers( );// All done drawing.  Let's show it.

 //   X_Rot+=X_Speed; Y_Rot+=Y_Speed; // Now let's do the motion calculations.
std::cout<<"."<<std::flush;
    ourDoFPS();// And collect our statistics.
    }

void ResizeScene(int Width,int Height){
   if (Height == 0) Height = 1;

   width  = Width;
   height = Height;
}

//////////////////// SDL Init and loop
void EXIT(char* errmsg){std::cout << errmsg << "\n\n";exit(1);}
void cleanup(void){FE_Quit(); SDL_Quit();}

static int runThread(void *nothing){
    int i;
    int val;
    SDL_Event ev;

    ev.type = SDL_USEREVENT;  ev.user.code = 0;  ev.user.data1 = 0;  ev.user.data2 = 0;
    i = 0;
    while (!doneYet) {
        SDL_Delay(100);
        ev.user.data1 = (void *)i;
        val = FE_PushEvent(&ev); // don't call this in the main thread
        num++;
      //  DEB("Sending Event:" << i);
        i++;
    }
  return 0;
}

void setupSDL(){
    // This "FastEvent" code won't work on pre OSX versions of Macintosh.
    int InitSDLEventThread=0;
    if (QPLATFORM==LINUX || QPLATFORM==MACINTOSH) InitSDLEventThread=SDL_INIT_EVENTTHREAD;

    SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE|InitSDLEventThread);
    TTF_Init();
    FE_Init();
    atexit(cleanup);

    /// SELECT VIDEO MODE AND INIT OPENGL
    SDL_Rect** modes=SDL_ListModes(NULL, 0);
    if (!modes) {DEB( SDL_GetError()); EXIT((char*)"No SDL Modes available");}
    if (modes == (SDL_Rect**)-1){width=1024; height=600;}  //{width=800; height=480;}
    else {width=modes[0]->w; height=modes[0]->h;}

    bpp=SDL_VideoModeOK(width, height, 16 ,0);

    const SDL_VideoInfo* video = SDL_GetVideoInfo();
    bpp=video->vfmt->BitsPerPixel;
    bpp=16;
    DEB("bpp: "<<bpp<<"  w:"<<width<<"  h:"<<height<<" mode:"<<modes);

    // Create a (hopefully) matching screen buffer
    screen=SDL_SetVideoMode( width, height, bpp, SDL_SWSURFACE /*SDL_OPENGL|SDL_FULLSCREEN*/ );
    if (screen == 0 ) {
        DEB( SDL_GetError());
        EXIT((char*)"Couldn't set video mode");
    }

    	SDL_WM_SetCaption("SlipStream", NULL);
	if(!(font = TTF_OpenFont("FreeSans.ttf", 32))) {
		printf("Can't open font.\n");
		exit(EXIT_FAILURE);
	}
}

void eventLoop(int argc, char** argv){
    int count=0, start=0, end=0, limit = 10*1000;
    double rate=0.0;
    if (2 <= argc) limit = 1000 * atoi(argv[1]);

    SDL_Thread *thread = SDL_CreateThread(runThread, NULL);
    SDL_Event ev;
    start = SDL_GetTicks();
    while ((SDL_GetTicks() < (start + limit)) && FE_WaitEvent(&ev)){
        num--;
        // DEB("Got: " << num);
        // DEB ("Time: " << SDL_GetTicks());
        switch (ev.type) {
        case SDL_USEREVENT: // Handle user events here.
            // DEB("count: " << count << "   event.data1: " << (int)ev.user.data1);
            count++;
            break;
        case SDL_KEYDOWN: // Handle any key presses here.
            switch(ev.key.keysym.sym){
            case SDLK_ESCAPE: exit(0); break;
            case SDLK_SPACE: exit(0);
                break;
               case SDLK_KP_PLUS:
                   break;
               case SDLK_KP_MINUS:
                   break;
            default:
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN: // Handle mouse clicks here.
               break;
        case SDL_QUIT: EXIT ((char*)"SDL Quit Event: Forced Exit?."); break;
        default:; // printSDLEvent(&ev); break;
       }
       DrawScreen();
      }
      doneYet = 1;

    while (FE_PollEvent(&ev)){} // drain the que
    SDL_WaitThread(thread, NULL); // wait for it to die
    end = SDL_GetTicks();
    rate = ((double)count) / (((double)(end - start)) / 1000.0);
    DEB("Events/Second:" << rate);
}

int main(int argc, char **argv){
    initWorldAndEventQueues();
    setupSDL();
  //  setupOpenGL();
    eventLoop(argc, argv);
    return (0);
}
