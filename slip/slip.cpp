// The SDL SlipStream Engine. Copyright 2008-2011 by Bruce Long
/*  This file is part of the "SlipStream Engine"
    The SlipStream Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The SlipStream Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the SlipStream Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#define windowTitle "Turbulance"
#define MAX_PORTALS 24

#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_image.h>

#include <pango/pangocairo.h>

enum {LINUX, MACINTOSH, WINDOWS, MEEGO, IOS, ANDROID, RIM, PALM, XBOX, PS3, WII, NOOK, KINDLE, WINCE};

static int doneYet=0, numEvents=0, numPortals=0;

//////////////////// Slip Code
#include <strstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "../core/Proteus.h"

using namespace std;

#define DEB(msg)  //{cout<< msg;}
#define DEBl(msg) //{cout<< msg << "\n";}
#define MSG(msg)  {cout<< msg << flush;}
#define MSGl(msg) {cout<< msg << "\n" << flush;}
#define ERR(msg)  {cout<< msg;}
#define ERRl(msg) {cout<< msg << "\n";}

#define gINT (theAgent.gIntNxt(&ItemPtr).get_ui())
#define gZto1 (theAgent.gRealNxt(&ItemPtr))
#define gSTR theAgent.gStrNxt(&ItemPtr, txtBuff)

#define Z1 {a=gZto1;  DEB(a)}
#define Z2 {a=gZto1; b=gZto1; DEB(a<<", "<<b)}
#define Z3 {a=gZto1; b=gZto1; c=gZto1; DEB(a<<", "<<b<<", "<< c)}
#define Z4 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; DEB(a<<", "<<b<<", "<< c<<", "<< d)}
#define Z5 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; e=gZto1; DEB(a<<", "<<b<<", "<< c<<", "<< d<<", "<< e)}
#define Z6 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; e=gZto1; f=gZto1; DEB(a<<", "<<b<<", "<< c<<", "<< d<<", "<< e<<", "<< f)}
#define I2 {Ia=gINT; Ib=gINT;}
#define I3 {Ia=gINT; Ib=gINT; Ic=gINT;}
#define I4 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT;}
#define I5 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT;}
#define I6 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT; If=gINT;}
#define I7 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT; If=gINT; Ih=gINT;}
#define I8 {Ia=gINT; Ib=gINT; Ic=gINT;  Id=gINT; Ie=gINT; If=gINT; Ih=gINT; II=gINT;}
#define S1 {Sa=gSTR;}

infon* debugInfon=0; // Assign this to an infon to assist in debugging.

int AutoEval(infon* CI, agent* a);
bool IsHardFunc(string tag);
agent theAgent(0, IsHardFunc, AutoEval);

struct User {
    string name;
    string defaultTheme;
    string password;
    string languages;
    string preferredLanguage;
    infon* location;
    string myStuff;
  //  ThemePrefs* myThemePreferences;
};

bool loadUserRecord(User* user, char* username, char* password){
    if (strcmp(username, "bruce")==0){
        user->name="bruce"; user->defaultTheme="darkTheme1.pr"; user->password="erty"; user->preferredLanguage="en"; user->myStuff="brucesStuff.pr";
    } else if (strcmp(username, "xander")==0){
        user->name="xander"; user->defaultTheme="lightTheme1.pr"; user->password="erty"; user->preferredLanguage="en"; user->myStuff="xandersStuff.pr";
    } else return 1;
    if (user->password.compare(password)==0){
        return 0;
    } else return 1;
}

struct InfonPortal; //forward
struct InfonViewPort {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool isMinimized;
    int posX, posY; // location in parent window.
    double scale; // MAYBE: Make views scalable?
    InfonPortal *parentPortal;
    InfonViewPort *next;
};

struct InfonPortal {
    InfonPortal(){memset(this, 0, sizeof(InfonPortal));};
    ~InfonPortal(){delete surface; };
    SDL_Surface *surface;
    cairo_surface_t *cairoSurf;
    cairo_t *cr;
    SDL_Surface *SDL_background;
    cairo_surface_t *cairo_background;
    infon *theme, *stuff, *crntFrame;
    User *user;
    bool needsToBeDrawn, viewsNeedRefactoring, isLocked;
    InfonViewPort *viewPorts;
};

InfonPortal *portals[MAX_PORTALS];

/*
int getArrayi(int* array){
    int size=gInt();
    for (int i=0; i<size; ++i){
        array[i]=gInt();
    }
    return size;
}

int getArrayz(double* array){ // gets an array of 0..1 values (Zero)
    int size=gInt();
    for (int i=0; i<size; ++i){
        array[i]=gZto1;
    }
    return size;
}
*/
static inline uint32_t map_value(uint32_t val, uint32_t max, uint32_t tomax){
    return((uint32_t)((double)val * (double)tomax/(double)max));
}

