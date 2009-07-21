// The SDL SlypStreem Engine. Copyright 2008 by Bruce Long
/*    This file is part of the "SlypStreem Engine"

    The SlypStreem Engine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The SlypStrem Engine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the SlypStreem Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#define OUT(msg) {std::cout<< msg;}
#define DEB(msg) {std::cout<< msg << "\n";}

#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "fastevents.h"
#include "SDLUtils.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <cairo.h>



enum {LINUX, MACINTOSH, WINDOWS}

const QPLATFORM=LINUX;

int height, width, bpp;
static int doneYet = 0;
static int num = 0;

//////////////////// Slyp Code
#include <strstream>
#include <iostream>
#include <fstream>
#include "../core/Proteus.h"

#define gINT gInt()
#define gZto1 (float)(((float)gInt()) / 1024.0)
#define gSTR gStr()

infon* ProteusDesc=0;

#define Z2 {a=gZto1; b=gZto1;}
#define Z3 {a=gZto1; b=gZto1; c=gZto1;}
#define Z4 {a=gZto1; b=gZto1; c=gZto1; d=gZto1;}
#define Z5 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; e=gZto1;}
#define Z6 {a=gZto1; b=gZto1; c=gZto1; d=gZto1; f=gZto1; e=gZto1;}
#define I2 {Ia=gINT; Ib=gINT;}
#define I3 {Ia=gINT; Ib=gINT; Ic=gINT;}

infon* ItmPtr=0;
int EOT_d2,j,sign;
char textBuff[1024];
const int numOfSlots=5;  // GL slots for textures
GLuint texSlots[numOfSlots];

int gInt(){
    getInt(ItmPtr,j,sign);  // std::cout << j << ", ";
    getNextTerm(ItmPtr,d2);
    return (sign)?-j:j;
}

char* gStr() {
	if((ItmPtr->flags&tType)==tString && !(ItmPtr->flags&fUnknown)) {
		memcpy(textBuff, ItmPtr->value, (uint)ItmPtr->size);
		textBuff[(uint)(ItmPtr->size)]=0;
	}
	getNextTerm(ItmPtr,d2);
	return textBuff;
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

void DrawTriStrip(int size, int* array){
    glDisable (GL_TEXTURE_2D);glEnable(GL_BLEND);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_INT, 0, array);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, size);
}

void DrawTriFan(int size, int* array){
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glDisable (GL_TEXTURE_2D);
glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_INT, 0, array);
    glDrawArrays(GL_TRIANGLE_FAN, 0, size);
}

void DrawTriStripTex(int size, int size2){
    glEnable (GL_TEXTURE_2D); 
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_INT, 0, vertexes);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, size);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void DrawTriFanTex(int size, int size2){
    glEnable (GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_INT, 0, vertexes);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glDrawArrays(GL_TRIANGLE_FAN, 0, size);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void setGradientSource(cairo_t* cr){
    float a,b,c,d,e,f;
	int linear=gINT; // 0=radial, 1=linear
	cairo_pattern_t* pattern=0;
	if (linear) {Z4 pattern=cairo_pattern_create_linear(a,b,c,d);}
	else {Z6 pattern=cairo_pattern_create_radial(a,b,c,d,e,f);}
	int numColorStops=gINT;
	for (int i=0; i<numColorStops; ++i){
		int useAlpha=gINT; 
		if (useAlpha) {Z5 cairo_pattern_add_color_stop_rgba(pattern, a,b,c,d,e);}
		else {Z4 cairo_pattern_add_color_stop_rgb(pattern, a,b,c,d);}
	}
	cairo_set_source(cr, pattern);
}

void TextureViaCairoPango(){
	cairo_surface_t* surface;	float a,b,c,d; int Ia,Ib,Ic;
infon* ItmPtrC=0;
if (ItmPtr==0) std::cout << "Err1 in cairo Draw\n";
if (ItmPtr->size==0) std::cout << "Err2 in cairo Draw\n";

int count=0;
int width=gINT;
int height=gINT;
const int channels=4;
unsigned char* ImgBuff=(unsigned char*)calloc (channels * width * height, sizeof (char));
surface=cairo_image_surface_create_for_data (ImgBuff,CAIRO_FORMAT_ARGB32, width, height, channels*width);
cairo_t* cr=cairo_create(surface);
cairo_status_t status;
//cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0); cairo_paint(cr);
int EOT_old=EOT_d2; ItmPtrC=ItmPtr;
while(!EOT_old){
//  std::cout << count<<":[" << printInfon(i).c_str() << "]\n";
        StartTerm(ItmPtrC,ItmPtr,d2);
//std::cout<<"ival["<<ItmPtr<<"]\n";
        int cmd=gINT;
std::cout<<"\n[Cairo CMD:"<< cmd << "]";
        switch(cmd){
            case 0:   Z3 cairo_set_source_rgb (cr,a,b,c);     break;
            case 1:   Z4 cairo_set_source_rgba(cr,a,b,c,d);  break;
            case 2:	   setGradientSource(cr);  			    break;
            
            case 10:   cairo_paint(cr);						             break;
            case 11:   cairo_stroke(cr);								     break;
            case 12:   cairo_fill(cr);								     break;
            case 13:   cairo_set_line_width(cr, gZto1);				     break;
            case 14:   Z2  cairo_move_to(cr,a, b);       				 break;
            case 15:   Z2  cairo_line_to(cr,a, b); 					 break;
 	        case 16:   Z2  cairo_rel_move_to(cr,a, b); 					 break;
            case 17:   Z2  cairo_rel_line_to(cr,a, b);	 break;
 			case 18:   Z4  cairo_rectangle(cr, a,b,c,d);
            case 19:   I3 Z2 cairo_arc(cr, Ia, Ib, Ic, a, b);	 break;
            
            case 30:	gSTR; I2 cairo_select_font_face(cr,textBuff, (cairo_font_slant_t)Ia, (cairo_font_weight_t)Ib); break;
            case 31:	cairo_set_font_size(cr, gZto1);				 break;
            case 32:	cairo_show_text(cr, gSTR);					 break;
        }
    if (++count==90) break;
    getNextTerm(ItmPtrC,old);
    }
//cairo_surface_write_to_png(surface, "image.png"); 
    EOT_d2=EOT_old; ItmPtr=ItmPtrC;
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGBA,width,height,0,GL_BGRA,GL_UNSIGNED_BYTE,ImgBuff);
	free(ImgBuff);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
//    std::cout << "\nCount: " << count << "\n======================\n";
}

void DrawProteusDescription(infon* ProteusDesc){
if (ProteusDesc==0) std::cout << "Err1\n";
if (ProteusDesc->size==0) std::cout << "Err2\n";

int count=0;
int size, size2; float a,b,c,d; int Ia,Ib,Ic;
int EOT_d1=0; infon* i=0;
StartTerm(ProteusDesc, i, d1)
while(!EOT_d1){
//  std::cout << count<<":[" << printInfon(i).c_str() << "]\n";
        StartTerm(i,ItmPtr,d2);
//std::cout<<"ival["<<ItmPtr<<"]\n";
        int cmd=gINT;
std::cout<<"\n[CMD:"<< cmd << "]";
        switch(cmd){
            case 0:    Z3          glColor3f(a,b,c);        break;
            case 1:    Z4         glColor4f(a,b,c,d);    break;
            case 2:             glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texSlots[gINT]);           break;
            case 3:    Z3         glTranslatef(a,b,c);     break;
            case 4:    Z4         glRotatef(a,b,c,d);  break;
            case 5:     I3        glNormal3i(Ia,Ib,Ic);       break;
            case 6:     size=getArrayi(vertexes); DrawTriStrip(size, vertexes); break;
            case 7:     size=getArrayi(vertexes); DrawTriFan  (size, vertexes); break;
            case 8:	size=getArrayi(vertexes);  size2=getArrayz(texCoords);  DrawTriStripTex(size, size2); break;
            case 9:	size=getArrayi(vertexes);  size2=getArrayz(texCoords); DrawTriFanTex  (size, size2); break;

            case 10:       glLoadIdentity(); break;
            case 11:       glPushMatrix();   break;
            case 12:       glPopMatrix();   break;
            
            case 90:	TextureViaCairoPango(); break;
        }
    if (++count==90) break;
    getNextTerm(i,d1);
    }
    std::cout << "\nCount: " << count << "\n======================\n";
}
//////////////////// End of Slyp Drawing, Begin Interface to Proteus Engine
int initWorldAndEventQueues(){
    // Load World
    std::cout << "Loading world.pr\n";
    std::fstream fin("world.pr");
    QParser q(fin);
extern infon *World; World=q.parse();
    if (World) std::cout<<"["<<printInfon(World)<<"]\n";
    else {std::cout<<"Error:"<<q.buf<<"\n"; exit(1);}
    agent a;
    a.normalize(World);

    // Load DispList
    std::cout << "Loading display.pr\n";
    std::fstream fin2("display.pr");
    QParser D(fin2);
    infon* displayList=D.parse(); std::cout << "parsed\n";
    if(displayList) std::cout<<"["<<printInfon(displayList).c_str()<<"]\n";
    else {std::cout<<"Error:"<<D.buf<<"\n"; exit(1);}

    a.normalize(displayList);
    DEB("Normed.");
//    DEB(printInfon(displayList))
    ProteusDesc=displayList;
}

//////////////////// End of Slyp Specific Code, Begin OpenGL Code
#define FRAME_RATE_SAMPLES 50
int FrameCount=0;
float FrameRate=0;

float Light_Ambient[]={0.1f, 0.1f, 0.1f, 1.0f};
float Light_Diffuse[]={1.2f,1.2f,1.2f,1.0f};
float Light_Position[]={0.0f, 0.0f, -5000.0f, 1.0f};
float X_Rot=0.2; float X_Speed=5.0; float Y_Rot=0; float Y_Speed=6.0;

static void ourDoFPS(){
   static clock_t last=0;
   clock_t now;
   float delta;

   if (++FrameCount >= FRAME_RATE_SAMPLES) {
      now  = clock();
      delta= (now - last) / (float) CLOCKS_PER_SEC;
      last = now;

      FrameRate = FRAME_RATE_SAMPLES / delta;
DEB(FrameRate)
      FrameCount = 0;
   }
}

void DrawScreen(){
    char buf[120]; // for the strings to print
    // Need to manipulate the ModelView matrix to move our model around.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); // Reset to 0,0,0; no rotation, no scaling.
    glTranslatef(0.0,0.0,-20000.0);// Move the object back from the screen.
    glRotatef(X_Rot,1.0f,0.0f,0.0f); glRotatef(Y_Rot,0.0f,1.0f,0.0f);// Rotate the calculated amount.

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// Clear the color and depth buffers.

    DrawProteusDescription(ProteusDesc);

{ // Draw any test over the top (change this to use textures?)
/*   glLoadIdentity();// Move back to the origin (for the text, below).
   glMatrixMode(GL_PROJECTION);// We need to change the projection matrix for the text rendering.
   glPushMatrix();// But we like our current view too; so we save it here.
   glLoadIdentity();glOrtho(0,width,0,height,-1.0,1.0);// Set up a new projection for text.
   glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);// Lit or textured text looks awful.
   glDisable(GL_DEPTH_TEST);  // We don't want depth-testing either.
   glColor4f(0.6,1.0,0.6,.75); // But, for fun, let's make the text partially transparent too.

   // Render our various display mode settings.
   sprintf(buf,"Mode: %s", TexModesStr[Curr_TexMode]);
   glRasterPos2i(2,2); //ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);

   sprintf(buf,"AAdd: %d", Alpha_Add);
   glRasterPos2i(2,14); //ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);

   sprintf(buf,"Blend: %d", Blend_On);
   glRasterPos2i(2,26); //ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);

   sprintf(buf,"Light: %d", Light_On);
   glRasterPos2i(2,38); //ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);

   sprintf(buf,"Tex: %d", Texture_On);
   glRasterPos2i(2,50); //ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);

   sprintf(buf,"Filt: %d", Filtering_On);
   glRasterPos2i(2,62); //ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);

   // Now we want to render the calulated FPS at the top.

   // To ease, simply translate up.  Note we're working in screen
   // pixels in this projection.

   glTranslatef(6.0f,Window_Height - 14,0.0f);

   // Make sure we can read the FPS section by first placing a
   // dark, mostly opaque backdrop rectangle.
   glColor4f(0.2,0.2,0.2,0.75);

   glBegin(GL_QUADS);
   glVertex3f(  0.0f, -2.0f, 0.0f);
   glVertex3f(  0.0f, 12.0f, 0.0f);
   glVertex3f(140.0f, 12.0f, 0.0f);
   glVertex3f(140.0f, -2.0f, 0.0f);
   glEnd();

   glColor4f(0.9,0.2,0.2,.75);
   sprintf(buf,"FPS: %f F: %2d", FrameRate, FrameCount);
   glRasterPos2i(6,0);
   //ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);

   // Done with this special projection matrix.  Throw it away.
   glPopMatrix();  */
}

    SDL_GL_SwapBuffers( );// All done drawing.  Let's show it.

 //   X_Rot+=X_Speed; Y_Rot+=Y_Speed; // Now let's do the motion calculations.
