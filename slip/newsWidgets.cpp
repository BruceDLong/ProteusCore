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

    factoid(int64_t idx, string headLine){index=idx; headline=headLine;}
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
        velocityX=velocityY=0; tmpOffset=offset=0; span=0; timestamp=0; scrollState=still;
        ptrX=ptrY=0; skewX=skewY=0; deltaX=deltaY=0;
    };
    ~ScrollingDispItem(){};
    /////// Scrolling functionality
    const int moveThreshold = 10;
    double tmpOffset, offset, span;
    int velocityX, velocityY;
    UInt timestamp;
    int ptrX, ptrY, skewX, skewY, deltaX, deltaY;
    scrollStates scrollState;
    SDL_TimerID timer_id;

    void setTmpOffset(double Offset){
        double off=Offset;
        if(off != tmpOffset){
            if(off< -10000) off=-10000;
            if(off> 10000) off=10000;
            tmpOffset=off;
            markDirty();
            SDL_Event ev; ev.type = SDL_USEREVENT;  ev.user.code = 0;  ev.user.data1 = 0;  ev.user.data2 = 0;
            SDL_PushEvent(&ev);
cout<<"OFFSET:"<<tmpOffset<<"\n";
        }
    }

    int handleEvent(SDL_Event &ev){
        int tmpX, tmpY, delta_x, delta_y;
        switch (ev.type) {
            case SDL_MOUSEBUTTONDOWN:
                if(ev.button.button!=SDL_BUTTON_LEFT) return 0;
                tmpX=ev.button.x; tmpY=ev.button.y;
                if(tmpX<=x || tmpX>(x+w) || tmpY<y || tmpY>(y+h)) return 0;
                captureMouseMsgs(this);
                if(scrollState==still){                  // Finger Pressed while still
                    scrollState=fingerPressed;
                    ptrX=tmpX; ptrY=tmpY;
                } else if(scrollState==freeScrolling){  // Finger Pressed while free-scrolling
                    scrollState=stopping;
                    velocityX=0; velocityY=0;
                    ptrX=tmpX; ptrY=tmpY;
                    offset=tmpOffset;
                    SDL_RemoveTimer(timer_id);  // NOTE: It is not safe to remove a timer multiple times.

                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(ev.button.button!=SDL_BUTTON_LEFT) return 0;
                releaseMouseMsgs();
                tmpX=ev.button.x; tmpY=ev.button.y;
                if(scrollState==fingerPressed){          // Finger released while pressed
                    scrollState=still;
                    skewX=0; skewY=0;
//                     for visible items, handleEvent(ev)

                } else if(scrollState==pullScrolling){   // Finger released while pull-scrolling
                    delta_x=ev.button.x-ptrX; delta_y=ev.button.y-ptrY;
                    if (SDL_GetTicks() - timestamp > 100) {
                        timestamp = SDL_GetTicks();
                        velocityX = delta_x - deltaX; velocityY = delta_y - deltaY;
                        deltaX=delta_x; deltaY=delta_y;
                    }
                    offset=tmpOffset;
                    ptrX=tmpX; ptrY=tmpY;
                    if(velocityX==0 && velocityY==0) {scrollState=still;}
                    else{
                   //     velocityX /= 4; velocityY /= 4;
                        scrollState=freeScrolling;
                        timer_id = SDL_AddTimer(20, tick, this);

                    }
                } else if(scrollState==stopping){        // Finger released while stopping/slowing.
                    scrollState=still;
                    skewX=0; skewY=0;
                    offset=tmpOffset;
                }
                break;
            case SDL_MOUSEMOTION:
                if(ev.button.button!=SDL_BUTTON_LEFT) return 0;
                tmpX=ev.motion.x; tmpY=ev.motion.y;
                if(scrollState==fingerPressed || scrollState==stopping){ // Finger moves while just pressed or stopping
                    delta_x=ev.button.x-ptrX; delta_y=ev.button.y-ptrY;
                    if(delta_x > moveThreshold || delta_x < -moveThreshold ||
                        delta_y > moveThreshold || delta_y < -moveThreshold){
                            timestamp = SDL_GetTicks();
                            scrollState=pullScrolling;
                            deltaX=deltaY=0;
                            ptrX=tmpX; ptrY=tmpY;
                    }
                } else if(scrollState==pullScrolling){     // Finger moves, pull-scrolling the view.
                    delta_x=ev.motion.x-ptrX; delta_y=ev.motion.y-ptrY;
                    setTmpOffset(offset - deltaX); offset=tmpOffset;
        cout<<"#\n";
                    if (SDL_GetTicks() - timestamp > 100) {
                        timestamp = SDL_GetTicks();
                        velocityX = delta_x - deltaX; velocityY = delta_y - deltaY;
                        deltaX=delta_x; deltaY=delta_y;
                    }
                }
                break;
            case SDL_MOUSEWHEEL:
            break;
        }
        return 0;
    }
};

