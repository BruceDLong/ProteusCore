/// Copyright 2013 Bruce Long. All rights reserved.
// This file is intended to be included in slip.cpp.

enum filterModes {
    fmReadOnly=1, fmUnreadOnly=2, fmReadUnread=3,
    fmUserKnownOnly=4, fmUserDoesntKnowOnly=8, fmKnownBoth=12,
    fmVerifiedNewsOnly=16, fmReliableSourceOnly=32,
    fmComedy=64, fmGossip=128, fmNoViolence=256
    // friendsKnow, dontCare, etc.
};
const uint64_t year_ticks=0x40000000;
const uint64_t day_ticks=year_ticks/365.25;
const uint64_t sec_ticks=year_ticks/31557600;  // 34 ticks/sec.  (should be 34.024825209)
const uint64_t week_ticks=day_ticks*7;
const int secondsPerDay=24*60*60;

const int daysFromJanuary[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
uint64_t timeToUnivTicks(tm* time, int64_t year=0){ // If year=0, use the year in tm.
	uint64_t yearTicks=(14000000000+((year!=0)?year:(time->tm_year+1900))) << 30;
	uint dayNum = daysFromJanuary[time->tm_mon] + time->tm_mday + (((time->tm_mon>1) && ( (year % 4 == 0) && ( year % 100 != 0 || year % 400 == 0 )))?1:0);
	uint secondsToday = time->tm_hour*3600 + time->tm_min*60 + time->tm_sec;
	
	return yearTicks + dayNum*day_ticks + secondsToday*sec_ticks;  // TODO: There are rounding errors here. For daily news this should be OK.
}

enum timeUnits {uSecond, uMinute, uHour, uWorkDay, uDay, uWeek, uWorkWeek, uMonth, uYear, uDecade, uCentury};
enum viewModes {vmHorizontal=0, vmVertical=1, vmTimeLine=2, vmCalendar=4};
enum deviceUI {uiTouch, uiKbdMouse, uiWeb};
enum OSBrand {osApple, osAndroid, osPC, osLinux};
enum scnDensity {sdComfortable, sdCozy, sdCompact};
enum skinStyle {skinLight, skinDark, skinBlue, skinMetal, skinUnicorn}; // Colors, graphics, animations, etc.

struct factoid {
    int64_t time;
    int height;
    string headline;
    string imagePath;
    uint picWidth, picHeight;
    int importance;
    bool hasRead;
    bool havent_read_dontWantTo;
    int userKnows;
    string topic; int topicCode;
    // friends who know...
    // list<links>; // links to articles, videos, people, etc.
    // list<sources>;

    factoid(int64_t Time, string headLine, int import){time=Time; headline=headLine; importance=import;}
};

#define rand64(max) ((((uint64_t) rand() <<  0) & 0x000000000000FFFFull) | (((uint64_t) rand() << 16) & 0x00000000FFFF0000ull) | (((uint64_t) rand() << 32) & 0x0000FFFF00000000ull) | (((uint64_t) rand() << 48) & 0xFFFF000000000000ull) % (max))

factoid* randomfact(uint64_t startTime, uint64_t timespan){
	factoid* f=new factoid(startTime+rand64(timespan), "Science cures Microsoft", rand()%7);
    f->picWidth=50+rand()%300;
    f->picHeight=f->picWidth + ((rand()%(f->picWidth/2))-(f->picWidth/4));
    f->topicCode=rand()%5;
    f->height=f->topicCode*200;
    switch(f->topicCode){
		case 0: f->topic="science"; break;
		case 1: f->topic="politics"; f->headline="Senator denies being 'Jolly'"; break;
		case 2: f->topic="entertainment:sports"; f->headline="Kim-K marries Hobo";break;
		case 3: f->topic="colorado:boulder"; f->headline="Boulder 'fracked' says local."; break;
		case 4: f->topic="US"; f->headline="Earth swallows family alive"; break;
	}
    return f;
    
}

void RegisterArticle(infon* articalInfon){ cout<<"AT-A\n"<<flush;
	string time= articalInfon->attrs->at("posted");
	string headLine=articalInfon->attrs->at("engTitle");
	string summary=articalInfon->attrs->at("summary");
	string image=articalInfon->attrs->at("image");
	string link=articalInfon->attrs->at("link");
	string category=articalInfon->attrs->at("category");
	string author=articalInfon->attrs->at("author");
	string import=articalInfon->attrs->at("import");
cout<<"AT-B\n"<<flush;	
	struct tm *tm;  memset (tm, '\0', sizeof (*tm));
	const char * cp = strptime (time.c_str(), "%F %r", tm);
    if (cp == NULL){ /* Does not match.  Try the US form.  */
	   cp = strptime (time.c_str(), "%D %r", tm);   // 06/28/2013 3:58 pm EDT
	 }
    uint64_t Time=timeToUnivTicks(tm);
cout<<"AT-C\n"<<flush;	
	factoid *f=new factoid(Time, headLine, atoi(import.c_str()));
	f->imagePath=image;
	f->topic=category;
	if     (category=="science") {f->topicCode=0;}
	else if(category=="politics"){f->topicCode=1;}
	else if(category=="sports")  {f->topicCode=2;}
	else if(category=="boulder") {f->topicCode=3;}
	else if(category=="US")      {f->topicCode=4;}
	
	f->picWidth=50+rand()%300;
    f->picHeight=f->picWidth + ((rand()%(f->picWidth/2))-(f->picWidth/4));
    f->height=f->topicCode*200+rand()%50;
cout<<"AT-E\n"<<flush;    
}


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
    uint64_t offset, span;
    double downX, downY, prevX, prevY, veloX, veloY, scale;
    UInt timestamp;
    scrollStates scrollState;
    SDL_TimerID timer_id;

    void setOffset(double off){
        if(off != offset){
    //        if(off< -10000) off=-10000;
    //        if(off> 10000) off=10000;
            offset=off;
		}
            markDirty();
            SDL_Event ev; ev.type = SDL_USEREVENT;  ev.user.code = 1;  ev.user.data1 = 0;  ev.user.data2 = 0;
            SDL_PushEvent(&ev);
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

const double friction=0.1;  // The rate at which scrolling decelerates.
void adjustVelocity(double &velocity){
	if (velocity<0){
		velocity+=friction;
		if(velocity>0) velocity=0;
	} else if (velocity>0){
		velocity-=friction;
		if(velocity<0) velocity=0;
	}
}

uint32_t tick(uint32_t interval, void *param){ cout<<':'<<flush;
    ScrollingDispItem *item=(ScrollingDispItem*)param;
    if (item->scrollState == freeScrolling) {
		adjustVelocity(item->veloX);
		adjustVelocity(item->veloY);
        
        item->setOffset(item->offset + (item->veloX*item->scale));
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

struct ItemView:DisplayItem {
    ItemView(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){};
    ~ItemView(){};
    factoid* fact;
    uint64_t time;
    int picWidth, picHeight;
    int draw(cairo_t *cr, int X, int Y){
  //      if(!dirty || !visible) return 0; else dirty=false;
        switch(fact->topicCode){
			case 0: cairo_set_source_rgba(cr, 0.5,0.4,1.0, 0.8); break;
			case 1: cairo_set_source_rgba(cr, 0.6,0.2,0.4, 0.8); break;
			case 2: cairo_set_source_rgba(cr, 0.3,0.8,0.1, 0.8); break;
			case 3: cairo_set_source_rgba(cr, 0.7,0.8,0.7, 0.8); break;
			default: cairo_set_source_rgba(cr, 0.7,0.6,0.2, 0.8);break;
		}
        roundedRectangle(cr, X,Y,picWidth,picHeight,10);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0,0,0.1, 0.8);
        cairo_stroke(cr);
        cairo_move_to(cr,X+5,Y+picHeight/2); renderText(cr,fact->headline.c_str()); cairo_fill(cr);
        return 1;
    }
    int handleEvent(SDL_Event &ev){
		if(!visible) return 0;

		return 0;
    }
};

struct navbarPane:DisplayItem {
    navbarPane(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){};
    ~navbarPane(){};
    int draw(cairo_t *cr){
        if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.2,0.2,0.4, 0.8);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, 0.8,0.8,1.0, 1);
        cairo_move_to(cr,60,18); renderText(cr,"<      Refresh      | . . . . | . . . .| June 24 2013 | . . . .| . . . . |      View      >"); cairo_fill(cr);
        // Items like: Important to you, trending, comedy, entertainment, sensational, editorial, ...
        return 1;
    }
};

typedef map<int64_t, ItemView*> ItemCache;
typedef ItemCache::iterator ItemCacheItr;
typedef pair<int64_t, ItemView*> ItemCacheVal;

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

    void fetchPrev(ItemCacheItr &itemItr){--itemItr;};
    void fetchNext(ItemCache* cache, ItemCacheItr &itemItr){
		if(std::next(itemItr)==cache->end()){
			factoid *f=randomfact((*itemItr).first, day_ticks/100);
			ItemView* IV=new ItemView(); 
			IV->time=f->time, IV->fact=f; IV->picWidth=f->picWidth; IV->picHeight=f->picHeight;
			cache->insert(ItemCacheVal(f->time, IV)); 
		}
		itemItr++;
		
	};

    TimelineView(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):ScrollingDispItem(Parent,X,Y,W,H){
		  time_t now_ticks, startTicks, endTicks;
		  time (&now_ticks);
		  startTicks=now_ticks-(30*secondsPerDay);
		  endTicks=now_ticks+(30*secondsPerDay);;
		  for(time_t T=startTicks; T<=endTicks; T+=60*60){
			  tm tm_T = *gmtime(&T);
			  uint64_t UnivTicks=timeToUnivTicks(&tm_T);
			  factoid *f=randomfact(UnivTicks, day_ticks/100);
			  ItemView* IV=new ItemView(); 
			  IV->time=f->time, IV->fact=f; IV->picWidth=f->picWidth; IV->picHeight=f->picHeight;
			  cache.insert(ItemCacheVal(UnivTicks, IV)); 
		  }
		  span=day_ticks; offset=timeToUnivTicks(gmtime(&now_ticks));
		
	};
    ~TimelineView(){};
    int draw(cairo_t *cr){
   //     if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0,0,1,1);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);
        int count=0;
        int64_t leftIdx=offset-span;
        scale=span/w;// cout<<"scale:"<<scale<<" ";
        for(ItemCacheItr itemItr=cache.lower_bound(leftIdx); itemItr!=cache.end() && ((*itemItr).first <= (uint64_t)offset); ++itemItr){
           	int64_t idx=(*itemItr).first;
           	ItemView* item=(*itemItr).second;
           	int64_t xPoint = (int64_t)(idx-leftIdx)/scale;  // Offset in screen units
			item->dirty=1; item->draw(cr, xPoint,55+item->fact->height);
			count++;
        }
        cout<<count<<" ";
        return 1;
    }
};


struct NewsViewer:DisplayItem{
    NewsViewer(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):
        navbar(this, X,Y,W,50), timeline(this, X,50,W,H-50) {srand (time(NULL)); WidgetWithCapturedMouse=0;};
    ~NewsViewer(){};
    navbarPane navbar;
    TimelineView timeline;
    int draw(cairo_t *cr){
      //  if(!dirty || !visible) return 0; else dirty=false;
        navbar.dirty=1; navbar.draw(cr);
        timeline.dirty=1; timeline.draw(cr);
        return 1;
    }

    int handleEvent(SDL_Event &ev){
        if(!visible) return 0;
        if(WidgetWithCapturedMouse) {WidgetWithCapturedMouse->handleEvent(ev); return 1;}
        if(timeline.handleEvent(ev)) return 1;
        if(navbar.handleEvent(ev)) return 1;
        return 0;
    }
};


NewsViewer* newsViewer;