std::cout<<"################\n";
    ourDoFPS();// And collect our statistics.
    }

void ResizeScene(int Width,int Height){
   if (Height == 0) Height = 1;
    GLfloat aspect=(GLfloat)Width/(GLfloat)Height;

   glViewport(0, 0, Width, Height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(45.0f, aspect, 5000.0, 150000.0) ;//5000.0f,+15000.0f);

   glMatrixMode(GL_MODELVIEW);

   width  = Width;
   height = Height;
}

// Does everything needed for OpenGL before losing control to the SDL event loop.
void setupOpenGL(){
 //   ourBuildTextures();

    /* Culling. */
//    glCullFace( GL_BACK );
//    glFrontFace( GL_CCW );
//    glEnable( GL_CULL_FACE );

    glClearColor(0.0f, 0.8f, 0.0f, 0.0f);// Color to clear color buffer to.

    glClearDepth(150000.0);        // Depth to clear depth buffer to;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);     // type of test.
    glShadeModel(GL_SMOOTH);  // Enables Smooth Color Shading.

    // Load up the correct perspective matrix; using a callback directly.
    ResizeScene(width,height);

    // Set up a light, turn it on.
    glLightfv(GL_LIGHT1, GL_POSITION, Light_Position);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  Light_Ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  Light_Diffuse);
    glEnable (GL_LIGHT1);

    // A handy trick -- have surface material mirror the color.
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    
    //////////
    // Set up texture system
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glGenTextures(numOfSlots, texSlots);
	GLuint texSlot1=texSlots[0];

//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
for (int i=0; i<=numOfSlots; ++i){
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texSlots[i]);
	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_DECAL);
}
glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texSlot1);
/*	// Autogenerate texture map
	GLfloat zPlane[]={0.0f, 0.0f, 1.0f, 0.0f};
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
//	glTexGenfv(GL_S, GL_OBJECT_PLANE, zPlane);
//	glTexGenfv(GL_T, GL_OBJECT_PLANE, zPlane); */
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

    // Set the OpenGL attributes
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, bpp );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); // 1 = double buffering in GL, 0 = none
    DEB("H: " << height << "    W:"<<width << "  bpp:" << bpp);
    // Create a (hopefully) matching screen buffer
    if (SDL_SetVideoMode( width, height, bpp, SDL_OPENGL|SDL_FULLSCREEN) == 0 ) {
        DEB( SDL_GetError());
        EXIT((char*)"Couldn't set GL video mode");
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
    setupOpenGL();
    eventLoop(argc, argv);
    return (0);
}