uint32_t tick(uint32_t interval, void *param){
    ScrollingDispItem *item=(ScrollingDispItem*)param;
    if (item->scrollState == freeScrolling) {
 //       if(--item->velocityX <0) item->velocityX=0;
 //       if(--item->velocityY <0) item->velocityY=0;
        item->setTmpOffset(item->offset - item->velocityX);
        item->offset = item->tmpOffset;
        if(item->velocityX==0 && item->velocityY==0) {
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
        {span=500; offset=500;};
    ~TimelineView(){};
    int draw(cairo_t *cr){
  //      if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.5,0.5,0.6, 0.9);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);

        int64_t leftIdx=offset-span;
        double scale=span/w; //cout<<scale<<" ";
        cairo_set_source_rgba(cr, 0.8,0.8,1.0, 1);
        for(ItemCacheItr item=cache.lower_bound(leftIdx); item!=cache.end() && (*item).first <= offset; ++item){
            int64_t idx=(*item).first;
            double xPoint = (double)(idx-leftIdx)/scale;  // Offset in screen units.
            cairo_move_to(cr,x+xPoint, y+10); renderText(cr,"XYZ"); cairo_fill(cr);
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
        cairo_set_source_rgba(cr, 0.5,0.5,1.0, 0.8);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);
        return 1;
    }
};

typedef list<TimelineView*> TimeLines;
typedef list<TimelineView*>::iterator TimeLineItr;

struct  TimelinesPane:DisplayItem {
    TimelinesPane(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){
        int hi=50;
        for(int i=0; i<4; ++i){
            TimelineView* TLV=new TimelineView(this,X+1,Y+((hi+10)*i)+40,W-2,hi);
            for(int j=0; j<=200; ++j){
                int rIdx=rand() % 10000;
                TLV->cache.insert(ItemCacheVal(rIdx, new factoid(rIdx, "Hello")));
            }
            timeLines.push_back(TLV);
        }
    };
    ~TimelinesPane(){for(TimeLineItr i=timeLines.begin(); i!=timeLines.end(); ++i) delete(*i);};
    TimeLines timeLines;
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.5,0.5,0.8, 0.8);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);
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
        ScrollingDispItem(Parent,X,Y,W,H),
        bookmarks(this, X+(W/3)*0+5,Y+10,W/3-10,H-20),
        timelines(this, X+(W/3)*1+5,Y+10,W/3-10,H-20),
        itemView (this, X+(W/3)*2+5,Y+10,W/3-10,H-20)
        {srand (time(NULL)); WidgetWithCapturedMouse=0;};
    ~NewsViewer(){};
    BookmarkPane bookmarks;
    TimelinesPane timelines;
    ItemsViewPane itemView;
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.8,0.2,0.9, 1);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_stroke(cr);
        bookmarks.dirty=1; bookmarks.draw(cr);
        timelines.dirty=1; timelines.draw(cr);
        itemView.dirty=1;  itemView.draw(cr);
        return 1;
    }

    int handleEvent(SDL_Event &ev){
        if(!visible) return 0;
        if(WidgetWithCapturedMouse) {WidgetWithCapturedMouse->handleEvent(ev); return 1;}
        if(timelines.handleEvent(ev)) return 1;
        if(itemView.handleEvent(ev))  return 1;
        if(bookmarks.handleEvent(ev)) return 1;
        return 0;
    }
};


NewsViewer* newsViewer;