static inline SDL_Surface *load_image(const char *filename){
    SDL_Surface* loadedImage = NULL;
  //  SDL_Surface* optimizedImage = NULL;

    if((loadedImage = IMG_Load(filename))) {
       // optimizedImage = SDL_DisplayFormat(loadedImage);
      //  SDL_FreeSurface(loadedImage);
    }
    return loadedImage; //optimizedImage;
}

SDL_Texture *LoadTexture(SDL_Renderer *renderer, char *file, SDL_bool transparent){
    SDL_Surface *temp=IMG_Load(file);;
    SDL_Texture *texture;

    if (temp == NULL) {fprintf(stderr, "Couldn't load %s: %s", file, SDL_GetError()); return NULL;}

    if (transparent) { //Set transparent pixel as the pixel at (0,0)
        if (temp->format->palette) {
            SDL_SetColorKey(temp, SDL_TRUE, *(Uint8 *) temp->pixels);
        } else {
            switch (temp->format->BitsPerPixel) {
            case 15: SDL_SetColorKey(temp, SDL_TRUE, (*(Uint16 *) temp->pixels) & 0x00007FFF); break;
            case 16: SDL_SetColorKey(temp, SDL_TRUE, *(Uint16 *) temp->pixels); break;
            case 24: SDL_SetColorKey(temp, SDL_TRUE, (*(Uint32 *) temp->pixels) & 0x00FFFFFF); break;
            case 32: SDL_SetColorKey(temp, SDL_TRUE, *(Uint32 *) temp->pixels); break;
            }
        }
    }

    texture = SDL_CreateTextureFromSurface(renderer, temp);
    if (!texture) {fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError()); SDL_FreeSurface(temp);  return NULL; }
    SDL_FreeSurface(temp);
    return texture;
}

string fontDescription="Sans Bold 12";

void renderText(cairo_t *cr, char* text){
    PangoLayout *layout=pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text, -1);

    PangoFontDescription *desc = pango_font_description_from_string(fontDescription.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    cairo_set_line_width(cr, 0.5);
    pango_cairo_update_layout(cr, layout);
    pango_cairo_layout_path(cr, layout);
    g_object_unref(layout);
}

void CreateLinearGradient(cairo_t *cr,double x1, double y1, double x2, double y2, infon* colorStops=0){
    cairo_pattern_t *pattern=0;
    pattern=cairo_pattern_create_linear(x1,y1,x2,y2);
    cairo_set_source(cr, pattern);
    cairo_pattern_destroy(pattern);
}

void CreateRadialGradient(cairo_t *cr,double cx1, double cy1, double r1, double cx2, double cy2, double r2, infon* colorStops=0){
    cairo_pattern_t *pattern=0;
    pattern=cairo_pattern_create_radial(cx1,cy1,r1,cx2,cy2,r2);
    cairo_set_source(cr, pattern);
    cairo_pattern_destroy(pattern);
}

void AddColorStop(cairo_t *cr, double offset, double r, double g, double b, double alpha){
    cairo_pattern_add_color_stop_rgba(cairo_get_source(cr), offset, r, g, b, alpha);
}

void loadImageSurface(cairo_t *cr, char* filename, double x, double y){
    cairo_surface_t *image=cairo_image_surface_create_from_png(filename);
    cairo_set_source_surface(cr, image, x, y);
    cairo_surface_destroy(image);
}

void roundedRectangle(cairo_t *cr, double x, double y, double w, double h, double r){
    cairo_move_to(cr,x+r,y);                      //# Move to A
    cairo_line_to(cr,x+w-r,y);                    //# Straight line to B
    cairo_curve_to(cr,x+w,y,x+w,y,x+w,y+r);       //# Curve to C, Control points are both at Q
    cairo_line_to(cr,x+w,y+h-r);                  //# Move to D
    cairo_curve_to(cr,x+w,y+h,x+w,y+h,x+w-r,y+h); //# Curve to E
    cairo_line_to(cr,x+r,y+h);                    //# Line to F
    cairo_curve_to(cr,x,y+h,x,y+h,x,y+h-r);       //# Curve to G
    cairo_line_to(cr,x,y+r);                      //# Line to H
    cairo_curve_to(cr,x,y,x,y,x+r,y);             //# Curve to A;
}

