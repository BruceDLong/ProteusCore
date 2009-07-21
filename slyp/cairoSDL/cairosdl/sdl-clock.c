<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>
<head>
<title>~joonas/cairosdl - Drawing to SDL surfaces with cairo.</title>
<meta name='generator' content='cgit v0.8.2.1'/>
<meta name='robots' content='index, nofollow'/>
<link rel='stylesheet' type='text/css' href='http://cgit.freedesktop.org/cgit.css'/>
<link rel='alternate' title='Atom feed' href='http://cgit.freedesktop.org/~joonas/cairosdl/atom/sdl-clock.c?h=demos' type='application/atom+xml'/></head>
<body>
<table id='header'>
<tr>
<td class='logo' rowspan='2'><a href='/'><img src='http://cgit.freedesktop.org/logo.png' alt='cgit logo'/></a></td>
<td class='main'><a href='/'>index</a> : <a title='~joonas/cairosdl' href='/~joonas/cairosdl/'>~joonas/cairosdl</a></td><td class='form'><form method='get' action=''>
<select name='h' onchange='this.form.submit();'>
<option value='demos' selected='selected'>demos</option>
<option value='master'>master</option>
</select> <input type='submit' name='' value='switch'/></form></td></tr>
<tr><td class='sub'>Drawing to SDL surfaces with cairo.</td><td class='sub right'>joonas</td></tr></table>
<table class='tabs'><tr><td>
<a href='/~joonas/cairosdl/?h=demos'>summary</a><a href='/~joonas/cairosdl/refs/?h=demos'>refs</a><a href='/~joonas/cairosdl/log/?h=demos'>log</a><a class='active' href='/~joonas/cairosdl/tree/?h=demos'>tree</a><a href='/~joonas/cairosdl/commit/?h=demos'>commit</a><a href='/~joonas/cairosdl/diff/?h=demos'>diff</a></td><td class='form'><form class='right' method='get' action='/~joonas/cairosdl/log/sdl-clock.c'>
<input type='hidden' name='h' value='demos'/><select name='qt'>
<option value='grep'>log msg</option>
<option value='author'>author</option>
<option value='committer'>committer</option>
</select>
<input class='txt' type='text' size='10' name='q' value=''/>
<input type='submit' value='search'/>
</form>
</td></tr></table>
<div class='content'>path: <a href='/~joonas/cairosdl/tree/?h=demos'>root</a>/<a href='/~joonas/cairosdl/tree/sdl-clock.c?h=demos'>sdl-clock.c</a> (<a href='/~joonas/cairosdl/plain/sdl-clock.c?h=demos'>plain</a>)<br/>blob: 71c0ca02ee0b44062672e2d4e15ffa30a00eaacf
<table summary='blob content' class='blob'>
<tr><td class='linenumbers'><pre><a class='no' id='n1' name='n1' href='#n1'>1</a>
<a class='no' id='n2' name='n2' href='#n2'>2</a>
<a class='no' id='n3' name='n3' href='#n3'>3</a>
<a class='no' id='n4' name='n4' href='#n4'>4</a>
<a class='no' id='n5' name='n5' href='#n5'>5</a>
<a class='no' id='n6' name='n6' href='#n6'>6</a>
<a class='no' id='n7' name='n7' href='#n7'>7</a>
<a class='no' id='n8' name='n8' href='#n8'>8</a>
<a class='no' id='n9' name='n9' href='#n9'>9</a>
<a class='no' id='n10' name='n10' href='#n10'>10</a>
<a class='no' id='n11' name='n11' href='#n11'>11</a>
<a class='no' id='n12' name='n12' href='#n12'>12</a>
<a class='no' id='n13' name='n13' href='#n13'>13</a>
<a class='no' id='n14' name='n14' href='#n14'>14</a>
<a class='no' id='n15' name='n15' href='#n15'>15</a>
<a class='no' id='n16' name='n16' href='#n16'>16</a>
<a class='no' id='n17' name='n17' href='#n17'>17</a>
<a class='no' id='n18' name='n18' href='#n18'>18</a>
<a class='no' id='n19' name='n19' href='#n19'>19</a>
<a class='no' id='n20' name='n20' href='#n20'>20</a>
<a class='no' id='n21' name='n21' href='#n21'>21</a>
<a class='no' id='n22' name='n22' href='#n22'>22</a>
<a class='no' id='n23' name='n23' href='#n23'>23</a>
<a class='no' id='n24' name='n24' href='#n24'>24</a>
<a class='no' id='n25' name='n25' href='#n25'>25</a>
<a class='no' id='n26' name='n26' href='#n26'>26</a>
<a class='no' id='n27' name='n27' href='#n27'>27</a>
<a class='no' id='n28' name='n28' href='#n28'>28</a>
<a class='no' id='n29' name='n29' href='#n29'>29</a>
<a class='no' id='n30' name='n30' href='#n30'>30</a>
<a class='no' id='n31' name='n31' href='#n31'>31</a>
<a class='no' id='n32' name='n32' href='#n32'>32</a>
<a class='no' id='n33' name='n33' href='#n33'>33</a>
<a class='no' id='n34' name='n34' href='#n34'>34</a>
<a class='no' id='n35' name='n35' href='#n35'>35</a>
<a class='no' id='n36' name='n36' href='#n36'>36</a>
<a class='no' id='n37' name='n37' href='#n37'>37</a>
<a class='no' id='n38' name='n38' href='#n38'>38</a>
<a class='no' id='n39' name='n39' href='#n39'>39</a>
<a class='no' id='n40' name='n40' href='#n40'>40</a>
<a class='no' id='n41' name='n41' href='#n41'>41</a>
<a class='no' id='n42' name='n42' href='#n42'>42</a>
<a class='no' id='n43' name='n43' href='#n43'>43</a>
<a class='no' id='n44' name='n44' href='#n44'>44</a>
<a class='no' id='n45' name='n45' href='#n45'>45</a>
<a class='no' id='n46' name='n46' href='#n46'>46</a>
<a class='no' id='n47' name='n47' href='#n47'>47</a>
<a class='no' id='n48' name='n48' href='#n48'>48</a>
<a class='no' id='n49' name='n49' href='#n49'>49</a>
<a class='no' id='n50' name='n50' href='#n50'>50</a>
<a class='no' id='n51' name='n51' href='#n51'>51</a>
<a class='no' id='n52' name='n52' href='#n52'>52</a>
<a class='no' id='n53' name='n53' href='#n53'>53</a>
<a class='no' id='n54' name='n54' href='#n54'>54</a>
<a class='no' id='n55' name='n55' href='#n55'>55</a>
<a class='no' id='n56' name='n56' href='#n56'>56</a>
<a class='no' id='n57' name='n57' href='#n57'>57</a>
<a class='no' id='n58' name='n58' href='#n58'>58</a>
<a class='no' id='n59' name='n59' href='#n59'>59</a>
<a class='no' id='n60' name='n60' href='#n60'>60</a>
<a class='no' id='n61' name='n61' href='#n61'>61</a>
<a class='no' id='n62' name='n62' href='#n62'>62</a>
<a class='no' id='n63' name='n63' href='#n63'>63</a>
<a class='no' id='n64' name='n64' href='#n64'>64</a>
<a class='no' id='n65' name='n65' href='#n65'>65</a>
<a class='no' id='n66' name='n66' href='#n66'>66</a>
<a class='no' id='n67' name='n67' href='#n67'>67</a>
<a class='no' id='n68' name='n68' href='#n68'>68</a>
<a class='no' id='n69' name='n69' href='#n69'>69</a>
<a class='no' id='n70' name='n70' href='#n70'>70</a>
<a class='no' id='n71' name='n71' href='#n71'>71</a>
<a class='no' id='n72' name='n72' href='#n72'>72</a>
<a class='no' id='n73' name='n73' href='#n73'>73</a>
<a class='no' id='n74' name='n74' href='#n74'>74</a>
<a class='no' id='n75' name='n75' href='#n75'>75</a>
<a class='no' id='n76' name='n76' href='#n76'>76</a>
<a class='no' id='n77' name='n77' href='#n77'>77</a>
<a class='no' id='n78' name='n78' href='#n78'>78</a>
<a class='no' id='n79' name='n79' href='#n79'>79</a>
<a class='no' id='n80' name='n80' href='#n80'>80</a>
<a class='no' id='n81' name='n81' href='#n81'>81</a>
<a class='no' id='n82' name='n82' href='#n82'>82</a>
<a class='no' id='n83' name='n83' href='#n83'>83</a>
<a class='no' id='n84' name='n84' href='#n84'>84</a>
<a class='no' id='n85' name='n85' href='#n85'>85</a>
<a class='no' id='n86' name='n86' href='#n86'>86</a>
<a class='no' id='n87' name='n87' href='#n87'>87</a>
<a class='no' id='n88' name='n88' href='#n88'>88</a>
<a class='no' id='n89' name='n89' href='#n89'>89</a>
<a class='no' id='n90' name='n90' href='#n90'>90</a>
<a class='no' id='n91' name='n91' href='#n91'>91</a>
<a class='no' id='n92' name='n92' href='#n92'>92</a>
<a class='no' id='n93' name='n93' href='#n93'>93</a>
<a class='no' id='n94' name='n94' href='#n94'>94</a>
<a class='no' id='n95' name='n95' href='#n95'>95</a>
<a class='no' id='n96' name='n96' href='#n96'>96</a>
<a class='no' id='n97' name='n97' href='#n97'>97</a>
<a class='no' id='n98' name='n98' href='#n98'>98</a>
<a class='no' id='n99' name='n99' href='#n99'>99</a>
<a class='no' id='n100' name='n100' href='#n100'>100</a>
<a class='no' id='n101' name='n101' href='#n101'>101</a>
<a class='no' id='n102' name='n102' href='#n102'>102</a>
<a class='no' id='n103' name='n103' href='#n103'>103</a>
<a class='no' id='n104' name='n104' href='#n104'>104</a>
<a class='no' id='n105' name='n105' href='#n105'>105</a>
<a class='no' id='n106' name='n106' href='#n106'>106</a>
<a class='no' id='n107' name='n107' href='#n107'>107</a>
<a class='no' id='n108' name='n108' href='#n108'>108</a>
<a class='no' id='n109' name='n109' href='#n109'>109</a>
<a class='no' id='n110' name='n110' href='#n110'>110</a>
<a class='no' id='n111' name='n111' href='#n111'>111</a>
<a class='no' id='n112' name='n112' href='#n112'>112</a>
<a class='no' id='n113' name='n113' href='#n113'>113</a>
<a class='no' id='n114' name='n114' href='#n114'>114</a>
<a class='no' id='n115' name='n115' href='#n115'>115</a>
<a class='no' id='n116' name='n116' href='#n116'>116</a>
<a class='no' id='n117' name='n117' href='#n117'>117</a>
<a class='no' id='n118' name='n118' href='#n118'>118</a>
<a class='no' id='n119' name='n119' href='#n119'>119</a>
<a class='no' id='n120' name='n120' href='#n120'>120</a>
<a class='no' id='n121' name='n121' href='#n121'>121</a>
<a class='no' id='n122' name='n122' href='#n122'>122</a>
<a class='no' id='n123' name='n123' href='#n123'>123</a>
<a class='no' id='n124' name='n124' href='#n124'>124</a>
<a class='no' id='n125' name='n125' href='#n125'>125</a>
<a class='no' id='n126' name='n126' href='#n126'>126</a>
<a class='no' id='n127' name='n127' href='#n127'>127</a>
<a class='no' id='n128' name='n128' href='#n128'>128</a>
<a class='no' id='n129' name='n129' href='#n129'>129</a>
<a class='no' id='n130' name='n130' href='#n130'>130</a>
<a class='no' id='n131' name='n131' href='#n131'>131</a>
<a class='no' id='n132' name='n132' href='#n132'>132</a>
<a class='no' id='n133' name='n133' href='#n133'>133</a>
<a class='no' id='n134' name='n134' href='#n134'>134</a>
<a class='no' id='n135' name='n135' href='#n135'>135</a>
<a class='no' id='n136' name='n136' href='#n136'>136</a>
<a class='no' id='n137' name='n137' href='#n137'>137</a>
<a class='no' id='n138' name='n138' href='#n138'>138</a>
<a class='no' id='n139' name='n139' href='#n139'>139</a>
<a class='no' id='n140' name='n140' href='#n140'>140</a>
<a class='no' id='n141' name='n141' href='#n141'>141</a>
<a class='no' id='n142' name='n142' href='#n142'>142</a>
<a class='no' id='n143' name='n143' href='#n143'>143</a>
<a class='no' id='n144' name='n144' href='#n144'>144</a>
<a class='no' id='n145' name='n145' href='#n145'>145</a>
<a class='no' id='n146' name='n146' href='#n146'>146</a>
<a class='no' id='n147' name='n147' href='#n147'>147</a>
<a class='no' id='n148' name='n148' href='#n148'>148</a>
<a class='no' id='n149' name='n149' href='#n149'>149</a>
<a class='no' id='n150' name='n150' href='#n150'>150</a>
<a class='no' id='n151' name='n151' href='#n151'>151</a>
<a class='no' id='n152' name='n152' href='#n152'>152</a>
<a class='no' id='n153' name='n153' href='#n153'>153</a>
<a class='no' id='n154' name='n154' href='#n154'>154</a>
<a class='no' id='n155' name='n155' href='#n155'>155</a>
<a class='no' id='n156' name='n156' href='#n156'>156</a>
<a class='no' id='n157' name='n157' href='#n157'>157</a>
<a class='no' id='n158' name='n158' href='#n158'>158</a>
<a class='no' id='n159' name='n159' href='#n159'>159</a>
<a class='no' id='n160' name='n160' href='#n160'>160</a>
<a class='no' id='n161' name='n161' href='#n161'>161</a>
<a class='no' id='n162' name='n162' href='#n162'>162</a>
<a class='no' id='n163' name='n163' href='#n163'>163</a>
<a class='no' id='n164' name='n164' href='#n164'>164</a>
<a class='no' id='n165' name='n165' href='#n165'>165</a>
<a class='no' id='n166' name='n166' href='#n166'>166</a>
<a class='no' id='n167' name='n167' href='#n167'>167</a>
<a class='no' id='n168' name='n168' href='#n168'>168</a>
<a class='no' id='n169' name='n169' href='#n169'>169</a>
<a class='no' id='n170' name='n170' href='#n170'>170</a>
<a class='no' id='n171' name='n171' href='#n171'>171</a>
<a class='no' id='n172' name='n172' href='#n172'>172</a>
<a class='no' id='n173' name='n173' href='#n173'>173</a>
<a class='no' id='n174' name='n174' href='#n174'>174</a>
<a class='no' id='n175' name='n175' href='#n175'>175</a>
<a class='no' id='n176' name='n176' href='#n176'>176</a>
<a class='no' id='n177' name='n177' href='#n177'>177</a>
<a class='no' id='n178' name='n178' href='#n178'>178</a>
<a class='no' id='n179' name='n179' href='#n179'>179</a>
<a class='no' id='n180' name='n180' href='#n180'>180</a>
<a class='no' id='n181' name='n181' href='#n181'>181</a>
<a class='no' id='n182' name='n182' href='#n182'>182</a>
<a class='no' id='n183' name='n183' href='#n183'>183</a>
<a class='no' id='n184' name='n184' href='#n184'>184</a>
<a class='no' id='n185' name='n185' href='#n185'>185</a>
<a class='no' id='n186' name='n186' href='#n186'>186</a>
<a class='no' id='n187' name='n187' href='#n187'>187</a>
</pre></td>
<td class='lines'><pre><code>/*
 * Cairo SDL clock. Shows how to use Cairo with SDL.
 * Made by Writser Cleveringa, based upon code from Eric Windisch.
 * Minor code clean up by Chris Nystrom (5/21/06) and converted to cairo-sdl
 * by Chris Wilson and converted to cairosdl by M Joonas Pihlaja.
 */

