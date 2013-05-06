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
    enum scrollStates {still, fingerPressed, pullScrolling, freeScrolling, stopping};
    DisplayItem(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):dirty(0),visible(1),x(X),y(Y),w(W),h(H),parent(Parent){};
    bool dirty,visible;             // If either is false, draw() should usually just return 0.
    int x,y,w,h,scale;              // Position, width, height, scale.
    DisplayItem* parent;           // Use to tell parent you are dirty -- need to be redrawn.
    cairo_t *cr;                    // Cairo drawing context;
    virtual int draw(cairo_t *cr){return 0;}; // Override to draw your item.
    virtual int handleEvent(){return 0;};     // Override to handle events
    virtual ~DisplayItem(){};

};

struct ScrollingDispItem:DisplayItem {
    ScrollingDispItem(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){};
    ~ScrollingDispItem(){};
    /////// Scrolling functionality
    int velocity;
    int offset;
    int timestamp;
    scrollStates scrollState;

    void onHover();
    void onFingerDown();
    void onFingerUp();
    void onFingerMoves();
    void onTwoFingersDown();
    void onTwoFingersUp();
    void onTwoFingersMove();
};

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
        return 1;
    }
};

struct TimelineView:DisplayItem {
    infon* timeLineData;
    infon* filter;
    UInt filterMode;
    timeUnits timeUnit;
    viewModes viewMode;
    string title;

    map<int64_t, factoid> cache;

    void setUnit(timeUnits unit);
    void setDisplayStart();
    void setDisplaySpan();
    void setFilter();
    void setFilterMode();
    void setViewMode();
    void setTitle();

    void fetchPrev(map<int64_t, factoid>::iterator item);
    void fetchNext(map<int64_t, factoid>::iterator item);

    TimelineView(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){};
    ~TimelineView(){};
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.5,0.5,0.6, 0.9);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);
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
            timeLines.push_back(new TimelineView(this,X+1,Y+((hi+10)*i)+40,W-2,hi));
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
        {};
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
};


NewsViewer* newsViewer;
