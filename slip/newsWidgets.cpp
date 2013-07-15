/// Copyright 2013 Bruce Long. All rights reserved.
// This file is intended to be included in slip.cpp.

enum filterModes {
    fmReadOnly=1, fmUnreadOnly=2, fmReadUnread=3,
    fmUserKnownOnly=4, fmUserDoesntKnowOnly=8, fmKnownBoth=12,
    fmVerifiedNewsOnly=16, fmReliableSourceOnly=32,
    fmComedy=64, fmGossip=128, fmNoViolence=256
    // friendsKnow, dontCare, etc.
};
const uint64_t yearTicks=0x40000000;
const uint64_t secTicks=yearTicks/31557600;  // 34 ticks/sec.  (should be 34.024825209)
const uint64_t minTicks=2041;
const uint64_t hourTicks=122489;
const uint64_t dayTicks=yearTicks/365.25;
const uint64_t weekTicks=dayTicks*7;
const int secondsPerDay=24*60*60;
const uint64_t yearsFrom0=14000000000;

const int daysFromJanuary[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
uint64_t timeToUnivTicks(tm* time, int64_t year=0){ // If year=0, use the year in tm.
	uint64_t year_Ticks=(yearsFrom0+((year!=0)?year:(time->tm_year+1900))) << 30;
	uint dayNum = daysFromJanuary[time->tm_mon] + time->tm_mday + (((time->tm_mon>1) && ( (year % 4 == 0) && ( year % 100 != 0 || year % 400 == 0 )))?1:0);
	uint secondsToday = time->tm_hour*3600 + time->tm_min*60 + time->tm_sec;
	
	return year_Ticks + dayNum*dayTicks + secondsToday*secTicks;  // TODO: There are rounding errors here. For daily news this should be OK.
}

enum timeUnits {uSecond, uMinute, uHour, uDay, uWeek, uMonth, uYear, uDecade, uCentury};
enum viewModes {vmHorizontal=0, vmVertical=1, vmTimeLine=2, vmCalendar=4};
enum deviceUI {uiTouch, uiKbdMouse, uiWeb};
enum OSBrand {osApple, osAndroid, osPC, osLinux};
enum scnDensity {sdComfortable, sdCozy, sdCompact};
enum skinStyle {skinLight, skinDark, skinBlue, skinMetal, skinUnicorn}; // Colors, graphics, animations, etc.

uint64_t ticksInUnit(timeUnits unit){
	switch(unit){
		case uSecond: return secTicks;
		case uMinute: return minTicks;
		case uHour:   return hourTicks;
		case uDay:    return dayTicks;
		case uWeek:   return weekTicks;
		case uYear:   return yearTicks;
		case uDecade: return yearTicks*10;
		case uCentury:return yearTicks*100;
		case uMonth:  return yearTicks/12;   // Do not use for calendar months.
	};
}

uint64_t nextInterval(uint64_t idx, timeUnits unit){
	if(unit!=uMonth) return idx+ticksInUnit(unit);
	uint64_t year=idx>>30;
	if(year<yearsFrom0+1753) return idx+ticksInUnit(unit); // TODO: Make this more intelligent.
	return 0; // TODO: Make months work.
	
}

uint64_t prevInterval(uint64_t idx, timeUnits unit){
	if(unit!=uMonth) return idx-ticksInUnit(unit);
	uint64_t year=idx>>30;
	if(year<yearsFrom0+1753) return idx-ticksInUnit(unit); // TODO: Make this more intelligent.
	return 0; // TODO: Make months work.
	
}

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

int areaForItem(int importance){
	return 1000*importance;
}

#define rand64(max) ((((uint64_t) rand() <<  0) & 0x000000000000FFFFull) | (((uint64_t) rand() << 16) & 0x00000000FFFF0000ull) | (((uint64_t) rand() << 32) & 0x0000FFFF00000000ull) | (((uint64_t) rand() << 48) & 0xFFFF000000000000ull) % (max))

factoid* randomfact(uint64_t startTime, uint64_t timespan){
	factoid* f=new factoid(startTime+rand64(timespan), "Science cures Microsoft", rand()%7);
	
    f->picWidth=50+rand()%300;
    f->picHeight=f->picWidth - ((rand()%(f->picWidth/2)));
//    f->topicCode=rand()%5;
//    f->height=f->topicCode*200;
	
	int topicRatio=rand()%100;
	if(topicRatio<70) f->topicCode=0;
	else if(topicRatio<80) f->topicCode=1;
	else if(topicRatio<88) f->topicCode=2;
	else if(topicRatio<92) f->topicCode=3;
	else if(topicRatio<98) f->topicCode=4;
    
    switch(f->topicCode){
		case 0: f->topic="science"; break;
		case 1: f->topic="politics"; f->headline="Senator denies being 'Jolly'"; break;
		case 2: f->topic="entertainment:sports"; f->headline="Kim-K marries Hobo";break;
		case 3: f->topic="colorado:boulder"; f->headline="Boulder 'fracked' says local."; break;
		case 4: f->topic="US"; f->headline="Earth swallows family alive"; break;
	}
    return f;
    
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
const double moveThreshold = 1;

struct ScrollingDispItem:DisplayItem {
    ScrollingDispItem(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):DisplayItem(Parent,X,Y,W,H){
        timestamp=0; scrollState=still;
        downX=downY=prevX=prevY=0; veloX=veloY=0;
    };
    ~ScrollingDispItem(){};
    /////// Scrolling functionality
//    static const double moveThreshold = 1;
    static const int interval=20; // Tick interval
    double veloX, veloY, scale;
    int downX, downY, prevX, prevY;
    UInt timestamp;
    scrollStates scrollState;
    SDL_TimerID timer_id;

    virtual void setOffset(double offset){} // Override this to handle movement, check boundaries, etc.

    int handleEvent(SDL_Event &ev){
        int crntX=0, crntY=0; double delta_x, delta_y;
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
                    setOffset(-delta_x*scale);
                    uint ticks=SDL_GetTicks()-timestamp; timestamp = SDL_GetTicks();
                    veloX = delta_x*interval/ticks; veloY = delta_y*interval/ticks;
                    if(veloX==0 && veloY==0) {scrollState=still;}
                }
                break;
            case SDL_MOUSEWHEEL:
            break;
        }
        return 1;
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
        
        item->setOffset(item->veloX*item->scale);
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

const int navBarHeight=55;
const int RowSpace=3;     // Space in between topic rows.

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
			default:cairo_set_source_rgba(cr, 0.7,0.6,0.2, 0.8); break;
		}
        roundedRectangle(cr, X,Y,picWidth,picHeight,10);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0,0,0.1, 0.8);
        cairo_stroke(cr);
        if(fact->imagePath!="") {displayImage(cr, fact->imagePath.c_str(), X, Y, 1);}
        cairo_move_to(cr,X+5,Y+5); renderText(cr,fact->headline.c_str());
        cairo_set_source_rgba(cr, 1,1,1, 1); cairo_fill_preserve(cr); cairo_set_source_rgba(cr, 0,0,0, 1); cairo_stroke(cr);
        return 1;
    }
    int handleEvent(SDL_Event &ev){
		if(!visible) return 0;

		return 0;
    }
};