#include &lt;stdio.h&gt;
#include &lt;time.h&gt;
#include &lt;math.h&gt;
#include "cairosdl.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Draws a clock on a normalized Cairo context */
static void
draw (cairo_t *cr)
{
    time_t t;
    struct tm *tm;
    double seconds, minutes, hours;

    /* In newer versions of Visual Studio localtime(..) is deprecated. */
    /* Use localtime_s instead. See MSDN. */
    t = time (NULL);
    tm = localtime (&amp;t);

    /* compute the angles for the indicators of our clock */
    seconds = tm-&gt;tm_sec * M_PI / 30;
    minutes = tm-&gt;tm_min * M_PI / 30;
    hours = tm-&gt;tm_hour * M_PI / 6;

    /* Fill the background with white. */
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_paint (cr);

    /* who doesn't want all those nice line settings :) */
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width (cr, 0.1);

    /* translate to the center of the rendering context and draw a black */
    /* clock outline */
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_translate (cr, 0.5, 0.5);
    cairo_arc (cr, 0, 0, 0.4, 0, M_PI * 2);
    cairo_stroke (cr);

    /* draw a white dot on the current second. */
    cairo_set_source_rgba (cr, 1, 1, 1, 0.6);
    cairo_arc (cr,
	       sin (seconds) * 0.4, -cos (seconds) * 0.4,
	       0.05, 0, M_PI * 2);
    cairo_fill (cr);

    /* draw the minutes indicator */
    cairo_set_source_rgba (cr, 0.2, 0.2, 1, 0.6);
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, sin (minutes) * 0.4, -cos (minutes) * 0.4);
    cairo_stroke (cr);

    /* draw the hours indicator      */
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, sin (hours) * 0.2, -cos (hours) * 0.2);
    cairo_stroke (cr);
}