enum dTools{rectangle=1, curvedRect, circle, lineTo, lineRel, moveTo, moveRel, curveTo, curveRel, arcClockWise, arcCtrClockW, text,
            strokePreserve=14, fillPreserve, strokePath=16, fillPath, paintSurface, closePath,
            inkColor=20, inkColorAlpha, inkLinearGrad, inkRadialGrad, inkImage, inkDrawing, inkSetColorPt,
            lineWidth=40, lineStyle, fontFace, fontSize,
            drawToScrnN=50, drawToWindowN, drawToMemory, drawToPDF, drawToPS, drawToSVG, drawToPNG,
            shiftTo=60, scaleTo, rotateTo,
            loadImage=70, setBackgndImg, dispBackgnd, cachePic, dispPic,
            drawItem=100
};

typedef map<string, cairo_surface_t*> picCache_t;
picCache_t picCache;

void DrawProteusDescription(InfonPortal* portal, infon* ProteusDesc){
    cairo_surface_t* surface=portal->cairoSurf;
    if (surface==0 || ProteusDesc==0 || ProteusDesc->getSize()==0) ERRl("Description Flag Raised."<<ProteusDesc); // Missing description.
    //DEBl("DISPLAY:["<<printInfon(ProteusDesc));
    int count=0; int EOT_d2; char txtBuff[1024];
    infon *i, *ItemPtr, *OldItmPtr, *subItem;
    cairo_t *cr = portal->cr;
    SDL_Surface* utilSurface;
DEB("\n-----------------\n")
    int size, size2; double a,b,c,d,e,f; int Ia,Ib,Ic,Id,Ie,If,Ih,II; char  *Sa, *Sb;
    for(int EOL=theAgent.StartTerm(ProteusDesc, &i); !EOL; EOL=theAgent.getNextTerm(&i)){
        DEBl(count<<":[" << printInfon(i).c_str() << "]");
        int EOT_d2=theAgent.StartTerm(i, &ItemPtr);
        int cmd=gINT;
        DEB("\n[CMD:"<< cmd << "]");
        switch(cmd){ // TODO: Pango, Audio, etc.
            case rectangle:DEB("rectangle:") Z4 cairo_rectangle (cr, a,b,c,d);     break;
            case curvedRect:DEB("curvedRect:")  Z5 roundedRectangle(cr, a,b,c,d,e);break;
            case circle:  Z3 cairo_arc (cr, a,b,c,0.0,2.0*M_PI); break;
            case lineTo: DEB("lineTo:")  Z2   cairo_line_to(cr, a,b);              break;
            case lineRel: DEB("lineRel:") Z2   cairo_rel_line_to(cr, a,b);         break;
            case moveTo: DEB("moveTo:") Z2 cairo_move_to(cr, a,b);                 break;
            case moveRel: DEB("moveRel:") Z2   cairo_rel_move_to(cr, a,b);         break;
            case curveTo:  Z6   cairo_curve_to(cr, a,b,c,d,e,f);    break;
            case curveRel: Z6   cairo_rel_curve_to(cr, a,b,c,d,e,f);break;
            case arcClockWise:Z5 cairo_arc(cr, a,b,c,d,e);          break; //xc, yc, radius, angle1, angle2
            case arcCtrClockW:Z5 cairo_arc_negative(cr, a,b,c,d,e); break;
            case text: DEB("text:") S1  renderText(cr, Sa);         break;

            case strokePreserve: cairo_stroke_preserve(cr); break;
            case fillPreserve: cairo_fill_preserve(cr); break;
            case strokePath:   cairo_stroke(cr); DEB("strokePath")  break;
            case fillPath:     cairo_fill(cr); DEB("fillPath")      break;
            case paintSurface: cairo_paint(cr);      break;
            case closePath:    cairo_close_path(cr); break;

            case inkColor:DEB("inkColor:") Z3 cairo_set_source_rgba(cr, a, b, c, 1);  break;
            case inkLinearGrad: Z4 CreateLinearGradient(cr, a, b, c, d, (infon*)0); break;
            case inkRadialGrad: Z6 CreateRadialGradient(cr, a, b, c, d, e, f, (infon*)0); break;
            case inkSetColorPt: Z5 AddColorStop(cr, a, b, c, d, e); break;
            case inkImage: S1 Z2 loadImageSurface(cr, Sa, a, b); break;
            case inkDrawing:  break;
            case inkColorAlpha:DEB("inkColorAlpha:") Z4 cairo_set_source_rgba(cr, a, b, c, d);  break;

            case lineWidth:DEB("lineWidth:") Z1 cairo_set_line_width (cr, a); break;
            case lineStyle:  break;
            case fontFace: S1 fontDescription=Sa; break;
            case fontSize:  break;

            case drawToScrnN:  break;
            case drawToWindowN:  break;
            case drawToMemory:  break;
            case drawToPDF:  break;
            case drawToPS:  break;
            case drawToSVG:  break;
            case drawToPNG:  break;

            case shiftTo: Z2 cairo_translate(cr, a,b); break;
            case scaleTo: Z2 cairo_scale(cr, a,b); break;
            case rotateTo: Z2 cairo_rotate(cr, a); break;

            case loadImage: DEB("background:") S1 utilSurface=IMG_Load(Sa); SDL_BlitSurface(utilSurface,NULL, portal->surface,NULL); break;
            case setBackgndImg: S1
                if(portal->cairo_background!=0) break;
          //      portal->SDL_background=IMG_Load(Sa);
                portal->cairo_background = cairo_image_surface_create_from_png(Sa);
              //  portal->cairo_background = cairo_image_surface_create_for_data((unsigned char*)portal->SDL_background->pixels, CAIRO_FORMAT_RGB24,
               //             portal->SDL_background->w, portal->SDL_background->h,cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, portal->SDL_background->w));//portal->SDL_background->pitch);
                if(cairo_surface_status(portal->cairo_background) == CAIRO_STATUS_INVALID_STRIDE) cout << "ERROR Invalid Stride\n";
                if(cairo_surface_status(portal->cairo_background) != CAIRO_STATUS_SUCCESS) cout << "ERROR CREATING BACKGROUND SURFACE\n";
                break;
            case dispBackgnd:{
         //SDL_BlitSurface(portal->SDL_background,NULL, portal->surface, NULL);
                int w = cairo_image_surface_get_width (portal->cairo_background);
                int h = cairo_image_surface_get_height (portal->cairo_background);
                double pw = portal->surface->w;
                double ph = portal->surface->h;
                cairo_save(cr);
                cairo_scale(cr,pw/w, ph/h);
                cairo_set_source_surface(cr,portal->cairo_background,0,0);
                cairo_paint(cr);
                cairo_restore(cr);
                } break;
            case cachePic:{ S1
                picCache_t::iterator picPtr=picCache.find(Sa);
                if (picPtr==picCache.end()) {picCache[Sa]=cairo_image_surface_create_from_png(Sa);}
            } break;
            case dispPic:{ S1 Z3
           // cout << Sa <<a<<" "<<b<<" "<<c<<"\n";
                cairo_surface_t* pic=0;
                picCache_t::iterator picPtr=picCache.find(Sa);
                if (picPtr==picCache.end()) {picCache[Sa]=pic=cairo_image_surface_create_from_png(Sa);}
                else pic=picPtr->second;
                {//pic=cairo_image_surface_create_from_png(Sa);
                    double scale=c;
                    int w = cairo_image_surface_get_width (pic);
                    int h = cairo_image_surface_get_height (pic);
                    double pw = portal->surface->w;
                    double ph = portal->surface->h;
                    cairo_save(cr);
                    cairo_move_to(cr, a*scale,b*scale);
                    cairo_scale(cr,1/scale,1/scale);

                    cairo_set_source_surface(cr,pic,a,b);
                    cairo_paint(cr);
                    cairo_restore(cr);
                }
            }break;
            case drawItem: DEB(">");
                subItem=theAgent.gListNxt(&ItemPtr);
         //       cairo_new_sub_path(cr); //cairo_push_group(cr);
                OldItmPtr=ItemPtr; DrawProteusDescription(portal, subItem); ItemPtr=OldItmPtr;
           //      cairo_close_path(cr); //cairo_pop_group_to_source (cr);
                DEB("<"); break;

            default: {ERRl("Invalid Drawing Command:"<<cmd<<"\n"); exit(2);}
        }
    if (++count==2000) break;
    }
 //   cairo_destroy (cr);
    DEBl("\nCount: " << count << "\n======================");
}

