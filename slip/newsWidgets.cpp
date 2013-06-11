/// Copyright 2013 Bruce Long. All rights reserved.
// This file is intended to be included in slip.cpp.


enum filterModes {
    fmReadOnly=1, fmUnreadOnly=2, fmReadUnread=3,
    fmUserKnownOnly=4, fmUserDoesntKnowOnly=8, fmKnownBoth=12,
    fmVerifiedNewsOnly=16, fmReliableSourceOnly=32,
    fmComedy=64, fmGossip=128, fmNoViolence=256
    // friendsKnow, dontCare, etc.
};
enum timeUnits {uSecond, uMinute, uHour, uWorkDay, uDay, uWeek, uWorkWeek, uMonth, uYear, uDecade, uCentury};
enum viewModes {vmHorizontal=0, vmVertical=1, vmTimeLine=2, vmCalendar=4};

struct factoid {
    int64_t index;
    string headline;
    int height;
    int importance;
    bool hasRead;
    bool havent_read_dontWantTo;
    int userKnows;
    // friends who know...
    // list<links>; // links to articles, videos, people, etc.
    // list<sources>;

    factoid(int64_t idx, string headLine, int import){index=idx; headline=headLine; importance=import;}
};

//////////////////////////////////////////////////////////////
//     D R A W I N G   M A N A G E M E N T   C L A S S E S
//////////////////////////////////////////////////////////////

struct DisplayItem {
    DisplayItem(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):dirty(0),visible(1),x(X),y(Y),w(W),h(H),parent(Parent){};
    bool dirty,visible;             // If either is false, draw() should usually just return 0.
    int x,y,w,h,scale;              // Position, width, height, scale.
    DisplayItem* parent;           // Use to tell parent you are dirty -- need to be redrawn.
    cairo_t *cr;                    // Cairo drawing context;
    virtual int draw(cairo_t *cr){return 0;}; // Override to draw your item.
    virtual int handleEvent(SDL_Event &ev){return 0;};     // Override to handle events
    void markDirty() {for(DisplayItem* d=this; d; d=d->parent) d->dirty=true;}
    virtual ~DisplayItem(){};

};

DisplayItem* WidgetWithCapturedMouse;
void captureMouseMsgs(DisplayItem* widget){WidgetWithCapturedMouse=widget;}
void releaseMouseMsgs(){WidgetWithCapturedMouse=0;}

enum scrollStates {still, fingerPressed, pullScrolling, freeScrolling, stopping};
uint32_t tick(uint32_t interval, void *param); // forward decl

struct ScrollingDispItem:DisplayItem {
    ScrollingDispItem(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){
        offset=0; span=0; timestamp=0; scrollState=still;
        downX=downY=prevX=prevY=veloX=veloY=0;
    };
    ~ScrollingDispItem(){};
    /////// Scrolling functionality
    const double moveThreshold = 1;
    const int interval=20; // Tick interval
    double offset, span;
    double downX, downY, prevX, prevY, veloX, veloY, scale;
    UInt timestamp;
    scrollStates scrollState;
    SDL_TimerID timer_id;

    void setOffset(double off){
        if(off != offset){
            if(off< -10000) off=-10000;
            if(off> 10000) off=10000;
            offset=off;
            markDirty();
            SDL_Event ev; ev.type = SDL_USEREVENT;  ev.user.code = 0;  ev.user.data1 = 0;  ev.user.data2 = 0;
            SDL_PushEvent(&ev);
        }
    }