/* Shows how to draw with Cairo on SDL surfaces */
static void
draw_screen (SDL_Surface *screen)
{
    cairo_t *cr;
    cairo_status_t status;

    /* Create a cairo drawing context, normalize it and draw a clock. */
    SDL_LockSurface (screen); {
        cr = cairosdl_create (screen);

        cairo_scale (cr, screen-&gt;w, screen-&gt;h);
        draw (cr);

        status = cairo_status (cr);
        cairosdl_destroy (cr);
    }
    SDL_UnlockSurface (screen);
    SDL_Flip (screen);

    /* Nasty nasty error handling. */
    if (status != CAIRO_STATUS_SUCCESS) {
        fprintf (stderr, "Unable to create or draw with a cairo context "
                 "for the screen: %s\n",
                 cairo_status_to_string (status));
        exit (1);
    }
}

static SDL_Surface *
init_screen (int width, int height, int bpp)
{
    SDL_Surface *screen;

    /* Initialize SDL */
    if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) &lt; 0) {
	fprintf (stderr, "Unable to initialize SDL: %s\n",
		 SDL_GetError ());
	exit (1);
    }

    /* Open a screen with the specified properties */
    screen = SDL_SetVideoMode (width, height, bpp,
			       SDL_HWSURFACE |
			       SDL_RESIZABLE);
    if (screen == NULL) {
	fprintf (stderr, "Unable to set %ix%i video: %s\n",
		 width, height, SDL_GetError ());
	exit (1);
    }

    SDL_WM_SetCaption ("Cairo clock - Press Q to quit", "ICON");

    return screen;
}