/////////////////// End of Slip Drawing, Begin Interface to Proteus Engine
void CloseTurbulanceViewport(InfonViewPort* viewPort){
    if(viewPort->renderer) SDL_DestroyRenderer(viewPort->renderer);
    if(viewPort->window) SDL_DestroyWindow(viewPort->window);
    InfonPortal *portal=viewPort->parentPortal;
        // Remove this portal from parent
        InfonViewPort *currP, *prevP=0;
        for(currP=portal->viewPorts; currP != NULL; prevP=currP, currP=currP->next) {
            if (currP == viewPort) {
                if (prevP == NULL) portal->viewPorts = currP->next;
                else prevP->next = currP->next;
            }
        }
        delete(viewPort);
        if (portal->viewPorts==0){
            if(portal->cairoSurf) cairo_surface_destroy(portal->cairoSurf);
            if(portal->surface) SDL_FreeSurface(portal->surface);
            delete(portal->theme); delete(portal->user); delete(portal->crntFrame);

            // remove self from Portals[]
            int p;
            for(p=0; p<numPortals; ++p){if(portals[p]==portal) break;}
            while(++p<numPortals) portals[p-1]=portals[p];
            --numPortals;
            delete(portal);
        }
}

void ResizeTurbulancePortal(InfonPortal* portal, int w, int h){
    if(portal->cairoSurf) cairo_surface_destroy(portal->cairoSurf);
    if(portal->surface) SDL_FreeSurface(portal->surface);
    portal->needsToBeDrawn=true;
    portal->surface = SDL_CreateRGBSurface (0, w, h, 32,0x00ff0000,0x0000ff00,0x000000ff,0);
    portal->cairoSurf = cairo_image_surface_create_for_data((unsigned char*)portal->surface->pixels, CAIRO_FORMAT_RGB24, w, h,portal->surface->pitch);
    for(InfonViewPort *portView=portal->viewPorts; portView; portView=portView->next) // for each view
        SDL_RenderSetViewport(portView->renderer, NULL);
}