typedef map<int64_t, ItemView*> ItemCache;
typedef ItemCache::iterator ItemCacheItr;
typedef pair<int64_t, ItemView*> ItemCacheVal;
ItemCache cache;

struct topicItem{
	uint numItemsInTopic, totalArea, Xcur, Ycur, height; 
	map<uint, uint> frontier;
	map<uint, uint>::iterator leader, follower;
	topicItem():numItemsInTopic(0),totalArea(0),Xcur(0),Ycur(0){};
};
typedef map<string, topicItem> ScnAreaMap; typedef ScnAreaMap::iterator ScnAreaMapItr;
ScnAreaMap scnAreas;

struct spanView:DisplayItem {
    spanView(DisplayItem* Parent, uint64_t start, uint64_t end)
		:DisplayItem(Parent), startIdx(start), endIdx(end){
		spanHeight=800+rand()%50;spanWidth=(rand()%200)+50;
		configLayout(scnAreas);
	};
	int64_t startIdx, endIdx;
	configLayout(ScnAreaMap &scnAreas){
		uint64_t maxWidth=100;
		for(ItemCacheItr itemItr=cache.lower_bound(startIdx); itemItr!=cache.end() && ((*itemItr).first < endIdx); ++itemItr){
           	int64_t idx=(*itemItr).first;
           	ItemView* item=(*itemItr).second;
           	topicItem &topicStats = scnAreas[item->fact->topic];
           	int64_t topicHeight=topicStats.height;
           	//--------------
           	uint minLeftMostPos=0, heightAtBest=0, leaderVal=0;
cout<<"#################   Topic:"<<item->fact->topic<<", "<<topicHeight<<", "<<item->picHeight<<"\n";
			if(topicStats.frontier.empty()){topicStats.frontier[0]=0;}
			topicStats.leader=topicStats.frontier.begin();
			topicStats.follower=topicStats.leader;
if(item->picHeight>=topicHeight) item->picHeight=60;			
           	uint HToPlace=item->picHeight;
           	while(topicStats.leader!=topicStats.frontier.end() && topicStats.leader->first < topicHeight){
cout<<"minleftMostPos:"<<minLeftMostPos<<" at "<<topicStats.follower->first<<flush;				
				if(topicStats.leader->first-topicStats.follower->first < HToPlace){ cout<<" too short\n";
					topicStats.leader++;
					if(topicStats.leader==topicStats.frontier.end()){leaderVal=topicHeight-1;}
					else leaderVal=topicStats.leader->first;
					// add to running area total
				} else { cout<<".\n";
					topicStats.follower++;
					// subtract from running area total
				}
				if(leaderVal-topicStats.follower->first >= HToPlace){
					uint leftMostPos=0;
					for(map<uint, uint>::iterator counter=topicStats.follower; counter!=topicStats.leader; counter++){
						if(counter->second>leftMostPos) leftMostPos=counter->second;
					}
					if(leftMostPos<minLeftMostPos || minLeftMostPos==0) {
						minLeftMostPos=leftMostPos; heightAtBest=topicStats.follower->first;
						cout<<"Best-so-far y:"<<leftMostPos<<" x:"<<heightAtBest<<"\n";
					}
				}
			}
			// Register the new rectangle in the frontier.
			uint crntLeft;
			map<uint, uint>::iterator delStart=topicStats.frontier.lower_bound(heightAtBest);
			map<uint, uint>::iterator delEnd=topicStats.frontier.lower_bound(heightAtBest+item->picHeight);
			if(delEnd==topicStats.frontier.end()) crntLeft=topicStats.frontier.rbegin()->second;
			else {
				if(delEnd!=delStart) delEnd--; 
				crntLeft=delEnd->second;
			}
cout<<"HAtBest:"<<heightAtBest<<", Deleting "<<delStart->first<<" to "<<delEnd->first<<"\n";				
			topicStats.frontier.erase(delStart, delEnd);
			topicStats.frontier[heightAtBest]=minLeftMostPos+item->picWidth;
			topicStats.frontier[heightAtBest+item->picHeight]=crntLeft;
		
           	item->y=heightAtBest;
           	item->x=minLeftMostPos;
           	
           	if(minLeftMostPos+item->picWidth>maxWidth) maxWidth=minLeftMostPos+item->picWidth;
 //          	int YOffset=0;
 //          	item->y=topicHeight+YOffset;
 //          	item->x=topicStats.Xcur; topicStats.Xcur+=item->picWidth;
           	//--------------
 //          	if(topicStats.Xcur>maxWidth) maxWidth=topicStats.Xcur;
		}
		spanWidth=maxWidth;
	};
    ~spanView(){};
	