/* This function pushes a custom event onto the SDL event queue.
 * Whenever the main loop receives it, the window will be redrawn.
 * We can't redraw the window here, since this function could be called
 * from another thread.
 */
static Uint32
timer_cb (Uint32 interval, void *param)
{
    SDL_Event event;

    event.type = SDL_USEREVENT;
    SDL_PushEvent (&amp;event);

    (void)param;
    return interval;
}

int
main (int argc, char **argv)
{
    SDL_Surface *screen;
    SDL_Event event;

    (void)argc;
    (void)argv;

    /* Initialize SDL, open a screen */
    screen = init_screen (640, 480, 32);

    /* Create a timer which will redraw the screen every 100 ms. */
    SDL_AddTimer (100, timer_cb, NULL);

    while (SDL_WaitEvent (&amp;event)) {
	switch (event.type) {
	case SDL_KEYDOWN:
	    if (event.key.keysym.sym == SDLK_q) {
		goto done;
	    }
	    break;

	case SDL_QUIT:
	    goto done;

	case SDL_VIDEORESIZE:
	    screen = SDL_SetVideoMode (event.resize.w,
				       event.resize.h, 32,
				       SDL_HWSURFACE |
				       SDL_RESIZABLE);
	    /* fall-through */
	case SDL_USEREVENT:
	    draw_screen (screen);
	    break;

	default:
	    break;
	}
    }

done:
    SDL_FreeSurface (screen);
    SDL_Quit ();
    return 0;
}
</code></pre></td></tr></table>
</div><div class='footer'>generated  by cgit v0.8.2.1 at 2009-07-03 15:44:07 (GMT)</div>
</body>
</html>