    int handleEvent(SDL_Event &ev){
        double crntX=0, crntY=0, delta_x, delta_y;
        switch (ev.type) {
            case SDL_MOUSEBUTTONDOWN:
                if(ev.button.button!=SDL_BUTTON_LEFT) return 0;
                crntX=ev.button.x-x; crntY=ev.button.y-y;
                if(crntX<=0 || crntX>(w) || crntY<0 || crntY>(h)) return 0;
                captureMouseMsgs(this);
                if(scrollState==still){                  // Finger Pressed while still
                    scrollState=fingerPressed;
                    prevX=downX=crntX; prevY=downY=crntY;
                } else if(scrollState==freeScrolling){  // Finger Pressed while free-scrolling
                    scrollState=stopping;
                    veloX=0; veloY=0;
                    downX=crntX; downY=crntY;
                    SDL_RemoveTimer(timer_id);  // NOTE: It is not safe to remove a timer multiple times.

                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(ev.button.button!=SDL_BUTTON_LEFT) return 0;
                releaseMouseMsgs();
                crntX=ev.button.x-x; crntY=ev.button.y-y;
                if(scrollState==fingerPressed){          // Finger released while pressed
                    scrollState=still;
 //                   skewX=0; skewY=0;
//                     for visible items, handleEvent(ev)

                } else if(scrollState==pullScrolling){   // Finger released while pull-scrolling
                    delta_x=crntX-prevX; delta_y=crntY-prevY;
                    prevX=crntX; prevY=crntY;
                    uint ticks=SDL_GetTicks()-timestamp; timestamp = SDL_GetTicks();
                    veloX = delta_x*interval/ticks; veloY = delta_y*interval/ticks;

                    if(veloX==0 && veloY==0) {scrollState=still;}
                    else{
                        veloX = (downX-crntX)/10;
			veloY = (downY-crntY)/10;
                        scrollState=freeScrolling;cout<<'!'<<flush;
                        timer_id = SDL_AddTimer(interval, tick, this);

                    }
                } else if(scrollState==stopping){        // Finger released while stopping/slowing.
                    scrollState=still;
           //         skewX=0; skewY=0;
                }
                break;
            case SDL_MOUSEMOTION:
                if(!(ev.motion.state & SDL_BUTTON_LMASK)) return 0;
                crntX=ev.motion.x-x; crntY=ev.motion.y-y;
                delta_x=crntX-prevX; delta_y=crntY-prevY;
                prevX=crntX; prevY=crntY;
                if(scrollState==fingerPressed || scrollState==stopping){ // Finger moves while just pressed or stopping
                    if(delta_x > moveThreshold || delta_y > moveThreshold || delta_x < -moveThreshold || delta_y < -moveThreshold){ cout<<'=';
						timestamp = SDL_GetTicks();
						scrollState=pullScrolling;
						delta_x=delta_y=0;
                    } else { cout<<'-'<<flush;// Movement, but not much.
					}
                } else if(scrollState==pullScrolling){ cout<<'~'<<flush;    // Finger moves, pull-scrolling the view.
                    setOffset(offset - (delta_x*scale));
                    uint ticks=SDL_GetTicks()-timestamp; timestamp = SDL_GetTicks();
                    veloX = delta_x*interval/ticks; veloY = delta_y*interval/ticks;
                    if(veloX==0 && veloY==0) {scrollState=still;}
                }
                break;
            case SDL_MOUSEWHEEL:
            break;
        }
        return 0;
    }
};

uint32_t tick(uint32_t interval, void *param){ cout<<':'<<flush;
    ScrollingDispItem *item=(ScrollingDispItem*)param;
    if (item->scrollState == freeScrolling) {
	item->veloX *= 0.99;
        if(item->veloX >0 && item->veloX <0.1) item->veloX=0;
        else if(item->veloX <0 && item->veloX >-0.1) item->veloX=0;
	item->veloY *= 0.99;
        if(item->veloY >0 && item->veloY <0.1) item->veloY=0;
        else if(item->veloY <0 && item->veloY >-0.1) item->veloY=0;
        item->setOffset(item->offset + (item->veloX));
        if(item->veloX==0 && item->veloY==0) {
            item->scrollState = still;
            return 0; // stop ticker
        }
    } else {return 0;} // stop ticker
    return interval;
}

//////////////////////////////////////////////////////////////
//        N E W S   M A N A G E M E N T   C L A S S E S
//////////////////////////////////////////////////////////////

struct BookmarkPane:DisplayItem {
    BookmarkPane(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){};
    ~BookmarkPane(){};
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.2,0.2,0.4, 0.8);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, 0.8,0.8,1.0, 1);
        cairo_move_to(cr,30,10); renderText(cr,"Bookmarks"); cairo_fill(cr);
        // Items like: Important to you, trending, comedy, entertainment, sensational, editorial, ...
        return 1;
    }
};

typedef map<int64_t, factoid*> ItemCache;
typedef ItemCache::iterator ItemCacheItr;
typedef pair<int64_t, factoid*> ItemCacheVal;

struct TimelineView:ScrollingDispItem {
    infon* timeLineData;
    infon* filter;
    UInt filterMode;
    timeUnits timeUnit;
    viewModes viewMode;
    string title;

    ItemCache cache;