void RefactorPortal(InfonPortal *portal){
    int viewX, viewY, viewW, viewH, portalW, portalH;
    int windowsHere=0, currPos=0, prevPos=0, gap=0;
    map <int, InfonViewPort*> sides;
    map <int, InfonViewPort*>::iterator it;
    InfonViewPort *currView=0; InfonViewPort *portView=0;
    // Find X positions for all this portal's views
    for(portView=portal->viewPorts; portView; portView=portView->next){ // for each view
        SDL_GetWindowPosition(portView->window, &viewX, &viewY);
        SDL_GetWindowSize(portView->window, &viewW, &viewH);
        sides[viewX]=portView;
        sides[viewX+viewW]=portView;
    }
    for (it=sides.begin() ; it != sides.end(); it++ ){
        currPos=(*it).first;
        currView=(*it).second;
        if(windowsHere==0) gap+=(currPos-prevPos);
        SDL_GetWindowPosition(currView->window, &viewX, &viewY);
        if(currPos==viewX) {windowsHere++; currView->posX=(currPos-gap);} else windowsHere--;
        prevPos=currPos;
    }
    portalW=(currPos-gap);

    // Find Y positions for all this portal's views
    sides.clear(); windowsHere=0; currPos=0; prevPos=0; gap=0;
    for(portView=portal->viewPorts; portView; portView=portView->next){ // for each view
        SDL_GetWindowPosition(portView->window, &viewX, &viewY);
        SDL_GetWindowSize(portView->window, &viewW, &viewH);
        sides[viewY]=portView;
        sides[viewY+viewH]=portView;
    }
    for (it=sides.begin() ; it != sides.end(); it++ ){
        currPos=(*it).first;
        currView=(*it).second;
        if(windowsHere==0) gap+=(currPos-prevPos);
        SDL_GetWindowPosition(currView->window, &viewX, &viewY);
        if(currPos==viewY) {windowsHere++; currView->posY=(currPos-gap);} else windowsHere--;
        prevPos=currPos;
    }
    portalH=(currPos-gap);

    ResizeTurbulancePortal(portal, portalW, portalH);
}