    int spanWidth, spanHeight;
    int draw(cairo_t *cr, int X, int Y){
   //     if(!dirty || !visible) return 0; else dirty=false;
        cairo_set_source_rgba(cr, 0.5,6,0.2,1);
        roundedRectangle(cr, X,Y,spanWidth, spanHeight,20);
        cairo_fill(cr);
        int count=0;
        scale=1; //span/w;// cout<<"scale:"<<scale<<" ";
        for(ItemCacheItr itemItr=cache.lower_bound(startIdx); itemItr!=cache.end() && ((*itemItr).first < endIdx); ++itemItr){
           	int64_t idx=(*itemItr).first;
           	ItemView* item=(*itemItr).second;
           	
           	topicItem &topicStats = scnAreas[item->fact->topic];
           	uint &Xcur=topicStats.Xcur;
           	int64_t xPoint = X+item->x;  // Offset in screen units
        //   	if(xPoint+1000>w) break;
			item->dirty=1; item->draw(cr, xPoint,topicStats.Ycur+item->y);
			count++;
        } 
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

struct TimelineView:ScrollingDispItem {
	TimelineView(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):ScrollingDispItem(Parent,X,Y,W,H){
		scale=1;
	}
	ChooseTopicSizes(){
		uint64_t span, offset;
		time_t now_ticks, startTicks, endTicks;
		time (&now_ticks);
		startTicks=now_ticks-(30*secondsPerDay);
		endTicks=now_ticks+(30*secondsPerDay);
		
		span=dayTicks; offset=timeToUnivTicks(gmtime(&now_ticks));
		uint64_t leftIdx=offset-span;
      
        // Find approximate areas for topics
        uint totalItems=0, totalArea=0;
        for(ItemCacheItr itemItr=cache.lower_bound(leftIdx); itemItr!=cache.end() && ((*itemItr).first <= (uint64_t)offset); ++itemItr){
           	int64_t idx=(*itemItr).first;
           	ItemView* item=(*itemItr).second;
           	topicItem &topicStats = scnAreas[item->fact->topic];
           	uint itemsArea=areaForItem(item->fact->importance);
           	topicStats.numItemsInTopic++;
           	topicStats.totalArea+=itemsArea;
           	totalItems++; totalArea+=itemsArea;
		}
		
		// Find height ratios
		uint areaPerItem=totalArea/(totalItems+1);
		double H_unit=sqrt(totalArea) / 1.618;
		
		// For each topic, allocate a row on the screen
		int crntRowPos=navBarHeight;
		for(ScnAreaMapItr aItr=scnAreas.begin(); aItr!=scnAreas.end(); aItr++){
			topicItem &s=aItr->second;
			cout<<"AREA ITEM:"<<aItr->first<<"\t\t\t"<<s.numItemsInTopic<<"\t"<<s.totalArea<<"\n";
			s.height=H_unit/(totalArea/(s.totalArea+1));
			if(s.height<200) s.height=200;
			s.Ycur=crntRowPos;
			crntRowPos+=s.height+RowSpace;
			
		}
	};
	typedef map<int64_t, spanView*> Spans; typedef Spans::iterator SpanItr;
	Spans spans;
	timeUnits timeUnit;
	SpanItr leftMostSpan;
	int64_t backLeft;		// The amount of space in leftMostSpan that is off the left of the screen.
	int64_t spanUnit;
	
    void setOffset(double relOffset){cout<<"|"<<flush;
		moveRel(relOffset);
		markDirty();
		SDL_Event ev; ev.type = SDL_USEREVENT;  ev.user.code = 1;  ev.user.data1 = 0;  ev.user.data2 = 0;
		SDL_PushEvent(&ev);
    }
    
	void setUnit(timeUnits unit){timeUnit=unit;};
	
	// The 'move' commands fetch spans as needed then set leftMostSpan and backLeft.
	void moveTo(uint64_t idx){
		SpanItr ret=spans.find(idx);
		if(ret!=spans.end()) leftMostSpan=ret;
		else leftMostSpan=spans.insert(std::pair<int64_t,spanView*>(idx, new spanView(this, idx, nextInterval(idx,timeUnit)))).first;
		backLeft=0;
	};
	void moveToEndOf(int64_t idx);
	void moveRel(int64_t n){cout<<"N:"<<n<<"\n";
		int64_t W;
		if(n>0){
			W=leftMostSpan->second->spanWidth;
			n+=backLeft;
			while(n >= W){
				n-=W;
				fetchNext(leftMostSpan);
				W=leftMostSpan->second->spanWidth;
			}
			backLeft=n;
		} else if(n<0){
			n=-n;
			W=leftMostSpan->second->spanWidth;
			n += W-backLeft;
			while(n >= W){
				n-=W;
				fetchPrev(leftMostSpan);
				W=leftMostSpan->second->spanWidth;
			}
			backLeft=W-n;
		}
	}
	
	SpanItr fetchSpan(int64_t idx){
		SpanItr ret=spans.find(idx); 
		if(ret!=spans.end()) return ret;
		spans.insert(std::pair<int64_t,spanView*>(idx, new spanView(this, idx, nextInterval(idx,timeUnit)))).first;
	};
	
	void fetchNext(SpanItr &s){
		if(++s==spans.end()) {
			uint64_t nxtInterval=nextInterval(s->first, timeUnit);
			s=spans.insert(std::pair<int64_t,spanView*>(nxtInterval, new spanView(this, s->first,nxtInterval))).first;
		}
	};
	
	void fetchPrev(SpanItr &s){
		if(s-- == spans.begin()) {
			uint64_t prvInterval=prevInterval(s->first, timeUnit);
			s=spans.insert(std::pair<int64_t,spanView*>(prvInterval, new spanView(this, prvInterval, s->first))).first;
		}
	};
	
	int draw(cairo_t *cr){
        cairo_set_source_rgba(cr, 0,0,1,1);
        roundedRectangle(cr, x,y,w,h,20);
        cairo_fill(cr);
        
		int64_t xPoint=(0-backLeft)/scale;
		SpanItr item=leftMostSpan;
		while(xPoint<w){
		cout<<"*";
			item->second->draw(cr, xPoint, navBarHeight);
			xPoint+=item->second->spanWidth;
			fetchNext(item);
		}cout<<'.'<<flush;
		return 1;
	}
};

void makeFakeItems(){
	time_t now_ticks, startTicks, endTicks;
	time (&now_ticks);
	startTicks=now_ticks-(30*secondsPerDay);
	endTicks=now_ticks+(30*secondsPerDay);;
	for(time_t T=startTicks; T<=endTicks; T+=60*30){
		tm tm_T = *gmtime(&T);
		uint64_t UnivTicks=timeToUnivTicks(&tm_T);
		factoid *f=randomfact(UnivTicks, dayTicks/100);
		ItemView* IV=new ItemView(); 
		IV->time=f->time, IV->fact=f; IV->picWidth=f->picWidth; IV->picHeight=f->picHeight;
		cache.insert(ItemCacheVal(UnivTicks, IV));
	}
}

struct NewsViewer:DisplayItem{
    NewsViewer(DisplayItem* Parent=0, int X=0, int Y=0, int W=100, int H=50):
        navbar(this, X,Y,W,navBarHeight), timeline(this, X,navBarHeight,W,H-navBarHeight) {
			srand (time(NULL));
			WidgetWithCapturedMouse=0;
			
	makeFakeItems();			
			time_t now_ticks;
			time (&now_ticks);
			tm tm_T = *gmtime(&now_ticks);
			uint64_t UnivTicks=timeToUnivTicks(&tm_T);
			timeline.setUnit(uDay);
			timeline.ChooseTopicSizes();
			timeline.moveTo(UnivTicks);

			};
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


void RegisterArticle(infon* articleInfon){
	attrItr itr;
	string posted, updated, engTitle, summary, image, link, category, author, import;
	attrStore &attrS = *articleInfon->attrs;
	if((itr=attrS.find("engTitle"))!=attrS.end()){engTitle=itr->second;}
	if((itr=attrS.find("summary"))!=attrS.end()){summary=itr->second;}
	if((itr=attrS.find("posted"))!=attrS.end()){posted=itr->second;}
	if((itr=attrS.find("updated"))!=attrS.end()){updated=itr->second;}
	if((itr=attrS.find("image"))!=attrS.end()){image=itr->second;}
	if((itr=attrS.find("link"))!=attrS.end()){link=itr->second;}
	if((itr=attrS.find("category"))!=attrS.end()){category=itr->second;}
	if((itr=attrS.find("author"))!=attrS.end()){author=itr->second;}
	if((itr=attrS.find("import"))!=attrS.end()){import=itr->second;}
		
	struct tm *tm;  memset (tm, '\0', sizeof (*tm));
	const char * cp = strptime (posted.c_str(), "%m/%d/%Y %I:%M %p", tm);    // 06/28/2013 3:58 pm EDT
    if (cp == NULL){ /* Does not match.  Try the another form.  */
	   cp = strptime (posted.c_str(), "%m/%d/%Y %I:%M %p", tm);
	 }
    uint64_t Time=timeToUnivTicks(tm); 
cout<<"TIME:"<<tm->tm_year<<", "<<tm->tm_mon<<", "<<tm->tm_mday<<", "<<tm->tm_hour<<", "<<image<<", "<<Time<<"\n";
	factoid *f=new factoid(Time, engTitle, atoi(import.c_str()));
	f->imagePath=string("/home/bruce/proteus/resources/news/github.com/BruceDLong/NewsTest.git/")+image;
	f->topic=category;
	if     (category=="science") {f->topicCode=0;}
	else if(category=="politics"){f->topicCode=1;}
	else if(category=="sports")  {f->topicCode=2;}
	else if(category=="boulder") {f->topicCode=3;}
	else if(category=="US")      {f->topicCode=4;}
	
	f->picWidth=50+rand()%300;
    f->picHeight=f->picWidth + ((rand()%(f->picWidth/2))-(f->picWidth/4));
    f->height=f->topicCode*200+rand()%50;   
    
    ItemView* IV=new ItemView(); 
    IV->time=f->time, IV->fact=f; IV->picWidth=f->picWidth; IV->picHeight=f->picHeight;
    cache.insert(ItemCacheVal(Time, IV));
}

NewsViewer* newsViewer;