    void setUnit(timeUnits unit);
    void setDisplayStart();
    void setDisplaySpan();
    void setFilter();
    void setFilterMode();
    void setViewMode();
    void setTitle();

    void fetchPrev(ItemCacheItr &item){--item;};
    void fetchNext(ItemCacheItr &item){++item;};

    TimelineView(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):ScrollingDispItem(Parent,X,Y,W,H)
        {span=501; offset=span+1;};
    ~TimelineView(){};
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0,0,1,1);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_stroke(cr);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_set_source_rgba(cr, 0,0,0,1);
	cairo_fill(cr);
        int64_t leftIdx=offset-span;
        scale=span/w; //cout<<"scale:"<<scale<<" ";
        for(ItemCacheItr item=cache.lower_bound(leftIdx); item!=cache.end() && (*item).first <= offset; ++item){
            	int64_t idx=(*item).first;
            	double xPoint = (double)(idx-leftIdx)/scale;  // Offset in screen units
		if(xPoint < 600) {
			if(idx >= 0)
				cairo_set_source_rgba(cr, 1,1,1, 1);
			else 
				cairo_set_source_rgba(cr, 1,0,0, 1);
			roundedRectangle(cr, x+xPoint,y,2*item->second->importance,2*item->second->importance,20);
			cairo_stroke(cr);
		}
        }
//offset+=10;
        return 1;
    }
};

struct ItemsViewPane:DisplayItem {
    ItemsViewPane(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){};
    ~ItemsViewPane(){};
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0,0,0,0);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);
        return 1;
    }
};

typedef list<TimelineView*> TimeLines;
typedef list<TimelineView*>::iterator TimeLineItr;

struct  TimelinesPane:DisplayItem {
    TimelinesPane(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){
	    TimelineView* TLV=new TimelineView(this,X+5,Y+5,W-10,H-10);
	    for(int j=0; j<=400; ++j){
		int rIdx=rand() % 20000 - 10000;
		int rImport=rand() % 100;
		TLV->cache.insert(ItemCacheVal(rIdx, new factoid(rIdx, "Hello", rImport)));
	    }
	    timeLines.push_back(TLV);
    };
    ~TimelinesPane(){for(TimeLineItr i=timeLines.begin(); i!=timeLines.end(); ++i) delete(*i);};
    TimeLines timeLines;
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 1,0,0, 1);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_stroke(cr);
        for(TimeLineItr i=timeLines.begin(); i!=timeLines.end(); ++i){
            (*i)->dirty=1; (*i)->draw(cr);
        }
        return 1;
    }

    int handleEvent(SDL_Event &ev){
        if(!visible) return 0;
        for(TimeLineItr i=timeLines.begin(); i!=timeLines.end(); ++i)
            if((*i)->handleEvent(ev)) return 1;
        return 0;
    }
};

struct ItemView:DisplayItem {
    ItemView(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){};
    ~ItemView(){};
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.8,0.2,0.9, 0.8);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);
        return 1;
    }
};

struct NewsViewer:ScrollingDispItem {
    NewsViewer(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):
        //ScrollingDispItem(Parent,X,Y,W,H),
        //bookmarks(this, X+(W/3)*0+5,Y+10,W/3-10,H-20),
        timelines(this, X,Y,W,H)
        //itemView (this, X+(W/3)*2+5,Y+10,W/3-10,H-20)
        {srand (time(NULL)); WidgetWithCapturedMouse=0;};
    ~NewsViewer(){};
    //BookmarkPane bookmarks;
    TimelinesPane timelines;
    //ItemsViewPane itemView;
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        //cairo_set_source_rgba(cr, 0,1,1,1);
        //roundedRectangle(cr, x,y,w,h,20);
        //cairo_stroke(cr);
        //bookmarks.dirty=1; bookmarks.draw(cr);
        timelines.dirty=1; timelines.draw(cr);
        //itemView.dirty=1;  itemView.draw(cr);
        return 1;
    }

    int handleEvent(SDL_Event &ev){
        if(!visible) return 0;
        if(WidgetWithCapturedMouse) {WidgetWithCapturedMouse->handleEvent(ev); return 1;}
        if(timelines.handleEvent(ev)) return 1;
        //if(itemView.handleEvent(ev))  return 1;
        //if(bookmarks.handleEvent(ev)) return 1;
        return 0;
    }
};


NewsViewer* newsViewer;