void AddViewToPortal(InfonPortal* portal, char* title, int x, int y, int w, int h){
    InfonViewPort* VP=new InfonViewPort;
    VP->window=SDL_CreateWindow(title, x,y,w,h, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowData(VP->window, "portalView", VP);
    //TODO: Hardcode: icon, bpp-depth=SDL_PIXELFORMAT_RGB24 / vid_mode
//    SDL_SetWindowDisplayMode(VP->window, NULL); // Mode for use during fullscreen mode
//    LoadIcon()
    VP->renderer=SDL_CreateRenderer(VP->window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(VP->renderer, 0xA0, 0xA0, 0xc0, 0xFF);
    VP->isMinimized=false;
    VP->posX=0; VP->posY=0; // location in parent window.
    VP->scale=1.0;
    VP->parentPortal=portal;
    VP->next=portal->viewPorts;
    portal->viewPorts=VP;
}

bool CreateTurbulancePortal(char* title, int x, int y, int w, int h, User* user, infon* theme, infon* stuff){
    if(numPortals >= MAX_PORTALS) return 1;
    char winTitle[1024] = windowTitle; // POLISH: Add Viewport ID to title
    if (numPortals>0){SDL_snprintf(winTitle, strlen(title), "%s %d", title, numPortals+1);}
    else {strcpy(winTitle, title);}

    InfonPortal* portal=new InfonPortal;
    portal->isLocked=false;
    AddViewToPortal(portal, title, x,y,w,h);
    ResizeTurbulancePortal(portal, w, h);

    portal->stuff=stuff;
    portal->theme=theme;

    portals[numPortals++]=portal;
    return 0;
}

void DestroyTurbulancePortal(InfonPortal *portal){
    //TODO slide portals items down
    cairo_surface_destroy(portal->cairoSurf);
    SDL_FreeSurface(portal->surface);
    for(InfonViewPort *portView=portal->viewPorts; portView; portView=portView->next) // for each view
        SDL_DestroyRenderer(portView->renderer);
}

//////////////////// End of Slip Specific Code, Begin Rendering Code

int secondsToRun=1200; // time until the program exits automatically. 0 = don't exit.
void EXIT(char* errmsg){ERRl(errmsg << "\n"); exit(1);}
void cleanup(void){SDL_Quit();}   //TODO: Add items to cleanup routine

enum userActions {TURB_UPDATE_SURFACE=1, TURB_ADD_SCREEN, TURB_DEL_SCREEN, TURB_ADD_WINDOW, TURB_DEL_WINDOW};

infon* NormalizeAndPresent(infon* i, infon* firstID=0){
        if (i==0) return 0;
        QitemPtr qp(new Qitem(i,firstID,(firstID)?1:0,0));
        infQ ItmQ; ItmQ.push(qp);
        while (!ItmQ.empty()){
            QitemPtr cn=ItmQ.front(); ItmQ.pop(); infon* CI=cn->item;
            theAgent.fetch_NodesNormalForm(cn);

            if(CI->type){
                if(CI->type->norm == "portal"){MSGl("PORTAL");}
                else if(CI->type->norm == "frame"){MSGl("FRAME");}
                else {MSGl(CI->type->norm <<" ["<<CI<<"]"); }

            }else {MSG("*");}
            //wait?
            theAgent.pushNextInfon(CI, cn, ItmQ);
        }
    return 0;
}

static int SimThread(void *nothing){
    SDL_Event ev; InfonPortal* portal; infon* nextFrame;
    ev.type = SDL_USEREVENT;  ev.user.code = TURB_UPDATE_SURFACE;  ev.user.data1 = 0;  ev.user.data2 = 0;
    while (!doneYet) {
        for (int i = 0; i < numPortals; ++i) {  // Collect and dispatch frames for portals.
            portal=portals[i];
            nextFrame=new infon;
            theAgent.deepCopy(portal->stuff, nextFrame);
            theAgent.normalize(nextFrame);
            ev.user.data1 = (void *)i;
            ev.user.data2 = nextFrame;
            SDL_PushEvent(&ev);
            ++numEvents;
        }
        SDL_Delay(100);
    }
  return 0;
}

void InitializePortalSystem(int argc, char** argv){
    numPortals=0;
    char* worldFile="world.pr"; char* username="bruce"; char* password="erty"; string theme; char* portalContent="";
    for (int i=1; i<argc;) {
        int consumed = 0;
        if (consumed == 0) {
            consumed = -1;
            if (SDL_strcasecmp(argv[i], "--world") == 0) {if (argv[i + 1]) {worldFile=argv[i+1]; consumed = 2;}}
            else if (SDL_strcasecmp(argv[i], "--theme") == 0) {if (argv[i + 1]) {theme=argv[i+1]; consumed = 2;}}
            else if (SDL_strcasecmp(argv[i], "--user") == 0) {if (argv[i + 1]) {username=argv[i+1]; consumed = 2;}}
            else if (SDL_strcasecmp(argv[i], "--pass") == 0) {if (argv[i + 1]) {password=argv[i+1]; consumed = 2;}}
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
    //MAYBE: if (DebugMode) PrintConfiguration from common.c Set debugmode via command-line argument

 //UNDO:   SDL_EnableKeyRepeat(300, 130);
  //UNDO:  SDL_EnableUNICODE(1);
    atexit(cleanup);

    populateLangExtentions();
    if(theAgent.loadInfon(worldFile, &theAgent.world)) exit(1);
    User* portalUser=new User;
    if(loadUserRecord(portalUser, username, password)) {MSGl("\nUser could not be authenticated. Exiting..."); exit(5);}
    if (theme=="") theme=portalUser->defaultTheme;
    infon *themeInfon, *stuffInfon;
    if(theAgent.loadInfon(theme.c_str(), &themeInfon, 0)) exit(1);
    theAgent.utilField=themeInfon;
    if(theAgent.loadInfon(portalUser->myStuff.c_str(), &stuffInfon, 0)) exit(1);
/*
infon* PORTAL;
if(theAgent.loadInfon("portals.pr", &PORTAL, 0)) exit(1);
debugInfon=PORTAL;
NormalizeAndPresent(PORTAL);
MSGl("\nOK\n"<<printInfon(PORTAL));
exit(2);
*/


    //CreateTurbulancePortal(windowTitle, 100,1300,1024,768, portalUser, themeInfon, stuffInfon);
    CreateTurbulancePortal(windowTitle, 150,100,800,600, portalUser, themeInfon, stuffInfon);
}

#define IS_CTRL (ev.key.keysym.mod&KMOD_CTRL)

void StreamEvents(){
    Uint32 frames=0, then=SDL_GetTicks(), now=0, limit = secondsToRun*1000;
    SDL_Thread *simulationThread = SDL_CreateThread(SimThread, "Proteus", NULL);
    SDL_Event ev;
    InfonViewPort *portalView=0; SDL_Window *window=0;
    while (secondsToRun && (SDL_GetTicks() < (then+limit)) && !doneYet){
         ++frames;
        while (SDL_PollEvent(&ev)) {
            window = SDL_GetWindowFromID(ev.key.windowID);
            if(window) portalView=(InfonViewPort*)SDL_GetWindowData(window, "portalView");
            switch (ev.type) {
            case SDL_USEREVENT:
                --numEvents;
                switch(ev.user.code){
                    case TURB_UPDATE_SURFACE:
                      portals[(uint)ev.user.data1]->crntFrame=(infon*)ev.user.data2;
                      portals[(uint)ev.user.data1]->needsToBeDrawn=true;
                      break;
                    case TURB_ADD_SCREEN: break;
                    case TURB_DEL_SCREEN: break;
                    case TURB_ADD_WINDOW: break;
                    case TURB_DEL_WINDOW: break;
                }
                break;
            case SDL_WINDOWEVENT:
                switch (ev.window.event) {
                case SDL_WINDOWEVENT_MOVED: portalView->parentPortal->viewsNeedRefactoring=true; break;
                case SDL_WINDOWEVENT_RESIZED: portalView->parentPortal->viewsNeedRefactoring=true; break;//ResizeTurbulancePortal(portalView->parentPortal, ev.window.data1, ev.window.data2); break;
                case SDL_WINDOWEVENT_MINIMIZED: portalView->isMinimized=true;  break;  // Stop rendering
                case SDL_WINDOWEVENT_MAXIMIZED: break;
                case SDL_WINDOWEVENT_RESTORED: portalView->isMinimized=false;  break;  // Resume rendering
                case SDL_WINDOWEVENT_FOCUS_GAINED:break; // Move window to front position
                case SDL_WINDOWEVENT_FOCUS_LOST:break;
                case SDL_WINDOWEVENT_CLOSE:  CloseTurbulanceViewport(portalView); break;
                case SDL_WINDOWEVENT_ENTER:break; // Mouse enters window
                case SDL_WINDOWEVENT_LEAVE:break; // Mouse leaves window
                }
                break;
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_PRINTSCREEN: break; // see sdl_common.c
                case SDLK_c: if (IS_CTRL) {SDL_SetClipboardText("SDL rocks!\nYou know it!");} break;
                case SDLK_v: if (IS_CTRL) {} break;       // Paste. see sdl_common.c
                case SDLK_g: if (IS_CTRL) {} break;       // Grab Input. see sdl_common.c
                case SDLK_p: if (IS_CTRL) {} break;       // Create PDF view and render to file
                case SDLK_k: if (IS_CTRL) {} break;       // Toggle on-screen Keyboard
                case SDLK_t: if (IS_CTRL) {} break;       // Cycle Themes
                case SDLK_l: if (IS_CTRL) {               // Toggle portal locking
                    portalView->parentPortal->isLocked=!portalView->parentPortal->isLocked;
                    portalView->parentPortal->viewsNeedRefactoring=true;
                    }
                    break;
                case SDLK_s: if (IS_CTRL) {} break;       // Shift window to a new screen or computer.
                case SDLK_e: if (IS_CTRL) {               // Extend this portal with another new view
                    {AddViewToPortal(portalView->parentPortal, "New View!", 100,1300,1024,768);}
                }
                break;
                case SDLK_f: // Toggle fullscreen.
                    if (IS_CTRL && window) {
                            if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
                                SDL_SetWindowFullscreen(window, SDL_FALSE);
                            } else {SDL_SetWindowFullscreen(window, SDL_TRUE);}
                    }
                    break;
                case SDLK_d: if (IS_CTRL) {
                    //CreateTurbulancePortal(windowTitle, 100,1300,1024,768, portalView->parentPortal->user,
                    CreateTurbulancePortal(windowTitle, 150,90,800,600, portalView->parentPortal->user,
                        portalView->parentPortal->theme, portalView->parentPortal->crntFrame);} break;       // New Portal
                case SDLK_RETURN: doneYet=true; break;
                case SDL_SCANCODE_ESCAPE:
                case SDLK_ESCAPE: doneYet=true; break;
                }
                break;
            case SDL_QUIT: doneYet=true; break;

            }
        }
        if(doneYet) continue;
        for (int i = 0; i < numPortals; ++i) {
            InfonPortal* portal=portals[i];
            if(portal->needsToBeDrawn) portal->needsToBeDrawn=false; else continue;
            if(portal->viewsNeedRefactoring && !portal->isLocked) RefactorPortal(portal);

            portal->cr = cairo_create(portal->cairoSurf); cairo_set_antialias(portal->cr,CAIRO_ANTIALIAS_GRAY);
            DrawProteusDescription(portal, portal->crntFrame);
            cairo_destroy(portal->cr);
            int viewW, viewH;
            for(InfonViewPort *portView=portal->viewPorts; portView; portView=portView->next){ // For each view...
                if(portView->isMinimized) continue;
                SDL_Renderer *renderer = portView->renderer;
                SDL_GetWindowSize(portView->window, &viewW, &viewH);
                SDL_Texture *tex = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB888,SDL_TEXTUREACCESS_STREAMING,viewW,viewH);
                SDL_LockSurface( portal->surface );
                Uint8 *p = (Uint8 *)portal->surface->pixels + portView->posY * portal->surface->pitch + portView->posX * 4;
                SDL_UpdateTexture(tex, NULL, p, portal->surface->pitch);
                SDL_UnlockSurface( portal->surface );

                SDL_RenderCopy(renderer, tex, NULL, NULL);
                SDL_RenderPresent(renderer);
                SDL_DestroyTexture(tex);
            }
        }
        SDL_Delay(10);
    }
    doneYet=true;
    if ((now = SDL_GetTicks()) > then) printf("\nFrames per second: %2.2f\n", (((double) frames * 1000) / (now - then)));

    SDL_WaitThread(simulationThread, NULL);
}

int main(int argc, char *argv[]){
    MSGl("\n\n         * * * * * Starting Proteus and The Slipstream * * * * *\n");
    InitializePortalSystem(argc, argv);
    StreamEvents();
    return (0);
}
