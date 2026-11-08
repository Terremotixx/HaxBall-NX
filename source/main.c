#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>
#include <SDL.h>
#include "font_atlas.h"
#include "controller_icons.h"
extern const int16_t kick_pcm[],click_pcm[],start_pcm[],menu_pcm[],match_pcm[],win_pcm[],goal_pcm[],beep3_pcm[],beep2_pcm[],beep1_pcm[]; extern const int16_t *kick_pcm_end,*click_pcm_end,*start_pcm_end,*menu_pcm_end,*match_pcm_end,*win_pcm_end,*goal_pcm_end,*beep3_pcm_end,*beep2_pcm_end,*beep1_pcm_end;
#define PCM_LEN(a,b) ((int)((b)-(a)))
#include "countdown_atlas.h"

#define SW 1280
#define GOAL_DEPTH 48
#define SH 720
#define FX ((SW-field_width())/2)
#define FY ((SH-field_height())/2)
#define FW field_width()
#define FH field_height()
#define PR player_radius()
#define BR ball_radius()
#define MAX_TEAM 4
#define EXTERNAL_PADS 8
#define MAX_PADS 9
#define TAP_WINDOW .20f
#define CONTROL_WINDOW .25f
#define MAX_PLAYERS 8
#define CW 64
#define CH 84
#define TEAM_BLUE_R 0
#define TEAM_BLUE_G 102
#define TEAM_BLUE_B 255
#define TEAM_RED_R 244
#define TEAM_RED_G 40
#define TEAM_RED_B 64

typedef enum { MENU, OPTIONS, TEAM_SELECT, ALL_READY, MATCH_CONFIG, MATCH, RESULTS } Screen;
typedef enum { COUNTDOWN, PLAYING, GOLDEN } MatchState;
typedef enum { CLASSIC, FUTSAL, HOCKEY, HANDBALL, BASH } Arena;
typedef struct { float x,y,vx,vy,aim_x,aim_y; Uint8 r,g,b; bool blue,human; int slot,team_index; float lost,press_time,press_flash; bool aiming,shot_lock,controlled; } Player;
typedef struct { float x,y,vx,vy; } Ball;
typedef struct { int duration,size,bh,rh,difficulty,arena,volume,contrast,effects,names,vibration,music; char blue[16],red[16]; } Settings;

static PadState pads[MAX_PADS];
static Player ps[MAX_PLAYERS];
static Ball ball;
static Settings cfg;
typedef struct {int kicks,contacts,posts,own_goals;float possession;} TeamStats;
static TeamStats team_stats[2];
static bool last_goal_blue=false,goal_finishes=false;
static float goal_effect=0,goal_net_target_x=0,goal_net_direction=1;static bool goal_flight_active=false,goal_net_released=false;
static float countdown_length=3;
static SDL_Renderer* ui_renderer=NULL;
static SDL_Texture* pitch_cache=NULL;
static void keyboard_backdrop(void);
static int field_width(void){return (cfg.arena==FUTSAL?1000:(cfg.arena==HANDBALL?1000:1040))+(cfg.size-1)*32;}
static int field_height(void){return (cfg.arena==FUTSAL?480:(cfg.arena==HANDBALL?480:496))+(cfg.size-1)*16;}
static float player_radius(void){return (cfg.arena==FUTSAL?25.0f:(cfg.arena==HANDBALL?23.0f:22.0f))-(cfg.size-1)*1.5f;}
static float ball_radius(void){return (cfg.arena==HOCKEY?10.5f:(cfg.arena==FUTSAL?14.0f:13.0f))-(cfg.size-1)*.8f;}
static float goal_line(bool right){float inset=cfg.arena==HOCKEY?FW*.13f:0.0f;return right?FX+FW-inset:FX+inset;}
static float ball_previous_x=0;
static Screen page=MENU;
static MatchState state=COUNTDOWN;
static SDL_Texture *font;
static SDL_Texture *countdown_texture;
static float contact_wait[MAX_PLAYERS];
static SDL_Texture *controller_textures[4];
static int glyph_left[224],glyph_width[224],glyph_top[224],glyph_height[224];
static float kick_flash=0;
static int ball_owner=-1;
static SDL_AudioDeviceID audio;
static bool running=true,paused=false,auto_pause=false,sd_ready=false;
static int menu_row=0,option_row=0,config_row=0,players_n=0,blue_n=0,red_n=0;
static int blue_score=0,red_score=0,match_seconds=300,shots=0,touches=0;
static float elapsed=0,countdown=0,stuck=0,stuck_x=0,stuck_y=0;
static u32 vote_mask=0;
static const int minutes[]={1,2,3};
static const char *arenas[]={"Fútbol","Fútbol sala","Hockey","Balonmano","Cuatro porterías"};
static const char *levels[]={"Fácil","Normal","Difícil"};

static int side[MAX_PADS]={0},confirmed[MAX_PADS]={0},order_counter=0;
static char human_names[MAX_PADS][16];
static float ai_wait[8],ai_tx[8],ai_ty[8],kick_wait[8],kick_grace[8];
typedef enum { CHASE, COVER, SUPPORT, RUN } BotRole;
static BotRole roles[8];
static float tactics_wait=0;
static void stop_rumble(void){}
static bool raise_native_applet_audio(float*main_volume,float*library_volume);
static void restore_native_applet_audio(bool restore,float main_volume,float library_volume);
static void pump_audio(void);
static SDL_atomic_t native_applet_audio_running;
static SDL_Thread*native_applet_audio_thread=NULL;
static int native_applet_audio_worker(void*unused){(void)unused;while(SDL_AtomicGet(&native_applet_audio_running)){pump_audio();SDL_Delay(8);}return 0;}
static int scorer=-1;
static int last_touch=-1,pause_row=0;static bool restart_confirm=false,paused_options=false;
static int captain(int team){int best=-1;for(int i=0;i<MAX_PADS;i++)if(side[i]==team&&confirmed[i]&&padIsConnected(&pads[i])&&(best<0||confirmed[i]<confirmed[best]))best=i;return best;}
static int human_count(int team){int n=0;for(int i=0;i<MAX_PADS;i++)if(side[i]==team&&padIsConnected(&pads[i]))n++;return n;}
static void save_people(void){if(!sd_ready)return;FILE*f=fopen("sdmc:/switch/HaxBall-NX/players.cfg","w");if(!f)return;for(int i=0;i<MAX_PADS;i++)fprintf(f,"%d\n%s\n",side[i],human_names[i]);fclose(f);}
static void edit_person(int slot){keyboard_backdrop();SwkbdConfig k;char out[16]={0};if(R_FAILED(swkbdCreate(&k,0)))return;swkbdConfigMakePresetDefault(&k);swkbdConfigSetInitialText(&k,human_names[slot]);swkbdConfigSetStringLenMax(&k,15);float previous_main_volume=1.0f,previous_library_volume=1.0f;bool restore_applet_audio=raise_native_applet_audio(&previous_main_volume,&previous_library_volume);Result keyboard_result=swkbdShow(&k,out,sizeof(out));restore_native_applet_audio(restore_applet_audio,previous_main_volume,previous_library_volume);if(R_SUCCEEDED(keyboard_result)&&out[0]){snprintf(human_names[slot],16,"%s",out);save_people();}swkbdClose(&k);}
static void reset_lobby_ready(void){for(int i=0;i<MAX_PADS;i++)confirmed[i]=0;}
static void restart_round(void);
static bool can_pause(int slot){if(last_touch<0)return false;bool blue=last_touch==1;if(side[slot]!=(blue?1:-1))return false;for(int i=0;i<players_n;i++)if(ps[i].blue==blue){float x=ps[i].x-ball.x,y=ps[i].y-ball.y;if(x*x+y*y<90*90)return true;}return false;}
static float clampf(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
static char controller_status[100]="";
static bool raise_native_applet_audio(float*main_volume,float*library_volume){*main_volume=1.0f;*library_volume=1.0f;bool restore=R_SUCCEEDED(appletGetExpectedMasterVolume(main_volume,library_volume));if(restore)appletSetExpectedMasterVolume(1.0f,1.0f);SDL_AtomicSet(&native_applet_audio_running,1);native_applet_audio_thread=SDL_CreateThread(native_applet_audio_worker,"native-applet-audio",NULL);if(!native_applet_audio_thread)SDL_AtomicSet(&native_applet_audio_running,0);return restore;}
static void restore_native_applet_audio(bool restore,float main_volume,float library_volume){if(native_applet_audio_thread){SDL_AtomicSet(&native_applet_audio_running,0);SDL_WaitThread(native_applet_audio_thread,NULL);native_applet_audio_thread=NULL;}if(restore)appletSetExpectedMasterVolume(main_volume,library_volume);}
static u64 input_held[MAX_PADS],input_down[MAX_PADS],menu_input[MAX_PADS],nav_axis[MAX_PADS];
static u32 input_style[MAX_PADS];
static float nav_repeat[MAX_PADS];
static bool input_connected[MAX_PADS];
static bool console_lite=false;
static void detect_console(void){
    if(R_SUCCEEDED(setsysInitialize())){SetSysProductModel model=SetSysProductModel_Invalid;if(R_SUCCEEDED(setsysGetProductModel(&model)))console_lite=model==SetSysProductModel_Hoag;setsysExit();}
}
static void load_controller_icons(SDL_Renderer*r){
    const unsigned char*masks[]={controller_left_alpha,controller_right_alpha,controller_pro_alpha,controller_handheld_alpha};
    for(int i=0;i<4;i++){
        SDL_Surface*surface=SDL_CreateRGBSurfaceWithFormat(0,100,100,32,SDL_PIXELFORMAT_RGBA32);if(!surface)continue;
        for(int y=0;y<100;y++)for(int x=0;x<100;x++){
            Uint32*row=(Uint32*)((Uint8*)surface->pixels+y*surface->pitch);row[x]=SDL_MapRGBA(surface->format,255,255,255,masks[i][y*100+x]);
        }
        controller_textures[i]=SDL_CreateTextureFromSurface(r,surface);SDL_FreeSurface(surface);
        if(controller_textures[i])SDL_SetTextureBlendMode(controller_textures[i],SDL_BLENDMODE_BLEND);
    }
}
static void controller_icon(SDL_Renderer*r,int slot,int x,int y,int size){
    u32 style=padGetStyleSet(&pads[slot]);bool paired=(style&HidNpadStyleTag_NpadJoyDual)!=0;
    int type=slot==EXTERNAL_PADS?3:((style&HidNpadStyleTag_NpadJoyLeft)?0:((style&HidNpadStyleTag_NpadJoyRight)?1:2));
    for(int part=0;part<(paired?2:1);part++){
        SDL_Texture*t=controller_textures[paired?part:type];if(!t)continue;
        int team=side[slot];SDL_SetTextureColorMod(t,team<0?TEAM_RED_R:(team>0?TEAM_BLUE_R:245),team<0?TEAM_RED_G:(team>0?TEAM_BLUE_G:248),team<0?TEAM_RED_B:(team>0?TEAM_BLUE_B:252));
        SDL_Rect dst={x+(paired?(part?size/5:-size/5):0),y,size,size};
        double angle=paired?0:(type==0?-90:(type==1?90:0));SDL_RenderCopyEx(r,t,NULL,&dst,angle,NULL,SDL_FLIP_NONE);
    }
}
static int joy_type(int slot){
    u32 style=padGetStyleSet(&pads[slot]);
    if(style&HidNpadStyleTag_NpadJoyLeft)return 1;
    if(style&HidNpadStyleTag_NpadJoyRight)return 2;
    return 0;
}
static u64 map_buttons(u64 raw,int type){
    if(!type)return raw;
    u64 mapped=0;
    if(type==1){
        if(raw&HidNpadButton_Down)mapped|=HidNpadButton_A;
        if(raw&HidNpadButton_Left)mapped|=HidNpadButton_B;
        if(raw&HidNpadButton_Right)mapped|=HidNpadButton_X;
        if(raw&HidNpadButton_Up)mapped|=HidNpadButton_Y;
    }else{
        if(raw&HidNpadButton_X)mapped|=HidNpadButton_A;
        if(raw&HidNpadButton_A)mapped|=HidNpadButton_B;
        if(raw&HidNpadButton_Y)mapped|=HidNpadButton_X;
        if(raw&HidNpadButton_B)mapped|=HidNpadButton_Y;
    }
    if(raw&(HidNpadButton_Plus|HidNpadButton_Minus))mapped|=HidNpadButton_Plus;
    if(raw&HidNpadButton_AnySL)mapped|=HidNpadButton_Minus;
    if(raw&HidNpadButton_AnySR)mapped|=HidNpadButton_R;
    return mapped;
}
static void read_stick(int slot,float*x,float*y){
    int type=joy_type(slot);HidAnalogStickState a=padGetStickPos(&pads[slot],type==2?1:0);
    float rx=(float)a.x/32767,ry=(float)a.y/32767;
    if(type==1){*x=-ry;*y=-rx;}else if(type==2){*x=ry;*y=rx;}else{*x=rx;*y=-ry;}
}
static void update_input(float dt){
    for(int i=0;i<MAX_PADS;i++){
        bool connected=padIsConnected(&pads[i]);u32 style=padGetStyleSet(&pads[i]);
        u64 held=connected?map_buttons(padGetButtons(&pads[i]),joy_type(i)):0;
        bool changed=connected!=input_connected[i]||style!=input_style[i];
        input_down[i]=changed?0:held&~input_held[i];input_held[i]=held;
        input_connected[i]=connected;input_style[i]=style;
        if(changed){confirmed[i]=0;nav_axis[i]=0;nav_repeat[i]=.30f;}
        float x=0,y=0;if(connected)read_stick(i,&x,&y);
        u64 axis=held&(HidNpadButton_Left|HidNpadButton_Right|HidNpadButton_Up|HidNpadButton_Down);
        if(fabsf(x)>.55f||fabsf(y)>.55f){
            if(fabsf(x)>fabsf(y))axis|=x<0?HidNpadButton_Left:HidNpadButton_Right;
            else axis|=y<0?HidNpadButton_Up:HidNpadButton_Down;
        }
        menu_input[i]=input_down[i]&~(HidNpadButton_Left|HidNpadButton_Right|HidNpadButton_Up|HidNpadButton_Down);
        if(axis!=nav_axis[i]){nav_axis[i]=axis;nav_repeat[i]=.32f;if(!changed)menu_input[i]|=axis;}
        else if(axis){nav_repeat[i]-=dt;if(nav_repeat[i]<=0){menu_input[i]|=axis;nav_repeat[i]=.13f;}}
    }
}
static u64 down(int i){return input_down[i];}
static u64 navigation(int i){return menu_input[i];}

static void connect_controllers(void){
    padConfigureInput(EXTERNAL_PADS,HidNpadStyleSet_NpadStandard);HidLaControllerSupportArg arg;hidLaCreateControllerSupportArg(&arg);
    arg.hdr.player_count_min=1;arg.hdr.player_count_max=8;arg.hdr.enable_permit_joy_dual=1;arg.hdr.enable_left_justify=0;
    stop_rumble();hidSetNpadJoyHoldType(HidNpadJoyHoldType_Vertical);
    appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleepNotify);
    float previous_main_volume=1.0f,previous_library_volume=1.0f;bool restore_applet_audio=raise_native_applet_audio(&previous_main_volume,&previous_library_volume);
    Result result=hidLaShowControllerSupportForSystem(NULL,&arg,false);
    if(R_FAILED(result))result=hidLaShowControllerSupport(NULL,&arg);
    restore_native_applet_audio(restore_applet_audio,previous_main_volume,previous_library_volume);
    appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);hidSetNpadJoyHoldType(HidNpadJoyHoldType_Horizontal);
    if(R_FAILED(result)){snprintf(controller_status,sizeof(controller_status),"Conecta desde HOME o une L + R aquí.");return;}
    controller_status[0]=0;reset_lobby_ready();memset(input_connected,0,sizeof(input_connected));
    for(int i=0;i<MAX_PADS;i++){padUpdate(&pads[i]);side[i]=0;}
}
static bool pair_in_lobby(void){
    int left=-1,right=-1;
    for(int i=0;i<EXTERNAL_PADS;i++)if(padIsConnected(&pads[i])){
        u32 style=padGetStyleSet(&pads[i]);u64 b=padGetButtons(&pads[i]);
        if((style&HidNpadStyleTag_NpadJoyLeft)&&(b&HidNpadButton_L))left=i;
        if((style&HidNpadStyleTag_NpadJoyRight)&&(b&HidNpadButton_R))right=i;
    }
    if(left<0||right<0)return false;
    hidSetNpadJoyAssignmentModeDual((HidNpadIdType)left);hidSetNpadJoyAssignmentModeDual((HidNpadIdType)right);
    Result rc=hidMergeSingleJoyAsDualJoy((HidNpadIdType)left,(HidNpadIdType)right);
    if(R_FAILED(rc)){snprintf(controller_status,sizeof(controller_status),"No se pudo unir la pareja. Reintenta desde Mandos.");return false;}
    reset_lobby_ready();side[left]=side[right]=0;memset(input_connected,0,sizeof(input_connected));controller_status[0]=0;return true;
}
static SDL_Texture* disk_masks[96];
static SDL_Texture* ring_masks[96][5];
static SDL_Texture* round_mask(SDL_Renderer*r,int radius,int thickness){
    int size=radius*2+1;SDL_Surface*surface=SDL_CreateRGBSurfaceWithFormat(0,size,size,32,SDL_PIXELFORMAT_RGBA32);if(!surface)return NULL;
    for(int y=0;y<size;y++)for(int x=0;x<size;x++){
        float d=hypotf(x-radius,y-radius),coverage=clampf(radius+.25f-d,0,1);
        if(thickness>0)coverage*=clampf(d-(radius-thickness)+.25f,0,1);
        Uint32*pixel=(Uint32*)((Uint8*)surface->pixels+y*surface->pitch+x*4);*pixel=SDL_MapRGBA(surface->format,255,255,255,(Uint8)(coverage*255));
    }
    SDL_Texture*t=SDL_CreateTextureFromSurface(r,surface);SDL_FreeSurface(surface);if(t)SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);return t;
}
static bool draw_round_mask(SDL_Renderer*r,int x,int y,int radius,int thickness){
    if(r!=ui_renderer||radius<1||radius>=96||thickness>4)return false;
    SDL_Texture**entry=thickness?&ring_masks[radius][thickness]:&disk_masks[radius];if(!*entry)*entry=round_mask(r,radius,thickness);if(!*entry)return false;
    Uint8 cr,cg,cb,ca;SDL_GetRenderDrawColor(r,&cr,&cg,&cb,&ca);SDL_SetTextureColorMod(*entry,cr,cg,cb);SDL_SetTextureAlphaMod(*entry,ca);SDL_Rect dst={x-radius,y-radius,radius*2+1,radius*2+1};SDL_RenderCopy(r,*entry,NULL,&dst);return true;
}
static void circle(SDL_Renderer*r,int x,int y,int rad){
    if(draw_round_mask(r,x,y,rad,0))return;
    // Pixel-centred spans avoid the isolated cardinal pixels of an integer-radius disc.
    for(int dy=-rad;dy<=rad;dy++){float rr=rad-.25f,inside=rr*rr-dy*dy;if(inside<0)continue;int w=(int)sqrtf(inside);SDL_RenderDrawLine(r,x-w,y+dy,x+w,y+dy);}
}
static void warm_round_masks(SDL_Renderer*r){
    for(int radius=2;radius<=48;radius++){
        disk_masks[radius]=round_mask(r,radius,0);
        for(int t=1;t<=4;t++)ring_masks[radius][t]=round_mask(r,radius,t);
    }
    disk_masks[64]=round_mask(r,64,0);ring_masks[64][4]=round_mask(r,64,4);
}
static int next_glyph(const unsigned char**cursor){
    unsigned int c=*(*cursor)++;if(c<128)return (int)c;
    if((c==0xc2||c==0xc3)&&(**cursor&0xc0)==0x80){unsigned int d=*(*cursor)++;return (int)(((c&31)<<6)|(d&63));}
    while((**cursor&0xc0)==0x80)(*cursor)++;
    return '?';
}
static bool english_ui=false;
static const char*tr(const char*s){
    if(!english_ui||!s)return s;
    struct Pair{const char*es,*en;};
    static const struct Pair words[]={
      {"LISTO","READY"},{"VOLVER","BACK"},{"CAPITÁN","CAPTAIN"},{"ROJO","RED"},{"VERDE","GREEN"},{"AZUL","BLUE"},{"AMARILLO","YELLOW"},{"MANDO DESCONECTADO","CONTROLLER DISCONNECTED"},{"Chutar","Shoot"},{"+ / -  Salir","+ / -  Exit"},{"HAXBALL NX","HAXBALL NX"},{"Fútbol local para 1 a 8 jugadores","Local football for 1 to 8 players"},{"Partido local","Local match"},{"De 1 vs 1 hasta 4 vs 4","From 1v1 up to 4v4"},{"Las plazas libres se completan con bots.","Open spots are filled by bots."},{"Jugar","Play"},{"Opciones","Options"},{"Equipos y ajustes del partido","Teams and match settings"},{"Sonido, nombres y vibración","Sound, names and vibration"},
      {"Ajustes guardados automáticamente","Settings saved automatically"},{"Volumen general","Master volume"},{"Efectos de sonido","Sound effects"},{"Nombres de jugadores","Player names"},{"Música de fondo","Background music"},{"Activado","On"},{"Desactivado","Off"},{"Volver","Back"},{"Stick / cruceta: seleccionar y cambiar","Stick / D-pad: navigate and change"},
      {"Elegir lado","Choose a side"},{"Elige equipo. Los mandos del centro no participan.","Choose a team. Center players are spectators."},{"Sin equipo","Unassigned"},{"No participa","Spectator"},{"Listo","Ready"},{"Sin confirmar","Not ready"},{"Continuar","Continue"},{"Tu nombre","Your name"},{"Equipo","Team"},{"Mandos","Controllers"},{"L+R / SL+SR: emparejar","L+R / SL+SR: pair controllers"},
      {"Configurar partido","Match setup"},{"Estadio, duración, dificultad y tamaño de equipo","Arena, time, difficulty and team size"},{"Estadio","Arena"},{"Duración","Time"},{"Dificultad","Difficulty"},{"Equipos","Teams"},{"Empezar partido","Start match"},{"Empezar","Start"},{"Fácil","Easy"},{"Normal","Normal"},{"Difícil","Hard"},{"Fútbol","Football"},{"Fútbol sala","Futsal"},{"Hockey","Hockey"},{"Balonmano","Handball"},{"Cuatro porterías","Four goals"},
      {"Todos listos","Everyone ready"},{"Edita los nombres o continúa a los ajustes","Edit names or continue to settings"},{"Solo el capitán cambia el equipo. R / SR: equipo contrario","Only the captain can change teams. R / SR: opposing team"},{"No necesita confirmar","No confirmation needed"},{"Plazas libres: bots","Open spots: bots"},{"Cancelar","Cancel"},{"Cambiar","Change"},{"Atrás","Back"},{"Elegir","Select"},{"Confirmar","Confirm"},{"Sustituir","Replace"},
      {"PAUSA","PAUSED"},{"REANUDAR","RESUME"},{"REINICIAR","RESTART"},{"SALIR A EQUIPOS","BACK TO TEAMS"},{"REINICIAR EL PARTIDO?","RESTART MATCH?"},{"Resultado","Results"},{"Resultado del partido","Match results"},{"Estadísticas por equipo","Team statistics"},{"Tiros","Shots"},{"Toques","Touches"},{"Postes","Posts"},{"En propia puerta","Own goals"},{"Posesión","Possession"},{"Revancha","Rematch"},{"GOL DE ORO","GOLDEN GOAL"},{"TIEMPO","TIME"},{"JUGADOR","PLAYER"},{"GF","GF"},{"GC","GA"},{"PUNTOS","POINTS"},{"CPU","CPU"},{"No se pudo unir la pareja. Reintenta desde Mandos.","Could not pair controllers. Try again from Controllers."},{"Conecta desde HOME o une L + R aquí.","Connect from HOME or pair with L + R here."},
      {"Amplia los equipos: faltan plazas para los jugadores","Increase team size to fit all players"},{"Capitán","Captain"},{"GANA %s","%s WINS"},{"EMPATE: %d JUGADORES","%d PLAYER DRAW"},{"GOL","GOAL"},{"¡GOL!","GOAL!"},{"Gol de oro","Golden goal"},{"REVANCHA EN %d","REMATCH IN %d"},{"Resultado","Results"},{"P%d","P%d"},{"4 jugadores","4 players"},{"Fútbol sala","Futsal"},{"Activado","On"},{"Desactivado","Off"},{"No participa","Spectator"},{"Sin confirmar","Not ready"},{"Listo","Ready"},{"Sin equipo","Unassigned"},{"No se pudo unir la pareja. Reintenta desde Mandos.","Could not pair controllers. Try again from Controllers."},{"Conecta desde HOME o une L + R aquí.","Connect from HOME or pair with L + R here."},{"Blue team name","Blue team name"},{"Red team name","Red team name"},{"Rojo","Red"},{"Azul","Blue"},{"Verde","Green"},{"Amarillo","Yellow"}
    };
    for(unsigned i=0;i<sizeof(words)/sizeof(words[0]);i++)if(strcmp(s,words[i].es)==0)return words[i].en;
    return s;
}static bool control_text=false;
static int text_width(const char*s,int adv){
    s=tr(s);
    if(!control_text)adv=(adv*110+50)/100;
    int w=0;const unsigned char*p=(const unsigned char*)s;while(*p){int c=next_glyph(&p);if(c>=32&&c<=255)w+=(glyph_width[c-32]+8)*adv/CW;}return w;
}
static void text(SDL_Renderer*r,const char*s,int x,int y,int adv,int h,Uint8 cr,Uint8 cg,Uint8 cb){
    s=tr(s);
    if(!font||!s)return;
    if(!control_text){adv=(adv*110+50)/100;h=(h*110+50)/100;}
    SDL_SetTextureColorMod(font,cr,cg,cb);const unsigned char*p=(const unsigned char*)s;
    while(*p){int c=next_glyph(&p);if(c<32||c>255)continue;int n=c-32,w=glyph_width[n];
        SDL_Rect a={(n%16)*CW+glyph_left[n],(n/16)*CH,w,CH},b={x,y,w*adv/CW,h};if(c!=' ')SDL_RenderCopy(r,font,&a,&b);x+=(w+8)*adv/CW;
    }
}
static void ink_text(SDL_Renderer*r,const char*s,int cx,int cy,int adv,int h,Uint8 cr,Uint8 cg,Uint8 cb){
    s=tr(s);
    int top=CH,bottom=0;const unsigned char*p=(const unsigned char*)s;while(*p){int n=next_glyph(&p)-32;if(n<0||n>=224)continue;if(glyph_top[n]<top)top=glyph_top[n];if(glyph_top[n]+glyph_height[n]>bottom)bottom=glyph_top[n]+glyph_height[n];}
    int realh=(h*110+50)/100;int trailing=8*((adv*110+50)/100)/CW;
    text(r,s,cx-(text_width(s,adv)-trailing)/2,cy-(top+bottom)*realh/CH/2,adv,h,cr,cg,cb);
}
static void score_text(SDL_Renderer*r,const char*s,int cx,int cy){
    int adv=36,h=45,w=0,top=CH,bottom=0;
    for(const char*p=s;*p;p++){int n=*p-32;w+=glyph_width[n]*adv/CW+4;if(glyph_top[n]<top)top=glyph_top[n];if(glyph_top[n]+glyph_height[n]>bottom)bottom=glyph_top[n]+glyph_height[n];}w-=4;
    int x=cx-w/2,y=cy-(bottom-top)*h/CH/2;
    SDL_SetTextureColorMod(font,255,255,255);
    for(const char*p=s;*p;p++){int n=*p-32;SDL_Rect a={(n%16)*CW+glyph_left[n],(n/16)*CH+top,glyph_width[n],bottom-top},b={x,y,glyph_width[n]*adv/CW,(bottom-top)*h/CH};SDL_RenderCopy(r,font,&a,&b);x+=b.w+4;}
}
static void centered(SDL_Renderer*r,const char*s,int y,int adv,int h,Uint8 cr,Uint8 cg,Uint8 cb){
    s=tr(s);text(r,s,(SW-text_width(s,adv))/2,y,adv,h,cr,cg,cb);}
static SDL_Texture *load_font(SDL_Renderer*r){
    SDL_RWops*rw=SDL_RWFromConstMem(FONT_ATLAS_BMP,(int)sizeof(FONT_ATLAS_BMP));if(!rw)return NULL;
    SDL_Surface*raw=SDL_LoadBMP_RW(rw,1);if(!raw)return NULL;
    SDL_Surface*s=SDL_ConvertSurfaceFormat(raw,SDL_PIXELFORMAT_ARGB8888,0);SDL_FreeSurface(raw);if(!s)return NULL;
    for(int n=0;n<224;n++){
        int lo=CW,hi=-1,top=CH,bottom=-1;
        for(int y=0;y<CH;y++)for(int x=0;x<CW;x++){
            Uint32 pixel=*(Uint32*)((Uint8*)s->pixels+((n/16)*CH+y)*s->pitch+((n%16)*CW+x)*4);
            if((pixel&0xff)>48){if(x<lo)lo=x;if(x>hi)hi=x;if(y<top)top=y;if(y>bottom)bottom=y;}
        }
        glyph_top[n]=bottom>=top?top:0;glyph_height[n]=bottom>=top?bottom-top+1:CH;glyph_left[n]=hi>=lo?lo:0;glyph_width[n]=hi>=lo?hi-lo+1:24;
    }
    for(int yy=0;yy<s->h;yy++)for(int xx=0;xx<s->w;xx++){Uint32*p=(Uint32*)((Uint8*)s->pixels+yy*s->pitch+xx*4);Uint8 alpha=*p&255;*p=SDL_MapRGBA(s->format,255,255,255,alpha);}
    SDL_Texture*t=SDL_CreateTextureFromSurface(r,s);SDL_FreeSurface(s);
    if(t)SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
    return t;
}
static void box(SDL_Renderer*r,int x,int y,int w,int h,Uint8 red,Uint8 green,Uint8 blue){
    SDL_Rect b={x,y,w,h};SDL_SetRenderDrawColor(r,red,green,blue,255);SDL_RenderFillRect(r,&b);
}
// Four face buttons in horizontal Joy-Con orientation: left, top, right, bottom.
static void button_hint(SDL_Renderer*r,int x,int y,char letter,const char*action){
    if(!action||!action[0])return;
    control_text=true;char label[2]={letter,0};text(r,label,x-text_width(label,22)/2,y-13,22,28,244,248,252);
    text(r,"/",x+19,y-14,18,28,132,151,173);
    const int ox[]={-13,0,13,0},oy[]={0,-13,0,13};
    int selected=letter=='A'?2:(letter=='B'?3:(letter=='X'?1:0));
    // Native-resolution solid shapes, separated by clear gaps: no tiny nested outlines or texture scaling.
    for(int i=0;i<4;i++){
        int bx=x+57+ox[i],by=y+oy[i];
        if(i==selected)SDL_SetRenderDrawColor(r,TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B,255);
        else SDL_SetRenderDrawColor(r,233,241,250,255);
        circle(r,bx,by,7);
    }
    control_text=false;text(r,action,x+94,y-13,22,28,228,238,249);
}
static void control_legend(SDL_Renderer*r){
    const char*a="Elegir",*b="",*x="",*y="";
    if(page==OPTIONS){a="Cambiar";b="Volver";}
    else if(page==TEAM_SELECT){a="Listo";b="Atrás";y="Mandos";}
    else if(page==ALL_READY){a="Continuar";b="Volver";x="Tu nombre";y="Equipo";}
    else if(page==MATCH_CONFIG){a="Empezar";b="Volver";}
    else if(page==RESULTS){a="Revancha";b="Equipos";}
    else if(page==MATCH){a=paused?(restart_confirm?"Confirmar":"Elegir"):"Chutar";if(paused)b=restart_confirm?"Cancelar":"Volver";if(paused&&auto_pause)x="Sustituir";}
    button_hint(r,76,683,'A',a);button_hint(r,376,683,'B',b);
    button_hint(r,676,683,'X',x);button_hint(r,976,683,'Y',y);if(page==MENU)text(r,"+ / -  Salir",1020,670,22,28,244,248,252);
}
static void panel(SDL_Renderer*r,const char*title,const char*sub){
    SDL_SetRenderDrawColor(r,12,17,27,255);SDL_RenderClear(r);
    box(r,0,0,SW,6,85,219,191);box(r,56,46,5,78,85,219,191);
    text(r,title,80,35,page==MENU?44:36,page==MENU?52:44,page==MENU?85:244,page==MENU?219:247,page==MENU?191:252);text(r,sub,82,99,24,28,146,162,182);
    box(r,56,145,1168,1,43,54,72);box(r,56,650,1168,1,43,54,72);
    control_legend(r);
}
static void row(SDL_Renderer*r,const char*s,int y,bool selected){
    box(r,300,y-9,680,51,selected?34:21,selected?59:29,selected?68:42);
    if(selected){box(r,300,y-9,4,51,85,219,191);if(!((page==OPTIONS&&option_row<4)||(page==MATCH_CONFIG&&config_row<4)))text(r,">",944,y,26,32,85,219,191);}
    text(r,s,324,y,27,34,selected?247:186,selected?250:198,selected?253:214);
}
static float goal_h(void){return (cfg.arena==FUTSAL?58.0f:(cfg.arena==HOCKEY?58.0f:(cfg.arena==HANDBALL?64.0f:87.0f)))+(cfg.size-1)*(cfg.arena==CLASSIC?8.0f:5.0f);}
static float friction(void){return cfg.arena==HOCKEY?.991f:(cfg.arena==FUTSAL?.983f:(cfg.arena==HANDBALL?.984f:.985f));}

static void save_cfg(void){if(!sd_ready)return;(void)mkdir("sdmc:/switch",0777);(void)mkdir("sdmc:/switch/HaxBall-NX",0777);FILE*f=fopen("sdmc:/switch/HaxBall-NX/settings.cfg","w");if(!f)return;fprintf(f,"%d %d %d %d %d %d %d %d\n%s\n%s\n",cfg.duration,cfg.size,cfg.bh,cfg.rh,cfg.difficulty,cfg.arena,cfg.volume,0,cfg.blue,cfg.red);fprintf(f,"%d %d %d %d\n",cfg.effects,cfg.names,cfg.vibration,cfg.music);fclose(f);}
static void trim(char*s){size_t n=strlen(s);while(n&&(s[n-1]=='\n'||s[n-1]=='\r'))s[--n]=0;}
static void read_saved_name(FILE*f,char*out){
    char line[128];if(fgets(line,sizeof(line),f)){trim(line);size_t n=strlen(line);if(n>15)n=15;memcpy(out,line,n);out[n]=0;}
}
static void load_cfg(void){cfg.duration=2;cfg.size=1;cfg.bh=1;cfg.rh=0;cfg.difficulty=1;cfg.arena=CLASSIC;cfg.volume=65;cfg.effects=cfg.names=cfg.music=1;cfg.vibration=0;strcpy(cfg.blue,"BLUE");strcpy(cfg.red,"RED");sd_ready=R_SUCCEEDED(fsdevMountSdmc());if(!sd_ready)return;FILE*f=fopen("sdmc:/switch/HaxBall-NX/settings.cfg","r");if(!f)return;int v[8];if(fscanf(f,"%d %d %d %d %d %d %d %d\n",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&v[6],&v[7])==8){cfg.duration=(int)clampf(v[0],0,2);cfg.size=(int)clampf(v[1],1,4);cfg.bh=(int)clampf(v[2],1,cfg.size);cfg.rh=(int)clampf(v[3],0,cfg.size);cfg.difficulty=(int)clampf(v[4],0,2);cfg.arena=(int)clampf(v[5],0,4);cfg.volume=(int)clampf(v[6],0,100);cfg.contrast=0;read_saved_name(f,cfg.blue);read_saved_name(f,cfg.red);if(!cfg.blue[0])strcpy(cfg.blue,"BLUE");if(!cfg.red[0])strcpy(cfg.red,"RED");int e,n,vib;if(fscanf(f,"%d %d %d",&e,&n,&vib)==3){cfg.effects=e!=0;cfg.names=n!=0;cfg.vibration=0;int music;if(fscanf(f,"%d",&music)==1)cfg.music=music!=0;}}fclose(f);}
static void name_entry(bool is_blue){keyboard_backdrop();SwkbdConfig k;char out[16]={0};if(R_FAILED(swkbdCreate(&k,0)))return;swkbdConfigMakePresetDefault(&k);swkbdConfigSetHeaderText(&k,english_ui?(is_blue?"Blue team name":"Red team name"):(is_blue?"Nombre del equipo azul":"Nombre del equipo rojo"));swkbdConfigSetInitialText(&k,is_blue?cfg.blue:cfg.red);swkbdConfigSetStringLenMax(&k,15);float previous_main_volume=1.0f,previous_library_volume=1.0f;bool restore_applet_audio=raise_native_applet_audio(&previous_main_volume,&previous_library_volume);Result keyboard_result=swkbdShow(&k,out,sizeof(out));restore_native_applet_audio(restore_applet_audio,previous_main_volume,previous_library_volume);if(R_SUCCEEDED(keyboard_result)&&out[0]){char clean[16]={0};int n=0;for(int i=0;out[i]&&n<15;i++)if((unsigned char)out[i]>=32&&(unsigned char)out[i]<=126)clean[n++]=out[i];if(n){strcpy(is_blue?cfg.blue:cfg.red,clean);save_cfg();}}swkbdClose(&k);}
static const int16_t *effect_data=NULL,*event_data=NULL;
static int effect_pos=0,effect_len=0,event_pos=0,event_len=0,effect_kind=0;
static u64 music_sample=0;static Uint32 last_click_tick=0;static bool click_started=false;static int countdown_beep_stage=-1;
static void tone(float hz,int ms){
    (void)ms;if(!cfg.effects)return;
    int kind=hz<300?1:(hz>=700?3:2);
    // Debounce repeated menu navigation while the supplied click is playing.
    if(kind==2){Uint32 now=SDL_GetTicks();if(click_started&&now-last_click_tick<220)return;click_started=true;last_click_tick=now;}
    effect_kind=kind;
    if(kind==1){effect_data=kick_pcm;effect_len=PCM_LEN(kick_pcm,kick_pcm_end);}
    else{effect_data=click_pcm;effect_len=PCM_LEN(click_pcm,click_pcm_end);}
    effect_pos=0;
}
static void countdown_beep(int stage){
    if(!cfg.effects)return;
    if(stage==0){effect_data=beep3_pcm;effect_len=PCM_LEN(beep3_pcm,beep3_pcm_end);}
    else if(stage==1){effect_data=beep2_pcm;effect_len=PCM_LEN(beep2_pcm,beep2_pcm_end);}
    else{effect_data=beep1_pcm;effect_len=PCM_LEN(beep1_pcm,beep1_pcm_end);}
    effect_kind=4+stage;effect_pos=0;
}
static void queue_audio_event(bool victory){
    event_data=victory?win_pcm:goal_pcm;
    event_len=victory?PCM_LEN(win_pcm,win_pcm_end):PCM_LEN(goal_pcm,goal_pcm_end);
    event_pos=0;
}
static void pump_audio(void){
    static Screen previous_page=MENU;static int previous_goals=0;static bool match_track=false;
    if(!audio)return;
    bool in_match=page==MATCH;int goals=blue_score+red_score;
    if(in_match&&state==COUNTDOWN){int stage=countdown_length<=1?2:(countdown<1?0:(countdown<2?1:2));if(stage!=countdown_beep_stage){countdown_beep_stage=stage;countdown_beep(stage);}}else countdown_beep_stage=-1;
    if(in_match!=match_track){music_sample=0;match_track=in_match;SDL_ClearQueuedAudio(audio);}
    if(page==MATCH&&goals>previous_goals)queue_audio_event(false);
    if(page==RESULTS&&previous_page!=RESULTS)queue_audio_event(true);
    if(page==MATCH&&previous_page!=MATCH&&goals==0)event_len=0;
    previous_page=page;previous_goals=goals;
    if(cfg.volume<=0){SDL_ClearQueuedAudio(audio);event_len=effect_len=effect_kind=0;return;}
    Uint32 queued=SDL_GetQueuedAudioSize(audio);if(queued>4096)return;
    int blocks=(6144-(int)queued+2047)/2048;if(blocks>2)blocks=2;
    const int16_t*track=match_track?match_pcm:menu_pcm;size_t track_len=match_track?PCM_LEN(match_pcm,match_pcm_end):PCM_LEN(menu_pcm,menu_pcm_end);float base_gain=(match_track?.43f:.324f)*(paused?.45f:1.0f),master_gain=cfg.volume*.022464f;
    while(blocks-->0){
        int16_t samples[1024];
        for(int i=0;i<1024;i++,music_sample++){
            float value=0;
            bool event_active=event_pos<event_len;
            if(cfg.music){
                float music_gain=base_gain;

                if(event_active)music_gain=.055f;
                if(music_sample>=track_len){music_sample=0;}
                value=track[music_sample]*music_gain;
            }
            if(event_active){value+=event_data[event_pos]*(event_data==win_pcm?2.45f:.95f);event_pos++;}
            if(effect_pos<effect_len){
                if(cfg.effects){float gain;if(effect_kind==1)gain=.25f;else if(effect_kind==2||effect_kind==3)gain=1.82f;else if(effect_kind==4)gain=.184f;else if(effect_kind==5)gain=.229f;else gain=.124f;value+=effect_data[effect_pos]*gain;}
                effect_pos++;if(effect_pos>=effect_len)effect_kind=0;
            }
            samples[i]=(int16_t)clampf(value*master_gain,-32767,32767);
        }
        SDL_QueueAudio(audio,samples,sizeof(samples));
    }
}
static void draw_menu(SDL_Renderer*r){
    panel(r,"HAXBALL NX","Fútbol local para 1 a 8 jugadores");
    text(r,"Partido local",82,209,35,44,244,247,252);
    text(r,"De 1 vs 1 hasta 4 vs 4",82,270,25,32,146,162,182);
    text(r,"Las plazas libres se completan con bots.",84,321,23,28,146,162,182);
    const char* labels[]={"Jugar","Opciones"};const char* sub[]={"Equipos y ajustes del partido","Sonido, nombres y vibración"};
    for(int i=0;i<2;i++){
        int y=403+i*91;bool active=menu_row==i;box(r,82,y,490,76,active?34:21,active?59:29,active?68:42);
        if(active)box(r,82,y,4,76,85,219,191);
        text(r,labels[i],105,y+7,31,36,244,247,252);text(r,sub[i],106,y+43,22,28,146,162,182);
        if(active)text(r,">",535,y+22,27,32,85,219,191);
    }
    box(r,680,205,514,374,19,30,43);SDL_SetRenderDrawColor(r,60,82,98,255);
    SDL_RenderDrawRect(r,&(SDL_Rect){710,235,454,314});SDL_RenderDrawLine(r,937,235,937,549);
    SDL_RenderDrawRect(r,&(SDL_Rect){710,325,65,130});SDL_RenderDrawRect(r,&(SDL_Rect){1099,325,65,130});
    SDL_SetRenderDrawColor(r,TEAM_RED_R,TEAM_RED_G,TEAM_RED_B,255);circle(r,815,355,25);circle(r,860,465,25);
    SDL_SetRenderDrawColor(r,TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B,255);circle(r,1060,315,25);circle(r,1022,425,25);
    SDL_SetRenderDrawColor(r,245,248,252,255);circle(r,930,390,13);
    }
static void draw_options(SDL_Renderer*r){
    panel(r,"Opciones","Ajustes guardados automáticamente");
    const char*labels[]={"Volumen general","Efectos de sonido","Nombres de jugadores","Música de fondo"};
    int values[]={cfg.volume>0,cfg.effects,cfg.names,cfg.music};
    for(int i=0;i<4;i++){
        int y=215+i*70;row(r,labels[i],y,option_row==i);
        box(r,755,y-1,185,38,values[i]?40:50,values[i]?102:59,values[i]?94:72);
        const char*value=values[i]?"Activado":"Desactivado";
        text(r,value,847-text_width(value,19)/2,y+3,19,26,245,249,252);
        if(option_row==i){text(r,"<",760,y+1,22,29,255,255,255);text(r,">",921,y+1,22,29,255,255,255);}
    }row(r,"Volver",519,option_row==4);centered(r,"Stick / cruceta: seleccionar y cambiar",605,22,26,190,204,220);
}
static void draw_team_select(SDL_Renderer*r){
    panel(r,"Elegir lado","Elige equipo. Los mandos del centro no participan.");
    const char*names[]={cfg.red,"Sin equipo",cfg.blue};int rows[3]={0},center_count=0;
    for(int i=0;i<MAX_PADS;i++)if(!side[i]&&padIsConnected(&pads[i]))center_count++;
    for(int c=0;c<3;c++){
        int x=64+c*393;box(r,x,180,366,407,21,29,42);
        box(r,x,180,366,4,c==0?TEAM_RED_R:(c==2?TEAM_BLUE_R:99),c==0?TEAM_RED_G:(c==2?TEAM_BLUE_G:115),c==0?TEAM_RED_B:(c==2?TEAM_BLUE_B:135));
        text(r,names[c],x+22,200,30,38,244,247,252);
        text(r,c==1?"No necesita confirmar":"Plazas libres: bots",x+22,245,20,25,146,162,182);
    }
    for(int i=0;i<MAX_PADS;i++)if(padIsConnected(&pads[i])){
        int c=side[i]+1,n=rows[c]++,x=78+c*393,y=283+n*72;char label[48];
        if(c==1&&center_count>4){
            x+=n%2*172;y=283+(n/2)*59;box(r,x,y,166,55,29,40,55);controller_icon(r,i,x+2,y+3,48);
            snprintf(label,sizeof(label),"P%d",i+1);text(r,label,x+56+(106-text_width(label,24))/2,y+2,24,28,244,248,252);
            snprintf(label,sizeof(label),"%.9s",human_names[i]);text(r,label,x+56+(106-text_width(label,18))/2,y+29,18,23,222,232,244);
        }else{
            box(r,x,y,338,66,29,40,55);controller_icon(r,i,x+5,y+1,64);
            snprintf(label,sizeof(label),"P%d  %.15s",i+1,human_names[i]);int adv=25;while(text_width(label,adv)>248&&adv>18)adv--;text(r,label,x+78+(248-text_width(label,adv))/2,y+3,adv,31,244,248,252);
            const char*status=confirmed[i]?"LISTO":(side[i]?"Sin confirmar":"No participa");text(r,status,x+78+(248-text_width(status,19))/2,y+36,19,24,confirmed[i]?85:146,confirmed[i]?219:162,confirmed[i]?191:182);
        }
    }
    if(controller_status[0])centered(r,controller_status,584,18,22,255,210,120);
    centered(r,"L+R / SL+SR: emparejar",608,22,26,190,204,220);
}
static u32 requested_ready_mask(void);
static void draw_all_ready(SDL_Renderer*r){
    panel(r,"Todos listos","Edita los nombres o continúa a los ajustes");
    int rows[2]={0};text(r,cfg.red,344-text_width(cfg.red,32)/2,174,32,39,TEAM_RED_R,TEAM_RED_G,TEAM_RED_B);text(r,cfg.blue,932-text_width(cfg.blue,32)/2,174,32,39,TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B);
    for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])){
        int team=side[i]>0?1:0,x=84+team*588,y=226+rows[team]++*68;char line[64];
        box(r,x,y,520,61,21,29,42);box(r,x,y,4,61,team?TEAM_BLUE_R:TEAM_RED_R,team?TEAM_BLUE_G:TEAM_RED_G,team?TEAM_BLUE_B:TEAM_RED_B);
        controller_icon(r,i,x+10,y,60);snprintf(line,sizeof(line),"P%d  %.15s",i+1,human_names[i]);int adv=28;while(text_width(line,adv)>414&&adv>18)adv--;text(r,line,x+80+(424-text_width(line,adv))/2,y+2,adv,35,244,247,252);
        if(captain(side[i])==i)text(r,"CAPITÁN",x+80+(424-text_width("CAPITÁN",19))/2,y+37,19,23,180,195,213);
    }
    row(r,"Continuar",550,true);
    centered(r,"Solo el capitán cambia el equipo. R / SR: equipo contrario",611,24,29,190,204,220);
}
static void keyboard_backdrop(void){
    if(!ui_renderer)return;
    draw_all_ready(ui_renderer);
    SDL_Surface*full=SDL_CreateRGBSurfaceWithFormat(0,SW,SH,32,SDL_PIXELFORMAT_RGBA32);
    SDL_Surface*small=SDL_CreateRGBSurfaceWithFormat(0,80,45,32,SDL_PIXELFORMAT_RGBA32);
    if(full&&small&&SDL_RenderReadPixels(ui_renderer,NULL,SDL_PIXELFORMAT_RGBA32,full->pixels,full->pitch)==0){
        SDL_BlitScaled(full,NULL,small,NULL);SDL_Texture*blur=SDL_CreateTextureFromSurface(ui_renderer,small);
        if(blur){SDL_SetTextureScaleMode(blur,SDL_ScaleModeLinear);SDL_RenderCopy(ui_renderer,blur,NULL,NULL);SDL_DestroyTexture(blur);}
    }
    if(full)SDL_FreeSurface(full);
    if(small)SDL_FreeSurface(small);
    SDL_SetRenderDrawBlendMode(ui_renderer,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(ui_renderer,8,14,24,135);SDL_RenderFillRect(ui_renderer,NULL);SDL_SetRenderDrawBlendMode(ui_renderer,SDL_BLENDMODE_NONE);SDL_RenderPresent(ui_renderer);
}
static void setting_row(SDL_Renderer*r,const char*label,const char*value,int y,bool selected){
    row(r,label,y,selected);char v[80];snprintf(v,sizeof(v),selected?"< %s >":"%s",tr(value));text(r,v,935-text_width(v,25),y,25,33,85,219,191);
}
static void draw_match_config(SDL_Renderer*r){
    char s[40];panel(r,"Configurar partido","Estadio, duración, dificultad y tamaño de equipo");
    setting_row(r,"Estadio",arenas[cfg.arena],210,config_row==0);
    snprintf(s,sizeof(s),english_ui?"%d min":"%d min",minutes[cfg.duration]);setting_row(r,"Duración",s,281,config_row==1);
    setting_row(r,"Dificultad",levels[cfg.difficulty],352,config_row==2);
    if(cfg.arena==BASH)snprintf(s,sizeof(s),"4 jugadores");else snprintf(s,sizeof(s),"%d vs %d",cfg.size,cfg.size);setting_row(r,"Equipos",s,423,config_row==3);
    row(r,"Empezar partido",512,config_row==4);
    centered(r,cfg.size<human_count(1)||cfg.size<human_count(-1)?"Amplia los equipos: faltan plazas para los jugadores":"Stick / cruceta: seleccionar y cambiar",604,22,26,146,162,182);
}
static u32 requested_ready_mask(void){u32 m=0;for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i]))m|=1u<<i;return m;}


static void draw_goal_net(SDL_Renderer*r,SDL_Rect g,bool left){
    box(r,g.x,g.y,g.w,g.h,cfg.arena==HOCKEY?170:21,cfg.arena==HOCKEY?190:39,cfg.arena==HOCKEY?201:42);
    SDL_SetRenderDrawColor(r,114,137,141,255);
    for(int x=g.x+6;x<g.x+g.w;x+=7)SDL_RenderDrawLine(r,x,g.y,x,g.y+g.h);
    for(int y=g.y+8;y<g.y+g.h;y+=10)SDL_RenderDrawLine(r,g.x,y,g.x+g.w,y);
    SDL_SetRenderDrawColor(r,255,cfg.arena==HOCKEY?65:255,cfg.arena==HOCKEY?65:255,255);
    for(int t=0;t<(cfg.arena==HOCKEY?5:3);t++){
        SDL_RenderDrawLine(r,g.x,g.y+t,g.x+g.w,g.y+t);
        SDL_RenderDrawLine(r,g.x,g.y+g.h-t,g.x+g.w,g.y+g.h-t);
        int back=left?g.x+t:g.x+g.w-t;SDL_RenderDrawLine(r,back,g.y,back,g.y+g.h);
    }
    if(cfg.arena==HOCKEY){
        SDL_SetRenderDrawColor(r,225,237,241,255);for(int yy=g.y+7;yy<g.y+g.h-6;yy+=10)SDL_RenderDrawLine(r,g.x+5,yy,g.x+g.w-5,yy-5);
        int back=left?g.x+2:g.x+g.w-2;
        SDL_SetRenderDrawColor(r,237,49,64,255);circle(r,back,g.y+2,4);circle(r,back,g.y+g.h-2,4);
        SDL_SetRenderDrawColor(r,255,158,153,255);SDL_RenderDrawLine(r,g.x+4,g.y+1,g.x+g.w-4,g.y+1);SDL_RenderDrawLine(r,back-1,g.y+5,back-1,g.y+g.h-5);
    }else if(cfg.arena==HANDBALL){
        int front=left?g.x+g.w:g.x;for(int yy=g.y;yy<g.y+g.h;yy+=12)box(r,front-3,yy,6,(int)fminf(12,g.y+g.h-yy),((yy-g.y)/12)%2?255:227,((yy-g.y)/12)%2?255:49,((yy-g.y)/12)%2?255:59);
    }

}
static void arc(SDL_Renderer*r,float x,float y,float rx,float ry,float from,float to,bool dashed){
    int count=(int)(fabsf(to-from)*30)+1;
    for(int i=0;i<count;i++){
        if(dashed&&(i/3)%2)continue;
        float a=from+(to-from)*i/count,b=from+(to-from)*(i+1)/count;
        for(int stroke=0;stroke<2;stroke++)SDL_RenderDrawLine(r,(int)(x+cosf(a)*(rx-stroke)),(int)(y+sinf(a)*(ry-stroke)),(int)(x+cosf(b)*(rx-stroke)),(int)(y+sinf(b)*(ry-stroke)));
    }
}
static void crowd(SDL_Renderer*r){
    // Walkway, barriers and three stepped rows; spectators never occupy the touchline runoff.
    box(r,FX-22,FY-22,FW+44,FH+44,42,53,58);
    for(int row=0;row<3;row++){
        int top=FY-39-row*16,bottom=FY+FH+30+row*16;
        box(r,FX-72,top-7,FW+144,15,25+row*7,33+row*7,43+row*7);
        box(r,FX-72,bottom-7,FW+144,15,25+row*7,33+row*7,43+row*7);
        box(r,FX-39-row*16,FY,15,FH,25+row*7,33+row*7,43+row*7);
        box(r,FX+FW+25+row*16,FY,15,FH,25+row*7,33+row*7,43+row*7);
        for(int x=FX-60;x<FX+FW+61;x+=18){
            if(((x-FX+60)/18)%12==0)continue;
            int k=((x-FX+60)/18+row*5)%9;bool blue=(x>SW/2);Uint8 cr=k<6?(blue?TEAM_BLUE_R:TEAM_RED_R):180,cg=k<6?(blue?TEAM_BLUE_G:TEAM_RED_G):173,cb=k<6?(blue?TEAM_BLUE_B:TEAM_RED_B):145;
            SDL_SetRenderDrawColor(r,cr,cg,cb,255);circle(r,x,top+2,5);circle(r,x,bottom+2,5);
            SDL_SetRenderDrawColor(r,k%3?220:157,k%3?175:114,k%3?136:84,255);circle(r,x,top-3,3);circle(r,x,bottom-3,3);
        }
        for(int y=FY+14;y<FY+FH;y+=18){
            if(abs(y-SH/2)<(int)goal_h()+48)continue;
            SDL_SetRenderDrawColor(r,TEAM_RED_R,TEAM_RED_G,TEAM_RED_B,255);circle(r,FX-32-row*16,y,5);
            SDL_SetRenderDrawColor(r,TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B,255);circle(r,FX+FW+32+row*16,y,5);
            SDL_SetRenderDrawColor(r,224,180,142,255);circle(r,FX-32-row*16,y-4,3);circle(r,FX+FW+32+row*16,y-4,3);
        }
    }
    SDL_SetRenderDrawColor(r,139,157,164,255);SDL_RenderDrawRect(r,&(SDL_Rect){FX-23,FY-23,FW+46,FH+46});
    
}
static void draw_bash_pitch(SDL_Renderer*r);
static void draw_bash_game(SDL_Renderer*r);
static void bash_init(void);
static void bash_update(float dt);
static void bash_results(SDL_Renderer*r);
static void draw_pitch(SDL_Renderer*r){
    if(cfg.arena==BASH){draw_bash_pitch(r);return;}
    SDL_SetRenderDrawColor(r,12,24,30,255);SDL_RenderClear(r);crowd(r);
    int red=cfg.arena==HOCKEY?255:(cfg.arena==HANDBALL?192:(cfg.arena==FUTSAL?15:23));
    int green=cfg.arena==HOCKEY?255:(cfg.arena==HANDBALL?137:(cfg.arena==FUTSAL?157:93));
    int blue=cfg.arena==HOCKEY?255:(cfg.arena==HANDBALL?96:(cfg.arena==FUTSAL?183:57));
    box(r,FX,FY,FW,FH,red,green,blue);
    if(cfg.arena==CLASSIC)for(int i=0;i<10;i+=2)box(r,FX+i*FW/10,FY,FW/10,FH,28,106,63);
    float cy=SH*.5f,gh=goal_h(),l=goal_line(false),rr=goal_line(true);
    if(cfg.arena!=HOCKEY){SDL_SetRenderDrawColor(r,245,250,248,255);SDL_RenderDrawRect(r,&(SDL_Rect){FX,FY,FW,FH});SDL_RenderDrawLine(r,SW/2,FY,SW/2,FY+FH);}
    if(cfg.arena==CLASSIC){
        arc(r,SW*.5f,cy,FH*.17f,FH*.17f,0,6.2831853f,false);
        int depth=FW/7,half=(int)(FH*.38f),small=FW/17;
        SDL_RenderDrawRect(r,&(SDL_Rect){FX,(int)cy-half,depth,2*half});SDL_RenderDrawRect(r,&(SDL_Rect){FX+FW-depth,(int)cy-half,depth,2*half});
        SDL_RenderDrawRect(r,&(SDL_Rect){FX,(int)(cy-gh-17),small,(int)(2*gh+34)});SDL_RenderDrawRect(r,&(SDL_Rect){FX+FW-small,(int)(cy-gh-17),small,(int)(2*gh+34)});
        arc(r,FX+depth,cy,FH*.075f,FH*.15f,-1.5707963f,1.5707963f,false);arc(r,FX+FW-depth,cy,FH*.075f,FH*.15f,1.5707963f,4.712389f,false);
        circle(r,FX+depth*2/3,(int)cy,3);circle(r,FX+FW-depth*2/3,(int)cy,3);    }else if(cfg.arena==FUTSAL){
        arc(r,SW*.5f,cy,FH*.14f,FH*.14f,0,6.2831853f,false);
        arc(r,FX,cy,FW*.15f,FH*.32f,-1.5707963f,1.5707963f,false);arc(r,FX+FW,cy,FW*.15f,FH*.32f,1.5707963f,4.712389f,false);
        circle(r,FX+FW*12/100,(int)cy,4);circle(r,FX+FW-FW*12/100,(int)cy,4);circle(r,FX+FW*20/100,(int)cy,4);circle(r,FX+FW-FW*20/100,(int)cy,4);
        arc(r,FX,FY,18,18,0,1.5707963f,false);arc(r,FX+FW,FY,18,18,1.5707963f,3.1415926f,false);
        arc(r,FX,FY+FH,18,18,-1.5707963f,0,false);arc(r,FX+FW,FY+FH,18,18,3.1415926f,4.712389f,false);
    }else if(cfg.arena==HANDBALL){
        for(int y=0;y<FH;y+=24)for(int x=0;x<FW;x+=32)if((x/32+y/24)%3==0)box(r,FX+x,FY+y,(int)fminf(32,FW-x),(int)fminf(24,FH-y),195,141,100);
        for(int side=0;side<2;side++){
            float gx=FX+(side?FW:0),core=FH*.13f,ry=FH*.19f,rx=FW*.14f;
            SDL_SetRenderDrawColor(r,57,115,177,255);
            for(int yy=(int)(SH*.5f-core-ry);yy<=SH*.5f+core+ry;yy++){float dy=fmaxf(0,fabsf(yy-SH*.5f)-core)/ry;int width=(int)(rx*sqrtf(fmaxf(0,1-dy*dy)));SDL_RenderDrawLine(r,(int)gx,yy,(int)(gx+(side?-width:width)),yy);}
            SDL_SetRenderDrawColor(r,255,255,239,255);
            float start=side?1.5707963f:-1.5707963f;
            arc(r,gx,SH*.5f-core,rx,ry,side?3.1415926f:start,side?4.712389f:0,false);arc(r,gx,SH*.5f+core,rx,ry,side?1.5707963f:0,side?3.1415926f:1.5707963f,false);
            SDL_RenderDrawLine(r,(int)(gx+(side?-rx:rx)),(int)(SH*.5f-core),(int)(gx+(side?-rx:rx)),(int)(SH*.5f+core));
            arc(r,gx,SH*.5f,FW*.22f,FH*.48f,start,start+3.1415926f,true);for(int mark=0;mark<2;mark++){int dist=mark?FW*18/100:FW*10/100,mx=(int)(gx+(side?-1:1)*dist);SDL_RenderDrawLine(r,mx,(int)(SH*.5f-(mark?7:4)),mx,(int)(SH*.5f+(mark?7:4)));SDL_RenderDrawLine(r,mx+1,(int)(SH*.5f-(mark?7:4)),mx+1,(int)(SH*.5f+(mark?7:4)));}
        }
        SDL_SetRenderDrawColor(r,255,255,239,255);SDL_RenderDrawRect(r,&(SDL_Rect){FX,FY,FW,FH});SDL_RenderDrawLine(r,SW/2,FY,SW/2,FY+FH);circle(r,SW/2,SH/2,14);SDL_SetRenderDrawColor(r,57,115,177,255);circle(r,SW/2,SH/2,11);
    }else{
        const int rinkR=(int)(FH*.24f);const float pi=3.14159265358979323846f;
        const int bx=12,by=125,br=172,hred=235,hgreen=45,hblue=48;
        SDL_SetRenderDrawColor(r,12,24,30,255);
        for(int yy=0;yy<rinkR;yy++){
            int cut=rinkR-(int)sqrtf((float)(rinkR*rinkR-(rinkR-yy)*(rinkR-yy)));
            SDL_RenderDrawLine(r,FX,FY+yy,FX+cut,FY+yy);SDL_RenderDrawLine(r,FX+FW-cut,FY+yy,FX+FW,FY+yy);
            SDL_RenderDrawLine(r,FX,FY+FH-1-yy,FX+cut,FY+FH-1-yy);SDL_RenderDrawLine(r,FX+FW-cut,FY+FH-1-yy,FX+FW,FY+FH-1-yy);
        }
        SDL_SetRenderDrawColor(r,bx,by,br,255);
        SDL_RenderDrawLine(r,FX+rinkR,FY,FX+FW-rinkR,FY);SDL_RenderDrawLine(r,FX+rinkR,FY+FH,FX+FW-rinkR,FY+FH);
        SDL_RenderDrawLine(r,FX,FY+rinkR,FX,FY+FH-rinkR);SDL_RenderDrawLine(r,FX+FW,FY+rinkR,FX+FW,FY+FH-rinkR);
        arc(r,FX+rinkR,FY+rinkR,rinkR,rinkR,pi,pi*1.5f,false);arc(r,FX+FW-rinkR,FY+rinkR,rinkR,rinkR,pi*1.5f,pi*2,false);
        arc(r,FX+FW-rinkR,FY+FH-rinkR,rinkR,rinkR,0,pi*.5f,false);arc(r,FX+rinkR,FY+FH-rinkR,rinkR,rinkR,pi*.5f,pi,false);
        SDL_RenderDrawLine(r,FX+(int)(FW*.365f),FY,FX+(int)(FW*.365f),FY+FH);SDL_RenderDrawLine(r,FX+(int)(FW*.635f),FY,FX+(int)(FW*.635f),FY+FH);
        arc(r,SW*.5f,cy,FH*.142f,FH*.142f,0,2*pi,false);circle(r,SW/2,(int)cy,3);
        SDL_SetRenderDrawColor(r,hred,hgreen,hblue,255);
        SDL_RenderDrawLine(r,SW/2,FY,SW/2,FY+FH);SDL_RenderDrawLine(r,(int)l,FY,(int)l,FY+FH);SDL_RenderDrawLine(r,(int)rr,FY,(int)rr,FY+FH);
        for(int side=0;side<2;side++)for(int lane=0;lane<2;lane++){
            float x=side?(rr+FX+FW*.635f)*.5f:(l+FX+FW*.365f)*.5f,y=FY+FH*(lane?.75f:.25f);
                        arc(r,x,y,FH*.135f,FH*.135f,0,2*pi,false);circle(r,(int)x,(int)y,4);
            float m=FH/274.0f;
            for(int sign=-1;sign<=1;sign+=2){
                SDL_RenderDrawLine(r,x+sign*7*m,y-10*m,x+sign*7*m,y-3*m);SDL_RenderDrawLine(r,x+sign*7*m,y+3*m,x+sign*7*m,y+10*m);
                SDL_RenderDrawLine(r,x-12*m,y+sign*3*m,x-3*m,y+sign*3*m);SDL_RenderDrawLine(r,x+3*m,y+sign*3*m,x+12*m,y+sign*3*m);
            }
            SDL_RenderDrawLine(r,(int)x-5,(int)y-FH*.158f,(int)x-5,(int)y-FH*.135f);SDL_RenderDrawLine(r,(int)x+5,(int)y-FH*.158f,(int)x+5,(int)y-FH*.135f);
            SDL_RenderDrawLine(r,(int)x-5,(int)y+FH*.135f,(int)x-5,(int)y+FH*.158f);SDL_RenderDrawLine(r,(int)x+5,(int)y+FH*.135f,(int)x+5,(int)y+FH*.158f);
        }
        for(int side=0;side<2;side++)for(int lane=0;lane<2;lane++){float x=FX+FW*(side?.613f:.387f),y=FY+FH*(lane?.75f:.25f);circle(r,(int)x,(int)y,3);}
        for(int side=0;side<2;side++){
            float gx=side?rr:l, radius=gh+12;
            SDL_SetRenderDrawColor(r,0,169,224,255);
            for(int yy=-(int)radius;yy<=(int)radius;yy++){float width=sqrtf(fmaxf(0,radius*radius-yy*yy));SDL_RenderDrawLine(r,(int)gx,(int)cy+yy,(int)(gx+(side?-width:width)),(int)cy+yy);}
            SDL_SetRenderDrawColor(r,hred,hgreen,hblue,255);arc(r,gx,cy,radius,radius,side?pi*.5f:-pi*.5f,side?pi*1.5f:pi*.5f,false);
        }
    }
    if(cfg.arena==HOCKEY){SDL_SetRenderDrawColor(r,40,123,198,255);circle(r,SW/2,SH/2,3);}else{SDL_SetRenderDrawColor(r,250,250,245,255);circle(r,SW/2,SH/2,3);}
    SDL_Rect gl={(int)l-GOAL_DEPTH,(int)(cy-gh),GOAL_DEPTH,(int)(2*gh)},gr={(int)rr,(int)(cy-gh),GOAL_DEPTH,(int)(2*gh)};draw_goal_net(r,gl,true);draw_goal_net(r,gr,false);
}
static SDL_Texture*crowd_poses[2][2];
static void fan_pose(SDL_Renderer*r,int x,int y,bool blue,bool happy,int phase,int shade){
    box(r,x-8,y-10,17,20,25+shade*7,33+shade*7,43+shade*7);
    int jump=happy?(phase?4:1):0;
    SDL_SetRenderDrawColor(r,blue?TEAM_BLUE_R:TEAM_RED_R,blue?TEAM_BLUE_G:TEAM_RED_G,blue?TEAM_BLUE_B:TEAM_RED_B,255);circle(r,x,y+2-jump,5);
    SDL_SetRenderDrawColor(r,230,185,145,255);circle(r,x,y-3-jump,3);
    int ay=happy?-7:(phase?1:5);SDL_RenderDrawLine(r,x-3,y-jump,x-7,y+ay-jump);SDL_RenderDrawLine(r,x+3,y-jump,x+7,y+ay-jump);
}
static void build_crowd_poses(SDL_Renderer*r){
    for(int team=0;team<2;team++)for(int phase=0;phase<2;phase++){
        if(crowd_poses[team][phase]){SDL_DestroyTexture(crowd_poses[team][phase]);}crowd_poses[team][phase]=NULL;
        SDL_Surface*s=SDL_CreateRGBSurfaceWithFormat(0,SW,SH,32,SDL_PIXELFORMAT_RGBA32);if(!s)continue;SDL_Renderer*tmp=SDL_CreateSoftwareRenderer(s);if(!tmp){SDL_FreeSurface(s);continue;}SDL_SetRenderDrawColor(tmp,0,0,0,0);SDL_RenderClear(tmp);
        for(int row=0;row<3;row++){
            for(int x=FX-60;x<FX+FW+61;x+=14){if(((x-FX+60)/14)%15==0)continue;bool blue=x>SW/2;fan_pose(tmp,x,FY-39-row*16,blue,blue==(team!=0),phase,row);fan_pose(tmp,x,FY+FH+30+row*16,blue,blue==(team!=0),phase,row);if(x+7<FX+FW+61){fan_pose(tmp,x+7,FY-39-row*16,blue,blue==(team!=0),phase,row);fan_pose(tmp,x+7,FY+FH+30+row*16,blue,blue==(team!=0),phase,row);}}
            for(int y=FY+14;y<FY+FH;y+=14){if(abs(y-SH/2)<(int)goal_h()+48)continue;fan_pose(tmp,FX-32-row*16,y,false,team==0,phase,row);fan_pose(tmp,FX+FW+32+row*16,y,true,team==1,phase,row);if(y+7<FY+FH){fan_pose(tmp,FX-32-row*16,y+7,false,team==0,phase,row);fan_pose(tmp,FX+FW+32+row*16,y+7,true,team==1,phase,row);}}
        }
        SDL_RenderPresent(tmp);SDL_DestroyRenderer(tmp);crowd_poses[team][phase]=SDL_CreateTextureFromSurface(r,s);SDL_FreeSurface(s);if(crowd_poses[team][phase])SDL_SetTextureBlendMode(crowd_poses[team][phase],SDL_BLENDMODE_BLEND);
    }
}
static void reacting_crowd(SDL_Renderer*r,bool results){
    if(!results&&goal_effect<=0)return;
    bool blue=results?blue_score>red_score:last_goal_blue;int phase=(SDL_GetTicks()/180)%2;
    if(!results){SDL_Texture*t=crowd_poses[blue?1:0][phase];if(t)SDL_RenderCopy(r,t,NULL,NULL);return;}
    for(int team=0;team<2;team++)for(int i=0;i<12;i++)fan_pose(r,FX+24+team*FW/2+i*(FW/2-48)/12,565,team!=0,(team!=0)==blue,phase,1);
}
static void render_field(SDL_Renderer*r,int sec){
    static int cached_arena=-1,cached_size=-1;
    if(!pitch_cache||cached_arena!=cfg.arena||cached_size!=cfg.size){
        if(pitch_cache){SDL_DestroyTexture(pitch_cache);pitch_cache=NULL;}
        SDL_Surface*surface=SDL_CreateRGBSurfaceWithFormat(0,SW,SH,32,SDL_PIXELFORMAT_RGBA32);
        if(surface){SDL_Renderer*temporary=SDL_CreateSoftwareRenderer(surface);if(temporary){draw_pitch(temporary);SDL_RenderPresent(temporary);SDL_DestroyRenderer(temporary);pitch_cache=SDL_CreateTextureFromSurface(r,surface);}SDL_FreeSurface(surface);}
        build_crowd_poses(r);cached_arena=cfg.arena;cached_size=cfg.size;
    }
    if(pitch_cache)SDL_RenderCopy(r,pitch_cache,NULL,NULL);else draw_pitch(r);
    reacting_crowd(r,false);
    // A few waving flags provide motion while the large crowd remains cached.
    float wave=sinf(elapsed*4)*4;
    for(int i=0;i<4;i++){
        int x=FX+FW*(i+1)/5,y=FY+FH+50;
        SDL_SetRenderDrawColor(r,190,203,214,255);SDL_RenderDrawLine(r,x,y-12,x,y+10);
        box(r,x+1,y-12+(int)(wave*(i%2?1:-1)),19,10,i<2?TEAM_RED_R:TEAM_BLUE_R,i<2?TEAM_RED_G:TEAM_BLUE_G,i<2?TEAM_RED_B:TEAM_BLUE_B);
    }
    box(r,275,18,730,54,12,20,31);box(r,275,18,5,54,TEAM_RED_R,TEAM_RED_G,TEAM_RED_B);box(r,1000,18,5,54,TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B);
    int ra=25,ba=25;while(text_width(cfg.red,ra)>210&&ra>14)ra--;while(text_width(cfg.blue,ba)>210&&ba>14)ba--;
    ink_text(r,cfg.red,395,45,ra,31,242,247,251);ink_text(r,cfg.blue,885,45,ba,31,242,247,251);
    box(r,515,18,62,54,TEAM_RED_R,TEAM_RED_G,TEAM_RED_B);box(r,703,18,62,54,TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B);
    char score[16];snprintf(score,sizeof(score),"%d",red_score);score_text(r,score,546,45);snprintf(score,sizeof(score),"%d",blue_score);score_text(r,score,734,45);
    if(state==GOLDEN)ink_text(r,"GOL DE ORO",640,45,16,23,255,223,144);else{char clock[20];snprintf(clock,sizeof(clock),"%02d:%02d",sec/60,sec%60);ink_text(r,clock,640,45,27,31,sec<=15?255:232,sec<=15?65:242,sec<=15?75:248);}

}
static void draw_countdown(SDL_Renderer*r){
    if(state!=COUNTDOWN)return;
    int cx=SW/2,cy=SH/2;SDL_SetRenderDrawColor(r,12,20,32,255);circle(r,cx,cy,64);
    SDL_SetRenderDrawColor(r,47,69,77,255);arc(r,cx,cy,62,62,0,6.2831853f,false);
    float fraction=1-(countdown-floorf(countdown));SDL_SetRenderDrawColor(r,85,219,191,255);
    for(int i=0;i<4;i++)arc(r,cx,cy,62-i,62-i,-1.5707963f,-1.5707963f+fraction*6.2831853f,false);
    if(countdown_texture){int n=countdown_length==1?0:(countdown<1?2:(countdown<2?1:0));SDL_Rect src=COUNTDOWN_RECTS[n],dst={cx-src.w/2,cy-src.h/2,src.w,src.h};SDL_RenderCopy(r,countdown_texture,&src,&dst);}
}
static void ring(SDL_Renderer*r,int x,int y,int radius,int thickness){
    if(draw_round_mask(r,x,y,radius,thickness))return;
    for(int t=0;t<thickness;t++)for(int a=0;a<96;a++){
        float q=a*6.2831853f/96,z=(a+1)*6.2831853f/96;
        SDL_RenderDrawLine(r,x+(int)(cosf(q)*(radius-t)),y+(int)(sinf(q)*(radius-t)),x+(int)(cosf(z)*(radius-t)),y+(int)(sinf(z)*(radius-t)));
    }
}
static Ball keepers[2];
static void draw_player(SDL_Renderer*r,Player*p,int index){
    (void)index;int x=(int)p->x,y=(int)p->y;
    if((p->aiming&&p->press_time<TAP_WINDOW+CONTROL_WINDOW)||p->press_flash>0){int c=cfg.arena==HOCKEY?0:255;SDL_SetRenderDrawColor(r,c,c,c,255);ring(r,x,y,(int)PR+6,3);}

    SDL_SetRenderDrawColor(r,p->blue?TEAM_BLUE_R:TEAM_RED_R,p->blue?TEAM_BLUE_G:TEAM_RED_G,p->blue?TEAM_BLUE_B:TEAM_RED_B,255);circle(r,x,y,(int)PR);
    if(cfg.names&&p->human){char name[20];if(p->human)snprintf(name,sizeof(name),"%.15s",human_names[p->slot]);else snprintf(name,sizeof(name),"CPU %d",index+1);
        int w=text_width(name,18),labelx=(int)clampf(x,w/2+7,SW-w/2-7),labely=y-(int)PR-18;box(r,labelx-w/2-7,labely-13,w+14,27,15,27,39);box(r,labelx-w/2-7,labely+12,w+14,2,p->blue?TEAM_BLUE_R:TEAM_RED_R,p->blue?TEAM_BLUE_G:TEAM_RED_G,p->blue?TEAM_BLUE_B:TEAM_RED_B);ink_text(r,name,labelx,labely,18,23,240,246,252);
    }
}
static void draw_game(SDL_Renderer*r,int sec){
    if(cfg.arena==BASH){draw_bash_game(r);return;}render_field(r,sec);if(cfg.arena==HANDBALL)for(int k=0;k<2;k++){SDL_SetRenderDrawColor(r,k?TEAM_BLUE_R:TEAM_RED_R,k?TEAM_BLUE_G:TEAM_RED_G,k?TEAM_BLUE_B:TEAM_RED_B,255);circle(r,(int)keepers[k].x,(int)keepers[k].y,11);SDL_SetRenderDrawColor(r,255,226,105,255);ring(r,(int)keepers[k].x,(int)keepers[k].y,11,2);}for(int i=0;i<players_n;i++)draw_player(r,&ps[i],i);

    if(kick_flash>0){int c=cfg.arena==HOCKEY?0:255;SDL_SetRenderDrawColor(r,c,c,c,255);ring(r,(int)ball.x,(int)ball.y,(int)(BR+5+10*kick_flash/.12f),2);}
    SDL_SetRenderDrawColor(r,cfg.arena==HOCKEY?24:255,cfg.arena==HOCKEY?31:255,cfg.arena==HOCKEY?39:255,255);circle(r,(int)ball.x,(int)ball.y,(int)BR);


    draw_countdown(r);
    if(goal_effect>0){
        float progress=1-goal_effect/1.9f;int width=520+(int)(sinf(fminf(1,progress*4)*1.5707963f)*90);
        box(r,SW/2-width/2,SH/2-76,width,152,12,20,32);
        box(r,SW/2-width/2,SH/2-76,width,5,last_goal_blue?TEAM_BLUE_R:TEAM_RED_R,last_goal_blue?TEAM_BLUE_G:TEAM_RED_G,last_goal_blue?TEAM_BLUE_B:TEAM_RED_B);
        centered(r,"¡GOL!",SH/2-66,56,69,255,255,255);char who[80];snprintf(who,sizeof(who),"%s · %s",last_goal_blue?cfg.blue:cfg.red,scorer>=0&&scorer<players_n&&ps[scorer].human?human_names[ps[scorer].slot]:"CPU");centered(r,who,SH/2+16,26,35,220,237,249);
        for(int i=0;i<18;i++){float phase=i*2.39996f;int x=SW/2+(int)(cosf(phase)*(170+progress*180)),y=SH/2+(int)(sinf(phase)*(90+progress*110));SDL_SetRenderDrawColor(r,last_goal_blue?TEAM_BLUE_R:TEAM_RED_R,last_goal_blue?TEAM_BLUE_G:TEAM_RED_G,last_goal_blue?TEAM_BLUE_B:TEAM_RED_B,255);circle(r,x,y,3);}
    }
    if(paused){box(r,275,267,730,315,8,20,21);centered(r,auto_pause?"MANDO DESCONECTADO":"PAUSA",286,34,44,255,255,255);
        row(r,restart_confirm?"REINICIAR EL PARTIDO?":"REANUDAR",365,pause_row==0);
        if(!restart_confirm){row(r,"REINICIAR",415,pause_row==1);row(r,"OPCIONES",465,pause_row==2);row(r,"SALIR A EQUIPOS",515,pause_row==3);}
    }if(paused)control_legend(r);SDL_RenderPresent(r);
}


static void reset_keepers(void){for(int i=0;i<2;i++)keepers[i]=(Ball){FX+(i?FW-24:24),SH*.5f,0,0};}
static void initialize_match(void){ball_owner=-1;players_n=blue_n=red_n=0;for(int team=0;team<2;team++){int sign=team==0?1:-1,slots[4],n=0;for(int i=0;i<MAX_PADS;i++)if(side[i]==sign&&padIsConnected(&pads[i])&&n<4)slots[n++]=i;for(int j=0;j<cfg.size;j++){Player*p=&ps[players_n++];memset(p,0,sizeof(*p));p->blue=sign==1;p->aim_x=p->blue?-1:1;p->r=p->blue?TEAM_BLUE_R:TEAM_RED_R;p->g=p->blue?TEAM_BLUE_G:TEAM_RED_G;p->b=p->blue?TEAM_BLUE_B:TEAM_RED_B;p->team_index=j;p->slot=j<n?slots[j]:-1;p->human=p->slot>=0;p->x=FX+FW*(p->blue?.73f:.27f);p->y=SH*.5f+(j-(cfg.size-1)*.5f)*60;if(p->blue)blue_n++;else red_n++;}}
 reset_keepers();ball=(Ball){SW*.5f,SH*.5f,0,0};blue_score=red_score=shots=touches=0;elapsed=countdown=stuck=0;countdown_length=3;stuck_x=ball.x;stuck_y=ball.y;match_seconds=minutes[cfg.duration]*60;state=COUNTDOWN;goal_effect=0;goal_flight_active=false;goal_net_released=false;memset(team_stats,0,sizeof(team_stats));memset(contact_wait,0,sizeof(contact_wait));paused=auto_pause=false;vote_mask=0;last_touch=-1;tactics_wait=0;memset(ai_wait,0,sizeof(ai_wait));memset(kick_wait,0,sizeof(kick_wait));memset(kick_grace,0,sizeof(kick_grace));page=MATCH;if(cfg.arena==BASH)bash_init();}
static void restart_round(void){initialize_match();}
static bool holds_ball(const Player*p){float x=ball.x-p->x,y=ball.y-p->y;return sqrtf(x*x+y*y)<PR+BR+18;}
static void segment_contact(float*x,float*y,float*vx,float*vy,float radius,float ax,float ay,float bx,float by){
    float sx=bx-ax,sy=by-ay,len=sx*sx+sy*sy,t=len>0?clampf(((*x-ax)*sx+(*y-ay)*sy)/len,0,1):0;
    float dx=*x-(ax+t*sx),dy=*y-(ay+t*sy),d=hypotf(dx,dy);if(d>=radius)return;
    float nx=d>.001f?dx/d:1,ny=d>.001f?dy/d:0;*x=ax+t*sx+nx*radius;*y=ay+t*sy+ny*radius;
    float closing=*vx*nx+*vy*ny;if(closing<0){*vx-=1.65f*closing*nx;*vy-=1.65f*closing*ny;}
}
static void hockey_net_contact(float*x,float*y,float*vx,float*vy,float radius){
    if(cfg.arena!=HOCKEY)return;
    for(int side=0;side<2;side++){
        float front=goal_line(side!=0),back=front+(side?GOAL_DEPTH:-GOAL_DEPTH),top=SH*.5f-goal_h(),bottom=SH*.5f+goal_h();
        segment_contact(x,y,vx,vy,radius,back,top,back,bottom);segment_contact(x,y,vx,vy,radius,back,top,front,top);segment_contact(x,y,vx,vy,radius,back,bottom,front,bottom);
    }
}
static void rounded_rink(float*x,float*y,float*vx,float*vy,float radius){
    if(cfg.arena!=HOCKEY)return;
    float corner=FH*.24f,cx=clampf(*x,FX+corner,FX+FW-corner),cy=clampf(*y,FY+corner,FY+FH-corner);
    float dx=*x-cx,dy=*y-cy,d=hypotf(dx,dy),limit=corner-radius;
    if(d>limit){float nx=dx/d,ny=dy/d;*x=cx+nx*limit;*y=cy+ny*limit;float v=*vx*nx+*vy*ny;if(v>0){*vx-=1.8f*v*nx;*vy-=1.8f*v*ny;}}
}
static void handball_area(Player*p){
    if(cfg.arena!=HANDBALL)return;
    for(int i=0;i<2;i++){
        float gx=FX+(i?FW:0),cy=clampf(p->y,SH*.5f-FH*.13f,SH*.5f+FH*.13f);
        float rx=FW*.14f+PR,ry=FH*.19f+PR,dx=p->x-gx,dy=p->y-cy,d=hypotf(dx/rx,dy/ry);
        if(d<1){if(d<.001f){dx=i?-rx:rx;dy=0;d=1;}p->x=gx+dx/d;p->y=cy+dy/d;float nx=dx/(rx*rx),ny=dy/(ry*ry),len=hypotf(nx,ny);nx/=len;ny/=len;float v=p->vx*nx+p->vy*ny;if(v<0){p->vx-=v*nx;p->vy-=v*ny;}}
    }
}
static void keeper_step(float dt){
    if(cfg.arena!=HANDBALL)return;
    for(int i=0;i<2;i++){
        Ball*k=&keepers[i];float gx=FX+(i?FW:0),dir=i?-1:1;
        float depth=clampf((ball.x-gx)*dir*.42f,24,FW*.105f),tx=gx+dir*depth,ty=clampf(ball.y,SH*.5f-FH*.25f,SH*.5f+FH*.25f);
        float dxmove=tx-k->x,dymove=ty-k->y,dmove=hypotf(dxmove,dymove),step=fminf(dmove,65*dt);
        k->vx=dmove>.001f?dxmove/dmove*65:0;k->vy=dmove>.001f?dymove/dmove*65:0;
        if(dmove>.001f){k->x+=dxmove/dmove*step;k->y+=dymove/dmove*step;}
        float core=FH*.13f,cy=clampf(k->y,SH*.5f-core,SH*.5f+core),rx=FW*.14f-12,ry=FH*.19f-12;
        float ex=k->x-gx,ey=k->y-cy,edge=hypotf(ex/rx,ey/ry);if(edge>1){k->x=gx+ex/edge;k->y=cy+ey/edge;}k->x=clampf(k->x,FX+12,FX+FW-12);
        float dx=ball.x-k->x,dy=ball.y-k->y,d=hypotf(dx,dy),radius=11+BR;
        if(d<radius+4){float nx=d>.001f?dx/d:(i?-1:1),ny=d>.001f?dy/d:0;ball.x=k->x+nx*radius;ball.y=k->y+ny*radius;float v=(ball.vx-k->vx)*nx+(ball.vy-k->vy)*ny;if(v<0||hypotf(ball.vx,ball.vy)<20){int target=-1;float best=1e9f;for(int j=0;j<players_n;j++)if(ps[j].blue==(i!=0)){float px=ps[j].x-ball.x,py=ps[j].y-ball.y,len=hypotf(px,py);bool clear=len>30;for(int o=0;o<players_n;o++)if(ps[o].blue!=(i!=0)){float t=clampf(((ps[o].x-ball.x)*px+(ps[o].y-ball.y)*py)/fmaxf(1,len*len),0,1);if(hypotf(ps[o].x-ball.x-t*px,ps[o].y-ball.y-t*py)<PR+BR+10)clear=false;}if(clear&&len<best){best=len;target=j;}}ball.vx=ball.vy=0;if(target>=0){float px=ps[target].x-ball.x,py=ps[target].y-ball.y,len=hypotf(px,py);px/=len;py/=len;ball.x=k->x+px*(radius+8);ball.y=k->y+py*(radius+8);ball.vx=px*430;ball.vy=py*430;}last_touch=i;scorer=-1;}}
    }
}
static void move_player(Player*p,float x,float y,float dt,float speed){
    // Short acceleration and braking preserve fine analog control without instant velocity jumps.
    float response=1-expf(-(x*x+y*y<.001f?30.0f:22.0f)*dt);
    p->vx+=(x*speed-p->vx)*response;p->vy+=(y*speed-p->vy)*response;
    float nx=p->x+p->vx*dt,ny=p->y+p->vy*dt;
    p->x=clampf(nx,FX+PR,FX+FW-PR);p->y=clampf(ny,FY+PR,FY+FH-PR);
    if(nx!=p->x)p->vx=0;
    if(ny!=p->y)p->vy=0;
    hockey_net_contact(&p->x,&p->y,&p->vx,&p->vy,PR);rounded_rink(&p->x,&p->y,&p->vx,&p->vy,PR);handball_area(p);
}
static void strike(Player*p,float x,float y){
    float d=hypotf(x,y);if(d<.001f)return;x/=d;y/=d;ball_owner=-1;
    // A real impulse retains some incoming momentum and rewards stepping into the shot.
    float power=p->controlled?clampf(315+fmaxf(0,p->vx*x+p->vy*y)*.12f,315,350):clampf(540+fmaxf(0,p->vx*x+p->vy*y)*.20f+(ball.vx*x+ball.vy*y)*.08f,520,610);p->controlled=false;
    ball.vx=x*power;ball.vy=y*power;p->aim_x=x;p->aim_y=y;p->press_flash=0;
    tone(180,65);kick_grace[p-ps]=.14f;kick_flash=.12f;last_touch=p->blue?1:0;scorer=(int)(p-ps);shots++;team_stats[p->blue?1:0].kicks++;
}
static void cancel_aim(void){ball_owner=-1;for(int i=0;i<players_n;i++){ps[i].aiming=false;ps[i].press_time=0;ps[i].shot_lock=true;}}
static void update_shot(Player*p,float dt){
    int slot=p->slot,id=(int)(p-ps);bool held=(input_held[slot]&HidNpadButton_A)!=0;
    if(!held)p->shot_lock=false;
    if((down(slot)&HidNpadButton_A)&&!p->shot_lock){p->aiming=true;p->controlled=false;p->press_time=0;p->press_flash=.20f;}
    if(!p->aiming)return;
    p->press_time+=dt;
    if(!held){
        p->aiming=false;p->press_flash=0;if(ball_owner==id)ball_owner=-1;
        if(holds_ball(p)){strike(p,p->aim_x,p->aim_y);}
        return;
    }
    if(p->press_time>=TAP_WINDOW&&p->press_time<TAP_WINDOW+CONTROL_WINDOW&&ball_owner<0&&holds_ball(p)&&hypotf(ball.vx-p->vx,ball.vy-p->vy)<450){ball_owner=id;p->controlled=true;}
    if(p->press_time>=TAP_WINDOW+CONTROL_WINDOW&&ball_owner==id)ball_owner=-1;
}
static void control_ball(float dt){
    if(ball_owner<0)return;
    Player*p=&ps[ball_owner];
    if(!p->aiming||p->press_time>=TAP_WINDOW+CONTROL_WINDOW||hypotf(ball.x-p->x,ball.y-p->y)>PR+BR+38){ball_owner=-1;return;}
    // Opponents can contest a controlled ball; control never disables their contacts.
    for(int j=0;j<players_n;j++)if(j!=ball_owner&&ps[j].blue!=p->blue&&hypotf(ball.x-ps[j].x,ball.y-ps[j].y)<PR+BR+5){ball_owner=-1;return;}
    float tx=p->x+p->aim_x*(PR+BR+3),ty=p->y+p->aim_y*(PR+BR+3);
    float target_vx=p->vx+(tx-ball.x)*18,target_vy=p->vy+(ty-ball.y)*18;
    float blend=1-expf(-24*dt);
    ball.vx+=(target_vx-ball.vx)*blend;ball.vy+=(target_vy-ball.vy)*blend;
    last_touch=p->blue?1:0;scorer=(int)(p-ps);
}
static float lane_clearance(float x,float y,float tx,float ty,bool blue){
    float vx=tx-x,vy=ty-y,len=vx*vx+vy*vy,clear=1000;
    for(int i=0;i<players_n;i++)if(ps[i].blue!=blue){
        float t=len>1?clampf(((ps[i].x-x)*vx+(ps[i].y-y)*vy)/len,0,1):0;
        float d=hypotf(ps[i].x-x-t*vx,ps[i].y-y-t*vy);if(d<clear)clear=d;
    }return clear;
}
static void assign_roles(void){
    for(int team=0;team<2;team++){
        bool blue=team==0;float attack=blue?-1:1,own=goal_line(blue);
        int chase=-1,cover=-1;float best=1e9f;
        for(int i=0;i<players_n;i++)if(ps[i].blue==blue){
            float d=hypotf(ps[i].x-ball.x,ps[i].y-ball.y);
            // A nearby human can contest; a distant human never immobilizes the bots.
            float score=d+fmaxf(0,(ps[i].x-ball.x)*attack)*.35f-(roles[i]==CHASE?32:0);
            if(score<best){best=score;chase=i;}
        }
        best=1e9f;
        for(int i=0;i<players_n;i++)if(ps[i].blue==blue&&i!=chase){
            float score=hypotf(ps[i].x-(own+attack*140),ps[i].y-SH*.5f)-(roles[i]==COVER?45:0);
            if(score<best){best=score;cover=i;}
        }
        int rest[4],n=0;for(int i=0;i<players_n;i++)if(ps[i].blue==blue&&i!=chase&&i!=cover)rest[n++]=i;
        if(chase>=0)roles[chase]=CHASE;
        if(cover>=0)roles[cover]=COVER;
        // Alternate the two support duties, staggered between teams.
        int rotation=((int)(elapsed/4.5f)+team)%2;
        for(int j=0;j<n;j++)roles[rest[j]]=((j+rotation)%2)?RUN:SUPPORT;
    }
}
static void bot_move(Player*p,float*dx,float*dy){
    int id=(int)(p-ps);float attack=p->blue?-1:1;
    if(ai_wait[id]<=0){
        float own=goal_line(p->blue),tx=ball.x,ty=ball.y;
        float prediction=cfg.difficulty==0?.06f:(cfg.difficulty==1?.18f:.30f);
        bool defending=(ball.x-SW*.5f)*attack<0;
        float wave=sinf(elapsed*(.75f+.08f*p->team_index)+id*1.7f);
        if(roles[id]==CHASE){
            tx=ball.x+ball.vx*prediction-attack*(PR+BR+8);ty=ball.y+ball.vy*prediction;
            if(holds_ball(p)){tx=ball.x+attack*65;ty=ball.y*.7f+SH*.15f;}
        }else if(roles[id]==COVER){
            tx=defending?own+attack*80:ball.x-attack*240;
            ty=clampf(SH*.5f+(ball.y-SH*.5f)*.55f+ball.vy*prediction,SH*.5f-140,SH*.5f+140);
        }else if(roles[id]==SUPPORT){
            tx=ball.x-attack*(defending?80:25);
            ty=ball.y+(ball.y<SH*.5f?1:-1)*(145+wave*35);
        }else{
            tx=ball.x+attack*(defending?90:200)+attack*wave*40;
            ty=SH*.5f+(ball.y<SH*.5f?-1:1)*145-wave*65;
        }
        // Spread teammates and seek an open passing lane instead of following a line.
        if(roles[id]!=CHASE){
            for(int j=0;j<players_n;j++)if(j!=id&&ps[j].blue==p->blue){
                float x=tx-ps[j].x,y=ty-ps[j].y,d=hypotf(x,y);
                if(d<110){tx+=(d>1?x/d:0)*(110-d)*.5f;ty+=(d>1?y/d:(id%2?1:-1))*(110-d);}
            }
            if(cfg.difficulty>0&&lane_clearance(ball.x,ball.y,tx,ty,p->blue)<PR+BR)ty+=wave>=0?70:-70;
        }
        // Any possessor carries forward; support/cover must not retreat with the ball.
        if(holds_ball(p)){tx=ball.x+attack*90;ty=ball.y+(SH*.5f-ball.y)*.22f;if(cfg.arena==HOCKEY&&(ball.x-goal_line(!p->blue))*attack>0){tx=goal_line(!p->blue)-attack*100;ty=SH*.5f+(ball.y<SH*.5f?-1:1)*(goal_h()+60);}}
        ai_tx[id]=clampf(tx,FX+PR+8,FX+FW-PR-8);ai_ty[id]=clampf(ty,FY+PR+8,FY+FH-PR-8);
        ai_wait[id]=(cfg.difficulty==0?.48f:(cfg.difficulty==1?.22f:.09f))*(.85f+(id%3)*.16f);
        if(holds_ball(p)&&kick_wait[id]<=0){
            float gx=goal_line(!p->blue)+(p->blue?-25:25),gy=SH*.5f;
            float goal_distance=fabsf(gx-ball.x),clear=lane_clearance(ball.x,ball.y,gx,gy,p->blue);
            if(cfg.difficulty>0)for(int aim=-1;aim<=1;aim++){float candidate=SH*.5f+aim*(goal_h()-BR-12),space=lane_clearance(ball.x,ball.y,gx,candidate,p->blue);if(space>clear){clear=space;gy=candidate;}}
            bool front=(goal_line(!p->blue)-ball.x)*attack>0;
            float pressure=1000;for(int j=0;j<players_n;j++)if(ps[j].blue!=p->blue){float d=hypotf(ps[j].x-p->x,ps[j].y-p->y);if(d<pressure)pressure=d;}
            int mate=-1;float best=-1e9f;
            for(int j=0;j<players_n;j++)if(j!=id&&ps[j].blue==p->blue){
                float distance=hypotf(ps[j].x-ball.x,ps[j].y-ball.y);
                float lane=lane_clearance(ball.x,ball.y,ps[j].x,ps[j].y,p->blue);
                float progress=(ps[j].x-ball.x)*attack;
                float score=progress+fminf(lane,140)*1.5f-distance*.15f;
                if(distance>85&&distance<500&&progress>-(pressure<100?130:25)&&lane>PR+BR+8&&score>best){mate=j;best=score;}
            }
            float range=cfg.difficulty==0?340:(cfg.difficulty==1?440:520);bool shoot=front&&goal_distance<range&&clear>PR+BR+5;
            bool pass=mate>=0&&!shoot&&(pressure<120||best>160||(clear<PR+BR+10&&best>80));
            if(cfg.difficulty==0)pass=pass&&sinf(elapsed+id)>.35f;
            if(pass&&!shoot){gx=ps[mate].x+ps[mate].vx*prediction;gy=ps[mate].y+ps[mate].vy*prediction;}
            // Carry the ball when safe; clear it when pressed and no pass is open.
            if(shoot||pass||pressure<75){
                gy+=(cfg.difficulty==0?75:(cfg.difficulty==1?30:8))*sinf(elapsed*3+id);
                float x=gx-ball.x,y=gy-ball.y,d=hypotf(x,y);
                if(d>1){strike(p,x/d,y/d);}
                kick_wait[id]=cfg.difficulty==0?.9f:(cfg.difficulty==1?.65f:.45f);
            }
        }
    }
    *dx=ai_tx[id]-p->x;*dy=ai_ty[id]-p->y;float d=hypotf(*dx,*dy);
    if(d>6){float speed=clampf(d/55,.22f,1);*dx=*dx/d*speed;*dy=*dy/d*speed;p->aim_x=*dx/speed;p->aim_y=*dy/speed;}else *dx=*dy=0;
}
static void collisions(void){for(int i=0;i<players_n;i++)for(int j=i+1;j<players_n;j++){float x=ps[j].x-ps[i].x,y=ps[j].y-ps[i].y,d=sqrtf(x*x+y*y),m=PR*2;if(d>0&&d<m){float nx=x/d,ny=y/d,p=(m-d)*.5f;ps[i].x-=nx*p;ps[i].y-=ny*p;ps[j].x+=nx*p;ps[j].y+=ny*p;}}}
static void cap_ball(void){float v=sqrtf(ball.vx*ball.vx+ball.vy*ball.vy);if(v>550){ball.vx*=550/v;ball.vy*=550/v;}}
static void ball_player(Player*p,float dx,float dy){
    (void)dx;(void)dy;if(kick_grace[p-ps]>0||ball_owner==(int)(p-ps))return;
    float x=ball.x-p->x,y=ball.y-p->y,d=hypotf(x,y),m=PR+BR;if(d>=m)return;
    float nx=d>.001f?x/d:1,ny=d>.001f?y/d:0;
    ball.x=p->x+nx*m;ball.y=p->y+ny*m;
    float closing=(ball.vx-p->vx)*nx+(ball.vy-p->vy)*ny;
    if(closing<0){
        // Soft low-speed contacts allow carrying; hard incoming balls still rebound.
        float restitution=fabsf(closing)<330?.08f:.50f;
        ball.vx-=(1+restitution)*closing*nx;ball.vy-=(1+restitution)*closing*ny;
        if(contact_wait[p-ps]<=0){touches++;team_stats[p->blue?1:0].contacts++;contact_wait[p-ps]=.15f;}last_touch=p->blue?1:0;scorer=(int)(p-ps);
    }
}
static void post_collisions(void){
    float xs[]={goal_line(false),goal_line(true)},ys[]={SH*.5f-goal_h(),SH*.5f+goal_h()};
    for(int i=0;i<2;i++)for(int j=0;j<2;j++){
        float x=ball.x-xs[i],y=ball.y-ys[j],d=hypotf(x,y),radius=BR+4;
        if(d>=radius)continue;
        float nx=d>.001f?x/d:1,ny=d>.001f?y/d:0,closing=ball.vx*nx+ball.vy*ny;
        ball.x=xs[i]+nx*radius;ball.y=ys[j]+ny*radius;
        if(closing<0){ball.vx-=1.8f*closing*nx;ball.vy-=1.8f*closing*ny;if(closing<-45){if(last_touch>=0)team_stats[last_touch].posts++;tone(750,65);}}
    }
}
static void start_goal_flight(bool left){
    float front=left?goal_line(false):goal_line(true),dir=left?-1.0f:1.0f;
    goal_net_target_x=front+dir*(GOAL_DEPTH-BR);goal_net_direction=dir;goal_flight_active=true;goal_net_released=false;
}
static bool goal(void){
    post_collisions();float top=SH*.5f-goal_h(),bottom=SH*.5f+goal_h(),left=goal_line(false),right=goal_line(true);
    if(ball.y>top+BR&&ball.y<bottom-BR){
        if(ball_previous_x>=left&&ball.x<left&&ball.vx<0){blue_score++;last_goal_blue=true;start_goal_flight(true);return true;}
        if(ball_previous_x<=right&&ball.x>right&&ball.vx>0){red_score++;last_goal_blue=false;start_goal_flight(false);return true;}
    }
    hockey_net_contact(&ball.x,&ball.y,&ball.vx,&ball.vy,BR);
    if(ball.y<FY+BR){ball.y=FY+BR;ball.vy=fabsf(ball.vy)*.86f;}else if(ball.y>FY+FH-BR){ball.y=FY+FH-BR;ball.vy=-fabsf(ball.vy)*.86f;}
    bool wall=cfg.arena==HOCKEY||ball.y<top||ball.y>bottom;
    if(wall&&ball.x<FX+BR){ball.x=FX+BR;ball.vx=fabsf(ball.vx)*.86f;}else if(wall&&ball.x>FX+FW-BR){ball.x=FX+FW-BR;ball.vx=-fabsf(ball.vx)*.86f;}
    return false;
}
static void advance_goal_flight(float dt){
    if(!goal_flight_active)return;
    float dir=goal_net_direction,front=goal_net_target_x-dir*(GOAL_DEPTH-BR);
    float top=SH*.5f-goal_h()+BR,bottom=SH*.5f+goal_h()-BR;
    ball.x+=ball.vx*dt;ball.y+=ball.vy*dt;
    float depth=(ball.x-front)*dir;
    if(depth>GOAL_DEPTH-BR){
        ball.x=goal_net_target_x;
        ball.vx=-dir*fabsf(ball.vx)*.19f;ball.vy*=.82f;
    }
    if(depth>=0){
        if(ball.y<top){ball.y=top;ball.vy=fabsf(ball.vy)*.19f;ball.vx*=.82f;}
        if(ball.y>bottom){ball.y=bottom;ball.vy=-fabsf(ball.vy)*.19f;ball.vx*=.82f;}
    }else{
        if(ball.y<FY+BR){ball.y=FY+BR;ball.vy=fabsf(ball.vy)*.65f;}
        if(ball.y>FY+FH-BR){ball.y=FY+FH-BR;ball.vy=-fabsf(ball.vy)*.65f;}
    }
    float damping=powf(friction(),dt*60);ball.vx*=damping;ball.vy*=damping;
}
static void kickoff(void){reset_keepers();ball_owner=-1;last_touch=-1;for(int i=0;i<players_n;i++){ps[i].x=FX+FW*(ps[i].blue?.73f:.27f);ps[i].y=SH*.5f+(ps[i].team_index-(cfg.size-1)*.5f)*60;ps[i].aiming=false;ps[i].shot_lock=true;ps[i].press_flash=0;ps[i].vx=ps[i].vy=0;ps[i].aim_x=ps[i].blue?-1:1;ps[i].aim_y=0;}tactics_wait=0;memset(kick_grace,0,sizeof(kick_grace));goal_flight_active=false;goal_net_released=false;ball=(Ball){SW*.5f,SH*.5f,0,0};state=COUNTDOWN;countdown_length=1;countdown=stuck=0;stuck_x=ball.x;stuck_y=ball.y;}
static bool any_lost(void){for(int i=0;i<players_n;i++)if(ps[i].human&&ps[i].slot>=0&&!padIsConnected(&pads[ps[i].slot]))return true;return false;}
static float results_elapsed=5;
static void update_match(float dt){
    if(cfg.arena==BASH){bash_update(dt);return;}if(page==RESULTS)results_elapsed+=dt;
    for(int i=0;i<players_n;i++)if(ps[i].human&&ps[i].slot>=0){int s=ps[i].slot;if(!padIsConnected(&pads[s])){ps[i].lost+=dt;paused=auto_pause=true;}else ps[i].lost=0;}
    for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])&&(down(i)&HidNpadButton_Plus)&&(state==COUNTDOWN||can_pause(i))){paused=true;pause_row=0;}
    if(paused){cancel_aim();u64 k=0;for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])){k=navigation(i);if(k)break;}
        if(restart_confirm){if(k&HidNpadButton_B)restart_confirm=false;if(k&HidNpadButton_A){restart_confirm=false;restart_round();}return;}
        if((k&HidNpadButton_B)&&!restart_confirm&&!any_lost()){paused=auto_pause=false;return;}
        if(k&HidNpadButton_Up)pause_row=(pause_row+3)%4;
        if(k&HidNpadButton_Down)pause_row=(pause_row+1)%4;
        if((k&HidNpadButton_X)&&auto_pause){int active=0;for(int i=0;i<players_n;i++)if(ps[i].human&&ps[i].slot>=0&&padIsConnected(&pads[ps[i].slot]))active++;if(active)for(int i=0;i<players_n;i++)if(ps[i].human&&!padIsConnected(&pads[ps[i].slot])){ps[i].human=false;ps[i].slot=-1;}}if(k&HidNpadButton_A){if(pause_row==0&&!any_lost()){paused=false;auto_pause=false;}else if(pause_row==1)restart_confirm=true;else if(pause_row==2){paused_options=true;page=OPTIONS;}else if(pause_row==3){reset_lobby_ready();paused=false;page=TEAM_SELECT;}}return;}    if(goal_effect>0){advance_goal_flight(dt);goal_effect=fmaxf(0,goal_effect-dt);if(goal_effect<=0){if(goal_finishes)page=RESULTS;else kickoff();}return;}if(state==COUNTDOWN){cancel_aim();countdown+=dt;if(countdown>=countdown_length)state=PLAYING;return;}
    tactics_wait-=dt;if(tactics_wait<=0){assign_roles();tactics_wait=cfg.difficulty==0?.65f:(cfg.difficulty==1?.35f:.18f);}for(int i=0;i<players_n;i++){contact_wait[i]-=dt;ps[i].press_flash=fmaxf(0,ps[i].press_flash-dt);ai_wait[i]-=dt;kick_wait[i]-=dt;kick_grace[i]-=dt;float dx=0,dy=0,speed=205.0f;if(ps[i].human&&ps[i].slot>=0){int s=ps[i].slot;read_stick(s,&dx,&dy);float m=sqrtf(dx*dx+dy*dy);if(m<.12f)dx=dy=0;else if(m>1){dx/=m;dy/=m;}else{float input=(m-.12f)/.88f;dx=dx/m*input;dy=dy/m*input;}float aim=hypotf(dx,dy);if(aim>.001f){ps[i].aim_x=dx/aim;ps[i].aim_y=dy/aim;}u64 b=down(s);update_shot(&ps[i],dt);if((b&HidNpadButton_Plus)&&can_pause(s)){paused=true;auto_pause=false;return;}}else{speed=205.0f;bot_move(&ps[i],&dx,&dy);}move_player(&ps[i],dx,dy,dt,speed);}
    collisions();for(int i=0;i<players_n;i++){ps[i].x=clampf(ps[i].x,FX+PR,FX+FW-PR);ps[i].y=clampf(ps[i].y,FY+PR,FY+FH-PR);hockey_net_contact(&ps[i].x,&ps[i].y,&ps[i].vx,&ps[i].vy,PR);rounded_rink(&ps[i].x,&ps[i].y,&ps[i].vx,&ps[i].vy,PR);handball_area(&ps[i]);}bool scored=false;
    int steps=(int)ceilf(dt*120);if(steps<1)steps=1;float step=dt/steps;
    for(int tick=0;tick<steps;tick++){
        ball_previous_x=ball.x;control_ball(step);ball.x+=ball.vx*step;ball.y+=ball.vy*step;
        float f=powf(friction(),step*60);ball.vx*=f;ball.vy*=f;
        if(fabsf(ball.vx)<3)ball.vx=0;
        if(fabsf(ball.vy)<3)ball.vy=0;
        for(int i=0;i<players_n;i++)ball_player(&ps[i],ps[i].vx/205,ps[i].vy/205);
        keeper_step(step);rounded_rink(&ball.x,&ball.y,&ball.vx,&ball.vy,BR);cap_ball();if(goal()){scored=true;break;}
    }
    float displacement=hypotf(ball.x-stuck_x,ball.y-stuck_y);
    if(displacement>22){stuck=0;stuck_x=ball.x;stuck_y=ball.y;}else stuck+=dt;
    if(scored){elapsed+=dt;goal_effect=1.9f;goal_finishes=state==GOLDEN||(elapsed>=match_seconds&&blue_score!=red_score);if(last_touch>=0&&last_touch!=(last_goal_blue?1:0))team_stats[last_touch].own_goals++;cancel_aim();return;}
    else if(stuck>=2.5f){ball_owner=-1;ball.x=SW*.5f;ball.y=SH*.5f;ball.vx=ball.vy=0;stuck=0;stuck_x=ball.x;stuck_y=ball.y;}
    if(last_touch>=0){for(int i=0;i<players_n;i++)if((ps[i].blue?1:0)==last_touch&&hypotf(ps[i].x-ball.x,ps[i].y-ball.y)<110){team_stats[last_touch].possession+=dt;break;}}
    elapsed+=dt;if(state==PLAYING&&elapsed>=match_seconds){if(blue_score==red_score)state=GOLDEN;else{results_elapsed=0;page=RESULTS;}}else if(state==GOLDEN&&blue_score!=red_score){results_elapsed=0;page=RESULTS;}
}
static void begin_match(void){initialize_match();}
static int connected_players(void){int n=0;for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i]))n++;return n;}
static void draw_results(SDL_Renderer*r){
    if(cfg.arena==BASH){bash_results(r);return;}
    panel(r,"Resultado","Estadísticas por equipo");
    box(r,180,180,920,365,21,29,42);box(r,180,180,8,365,TEAM_RED_R,TEAM_RED_G,TEAM_RED_B);box(r,1092,180,8,365,TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B);
    text(r,cfg.red,220,193,30,37,244,248,252);text(r,cfg.blue,1060-text_width(cfg.blue,30),193,30,37,244,248,252);
    char score[32];snprintf(score,sizeof(score),"%d   -   %d",red_score,blue_score);centered(r,score,191,34,42,255,255,255);
    const char*labels[]={"Tiros","Toques","Postes","En propia puerta","Posesión"};
    float total=team_stats[0].possession+team_stats[1].possession;
    for(int row=0;row<5;row++){
        int y=261+row*51;centered(r,labels[row],y,23,29,183,202,219);
        for(int team=0;team<2;team++){
            TeamStats*t=&team_stats[team];char value[32];int n=row==0?t->kicks:(row==1?t->contacts:(row==2?t->posts:t->own_goals));
            if(row==4){if(total>0)snprintf(value,sizeof(value),"%.0f%%",100*t->possession/total);else snprintf(value,sizeof(value),"--");}else snprintf(value,sizeof(value),"%d",n);
            text(r,value,(team?950:330)-text_width(value,27)/2,y-2,27,34,245,249,253);
        }
    }
    if(results_elapsed<5){char waitline[64];snprintf(waitline,sizeof(waitline),english_ui?"REMATCH IN %d":"REVANCHA EN %d",(int)ceilf(5-results_elapsed));box(r,355,605,570,58,12,24,36);centered(r,waitline,615,30,38,255,220,112);}
    int n=connected_players(),need=n?(n+1)/2:1,votes=0;for(int j=0;j<MAX_PADS;j++)if(side[j]&&padIsConnected(&pads[j])&&(vote_mask&(1u<<j)))votes++;
    char line[80];snprintf(line,sizeof(line),english_ui?"Rematch: %d / %d votes":"Revancha: %d / %d votos",votes,need);if(results_elapsed>=5)centered(r,line,615,30,38,255,220,112);reacting_crowd(r,true);SDL_RenderPresent(r);
}

#define BASH_X 375
#define BASH_Y 95
#define BASH_S 530
#define BASH_BUMP 78
#define BASH_RADIUS 15
static int bash_goal_scorer[4];
static Ball bash_balls[4],bash_paddles[4];static int bash_score[4],bash_last[4],bash_for[4],bash_against[4];static bool bash_active[4];static float bash_spawn[4],bash_goal_flash[4],bash_ai_target[4],bash_ai_reaction[4];static bool bash_pause=false,bash_finish=false,bash_collision_pending=false;static int bash_pause_row=0,bash_winner=-1;
static SDL_Texture*bash_cache=NULL;
static Uint8 gate_rgb[4][3]={{TEAM_RED_R,TEAM_RED_G,TEAM_RED_B},{61,220,124},{TEAM_BLUE_R,TEAM_BLUE_G,TEAM_BLUE_B},{255,201,54}};
static const char*gate_names[4]={"ROJO","VERDE","AZUL","AMARILLO"};

static int bash_required_balls(void){float f=(float)elapsed/(float)match_seconds;return 1+(f>=.75f)+(f>=.50f)+(f>=.25f);}
static void bash_init(void){
    memset(bash_goal_scorer,0xff,sizeof(bash_goal_scorer));memset(bash_for,0,sizeof(bash_for));memset(bash_against,0,sizeof(bash_against));memset(bash_score,0,sizeof(bash_score));memset(bash_active,0,sizeof(bash_active));memset(bash_spawn,0,sizeof(bash_spawn));memset(bash_goal_flash,0,sizeof(bash_goal_flash));memset(bash_ai_reaction,0,sizeof(bash_ai_reaction));memset(bash_last,0xff,sizeof(bash_last));bash_collision_pending=false;
    if(bash_cache){SDL_DestroyTexture(bash_cache);bash_cache=NULL;}bash_pause=bash_finish=false;bash_pause_row=0;bash_winner=-1;int n=0;
    for(int i=0;i<MAX_PADS&&n<4;i++)if(side[i]&&padIsConnected(&pads[i])){Player*p=&ps[n];memset(p,0,sizeof(*p));p->human=true;p->slot=i;p->x=BASH_X+BASH_S/2;p->y=BASH_Y+BASH_S/2;p->r=gate_rgb[n][0];p->g=gate_rgb[n][1];p->b=gate_rgb[n][2];n++;}
    for(int i=n;i<4;i++){Player*p=&ps[i];memset(p,0,sizeof(*p));p->human=false;p->slot=-1;p->x=BASH_X+BASH_S/2;p->y=BASH_Y+BASH_S/2;p->r=gate_rgb[i][0];p->g=gate_rgb[i][1];p->b=gate_rgb[i][2];}
    bool used[4]={false};const Uint8 palette[4][3]={{244,40,64},{61,220,124},{0,102,255},{255,201,54}};for(int j=0;j<4;j++){int color=ps[j].human?(side[ps[j].slot]>0?2:0):j;if(used[color]){int choices[4],n=0;for(int c=0;c<4;c++)if(!used[c])choices[n++]=c;color=choices[rand()%n];}used[color]=true;memcpy(gate_rgb[j],palette[color],3);gate_names[j]=color==0?"ROJO":color==1?"VERDE":color==2?"AZUL":"AMARILLO";}// Keep each controller and its chosen colour together when assigning goal sides.
    static int previous[4]={-1,-1,-1,-1};int order[4],attempt=0;bool repeated;
    do{for(int i=0;i<4;i++)order[i]=i;for(int i=3;i>0;i--){int j=rand()%(i+1),tmp=order[i];order[i]=order[j];order[j]=tmp;}repeated=false;for(int i=0;i<4;i++)if(order[i]==previous[i])repeated=true;}while(repeated&&++attempt<32);
    if(repeated){int shift=1+rand()%3;for(int i=0;i<4;i++)order[i]=previous[(i+shift)%4];}
    Player original[4];Uint8 colours[4][3];const char*names[4];memcpy(original,ps,sizeof(original));memcpy(colours,gate_rgb,sizeof(colours));memcpy(names,gate_names,sizeof(names));
    for(int i=0;i<4;i++){int from=order[i];ps[i]=original[from];memcpy(gate_rgb[i],colours[from],3);gate_names[i]=names[from];previous[i]=from;ps[i].r=gate_rgb[i][0];ps[i].g=gate_rgb[i][1];ps[i].b=gate_rgb[i][2];}
    players_n=4;for(int i=0;i<4;i++){float x=i==1?BASH_X+BASH_S:(i==3?BASH_X:BASH_X+BASH_S/2),y=i==0?BASH_Y:(i==2?BASH_Y+BASH_S:BASH_Y+BASH_S/2);bash_paddles[i]=(Ball){x,y,0,0};bash_balls[i]=(Ball){x,y,0,0};bash_ai_target[i]=i%2?y:x;}
    bash_active[0]=true;bash_balls[0].x=BASH_X+BASH_S/2;bash_balls[0].y=BASH_Y+BASH_S/2;float angle=(rand()%6283)*.001f;bash_balls[0].vx=cosf(angle)*340;bash_balls[0].vy=sinf(angle)*340;countdown_length=3;
}
static void bash_pitch_circle(SDL_Renderer*r,int x,int y,int radius,Uint8 cr,Uint8 cg,Uint8 cb,int thick){SDL_SetRenderDrawColor(r,cr,cg,cb,255);if(thick>0){for(int rr=radius;rr>radius-thick;rr--)for(int a=0;a<128;a++){float q=a*6.2831853f/128,z=(a+1)*6.2831853f/128;SDL_RenderDrawLine(r,x+(int)(cosf(q)*rr),y+(int)(sinf(q)*rr),x+(int)(cosf(z)*rr),y+(int)(sinf(z)*rr));}}else circle(r,x,y,radius);}
static void draw_bash_pitch(SDL_Renderer*r){
    SDL_SetRenderDrawColor(r,12,18,27,255);SDL_RenderClear(r);box(r,BASH_X-34,BASH_Y-34,BASH_S+68,BASH_S+68,29,43,55);
    box(r,BASH_X,BASH_Y,BASH_S,BASH_S,89,98,104);for(int side=0;side<4;side++)for(int t=BASH_BUMP+12;t<BASH_S-BASH_BUMP-12;t+=40){Uint8 cr=(gate_rgb[side][0]+100)/2,cg=(gate_rgb[side][1]+100)/2,cb=(gate_rgb[side][2]+100)/2;if(side%2)box(r,side==1?BASH_X+BASH_S+11:BASH_X-18,BASH_Y+t,7,24,cr,cg,cb);else box(r,BASH_X+t,side==0?BASH_Y-18:BASH_Y+BASH_S+11,24,7,cr,cg,cb);}for(int y=0;y<BASH_S;y+=16)box(r,BASH_X,BASH_Y+y,BASH_S,1,(y/16)%2?91:96,(y/16)%2?100:105,(y/16)%2?107:112);
    SDL_SetRenderDrawColor(r,184,197,204,255);SDL_RenderDrawRect(r,&(SDL_Rect){BASH_X,BASH_Y,BASH_S,BASH_S});
    for(int i=0;i<4;i++){int x=BASH_X+(i==1||i==2?BASH_S:0),y=BASH_Y+(i>=2?BASH_S:0);bash_pitch_circle(r,x,y,BASH_BUMP+12,45,63,70,0);bash_pitch_circle(r,x,y,BASH_BUMP+8,148,166,176,4);bash_pitch_circle(r,x,y,BASH_BUMP,255,196,30,4);bash_pitch_circle(r,x,y,BASH_BUMP-8,20,46,54,0);bash_pitch_circle(r,x,y,BASH_BUMP-13,45,230,235,4);SDL_SetRenderDrawColor(r,85,245,255,255);int dirx=x<BASH_X+BASH_S/2?1:-1,diry=y<BASH_Y+BASH_S/2?1:-1;for(int pulse=0;pulse<3;pulse++){int px=x+dirx*(18+pulse*10),py=y+diry*(18+pulse*10);SDL_RenderDrawLine(r,px-dirx*5,py-diry*5,px+dirx*5,py+diry*5);}}


}
static void bash_score_goal(int ballid,int gate){int owner=bash_last[ballid];bool credited=owner>=0&&owner<4&&owner!=gate;bash_score[gate]--;bash_against[gate]++;if(credited){queue_audio_event(false);bash_score[owner]++;bash_for[owner]++;bash_goal_scorer[gate]=owner;bash_goal_flash[gate]=1.5f;}else{bash_goal_scorer[gate]=-1;bash_goal_flash[gate]=0;}bash_last[ballid]=-1;bash_active[ballid]=false;bash_spawn[ballid]=.65f;}
static bool bash_collision_soon(void){
    for(int a=0;a<4;a++)if(bash_active[a])for(int b=a+1;b<4;b++)if(bash_active[b]){
        Ball*x=&bash_balls[a],*y=&bash_balls[b];float rx=y->x-x->x,ry=y->y-x->y,rvx=y->vx-x->vx,rvy=y->vy-x->vy;
        float vv=rvx*rvx+rvy*rvy;if(vv<1)continue;float t=-(rx*rvx+ry*rvy)/vv;if(t<0||t>.55f)continue;
        float cx=rx+rvx*t,cy=ry+rvy*t,limit=BASH_RADIUS*2+40;if(cx*cx+cy*cy<limit*limit)return true;
    }
    return false;
}
static void bash_update(float dt){
    if(state==COUNTDOWN){countdown+=dt;if(countdown>=3)state=PLAYING;return;}
    for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])&&(down(i)&HidNpadButton_Plus))bash_pause=true;
    if(bash_pause){u64 k=0;for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])&&(k=navigation(i)))break;if(k&HidNpadButton_B){bash_pause=false;return;}if(k&HidNpadButton_Up)bash_pause_row=(bash_pause_row+2)%3;if(k&HidNpadButton_Down)bash_pause_row=(bash_pause_row+1)%3;if(k&HidNpadButton_A){if(bash_pause_row==0){bash_pause=false;return;}if(bash_pause_row==1){initialize_match();return;}if(bash_pause_row==2){reset_lobby_ready();page=TEAM_SELECT;return;}}return;}
    if(bash_finish)return;

    bool collision_soon=bash_collision_soon();if(collision_soon&&!bash_collision_pending){for(int p=0;p<4;p++)if(!ps[p].human)bash_ai_reaction[p]=(cfg.difficulty==0?.48f:(cfg.difficulty==1?.35f:.24f))+(rand()%180)*.001f;}bash_collision_pending=collision_soon;
    for(int p=0;p<4;p++){Player*g=&ps[p];Ball*k=&bash_paddles[p];float ox=k->x,oy=k->y;float dx=0,dy=0;if(g->human&&g->slot>=0){float sx,sy;read_stick(g->slot,&sx,&sy);if(p==0||p==2)dx=sx*370;else dy=sy*370;}else{float target=bash_ai_target[p];bash_ai_reaction[p]=fmaxf(0,bash_ai_reaction[p]-dt);if(bash_ai_reaction[p]<=0){target=p%2?BASH_Y+BASH_S/2:BASH_X+BASH_S/2;float best=1e9f;for(int q=0;q<4;q++)if(bash_active[q]){Ball*b=&bash_balls[q];float velocity=p%2?b->vx:b->vy,delta=p%2?k->x-b->x:k->y-b->y,time=fabsf(velocity)>1?delta/velocity:-1;if(time>0&&time<best){best=time;float look=cfg.difficulty==0?0:(cfg.difficulty==1?.25f:.50f);target=p%2?b->y+b->vy*fminf(time,1)*look:b->x+b->vx*fminf(time,1)*look;}}bash_ai_target[p]=target;}else target=bash_ai_target[p];float speed=cfg.difficulty==0?180:(cfg.difficulty==1?235:285);if(p%2)dy=clampf((target-k->y)*3.2f,-speed,speed);else dx=clampf((target-k->x)*3.2f,-speed,speed);}
      if(p%2){k->y=clampf(k->y+dy*dt,BASH_Y+BASH_BUMP+28,BASH_Y+BASH_S-BASH_BUMP-28);k->x=p==1?BASH_X+BASH_S:BASH_X;}else{k->x=clampf(k->x+dx*dt,BASH_X+BASH_BUMP+28,BASH_X+BASH_S-BASH_BUMP-28);k->y=p==0?BASH_Y:BASH_Y+BASH_S;}
      k->vx=(k->x-ox)/fmaxf(dt,.001f);k->vy=(k->y-oy)/fmaxf(dt,.001f);g->press_flash=fmaxf(0,g->press_flash-dt);if(g->human&&g->slot>=0){bool held=(input_held[g->slot]&HidNpadButton_A)!=0;if(down(g->slot)&HidNpadButton_A){g->aiming=true;g->press_time=0;g->press_flash=.15f;}if(g->aiming){g->press_time+=dt;if(!held){bool long_hold=g->press_time>=TAP_WINDOW+CONTROL_WINDOW;bool controlled=g->press_time>=TAP_WINDOW&&!long_hold;g->aiming=false;g->press_flash=0;for(int q=0;q<4;q++)if(bash_active[q]&&hypotf(bash_balls[q].x-k->x,bash_balls[q].y-k->y)<65){float nx=p==1?-1:(p==3?1:0),ny=p==0?1:(p==2?-1:0);float steer=controlled?.30f:(long_hold?.38f:.08f);float tx=nx+k->vx/700,ty=ny+k->vy/700;if(!controlled){if(p%2==0)tx+=clampf(k->vx/370*.50f,-.55f,.55f);else ty+=clampf(k->vy/370*.50f,-.55f,.55f);}tx+=-ny*clampf(dx/370*steer,-.30f,.30f);ty+=nx*clampf(dy/370*steer,-.30f,.30f);float len=hypotf(tx,ty);float power=long_hold?520.0f:(controlled?535.0f:550.0f);bash_balls[q].vx=tx/len*power+k->vx*(controlled?.10f:.20f);bash_balls[q].vy=ty/len*power+k->vy*(controlled?.10f:.20f);bash_last[q]=p;tone(180,65);}}}}

    }
    for(int p=0;p<4;p++)if(!ps[p].human){kick_wait[p]-=dt;if(kick_wait[p]<=0)for(int q=0;q<4;q++)if(bash_active[q]&&hypotf(bash_balls[q].x-bash_paddles[p].x,bash_balls[q].y-bash_paddles[p].y)<65){float nx=p==1?-1:(p==3?1:0),ny=p==0?1:(p==2?-1:0);float angle=(float)(rand()%1601-800)*.001f;float ix=nx,iy=ny;nx=ix*cosf(angle)-iy*sinf(angle);ny=ix*sinf(angle)+iy*cosf(angle);float power=(float)(270+rand()%251);bash_balls[q].vx=nx*power+bash_paddles[p].vx*.35f;bash_balls[q].vy=ny*power+bash_paddles[p].vy*.35f;bash_last[q]=p;ps[p].press_flash=.18f;kick_wait[p]=cfg.difficulty==0?.85f:(cfg.difficulty==1?.58f:.40f);tone(180,65);break;}}
    for(int id=0;id<4;id++){
      if(!bash_active[id]){bash_spawn[id]=fmaxf(0,bash_spawn[id]-dt);if(!bash_spawn[id]&&id<bash_required_balls()){bash_active[id]=true;bash_balls[id].x=BASH_X+BASH_S/2;bash_balls[id].y=BASH_Y+BASH_S/2;float angle=(rand()%6283)*.001f;bash_balls[id].vx=cosf(angle)*340;bash_balls[id].vy=sinf(angle)*340;}continue;}
      Ball*b=&bash_balls[id];int steps=(int)ceilf(dt*120);if(steps<1)steps=1;float step=dt/steps;float damp=powf(.9985f,step*60);
      for(int z=0;z<steps&&bash_active[id];z++){
        b->x+=b->vx*step;b->y+=b->vy*step;b->vx*=damp;b->vy*=damp;
        float corners[4][2]={{BASH_X,BASH_Y},{BASH_X+BASH_S,BASH_Y},{BASH_X+BASH_S,BASH_Y+BASH_S},{BASH_X,BASH_Y+BASH_S}};
        for(int c=0;c<4;c++){float dx=b->x-corners[c][0],dy=b->y-corners[c][1],d=hypotf(dx,dy),rad=BASH_BUMP+BASH_RADIUS;if(d<rad){float nx=d>.01f?dx/d:(c==0||c==3?1:-1),ny=d>.01f?dy/d:(c<2?1:-1);b->x=corners[c][0]+nx*rad;b->y=corners[c][1]+ny*rad;float v=b->vx*nx+b->vy*ny;if(v<0){b->vx-=2*v*nx;b->vy-=2*v*ny;float speed=hypotf(b->vx,b->vy),target=clampf(speed*1.45f,460,650);if(speed>.001f){b->vx*=target/speed;b->vy*=target/speed;}}}}
        int gate=-1;if(b->x<=BASH_X+2&&b->y>BASH_Y+BASH_BUMP&&b->y<BASH_Y+BASH_S-BASH_BUMP)gate=3;else if(b->x>=BASH_X+BASH_S-2&&b->y>BASH_Y+BASH_BUMP&&b->y<BASH_Y+BASH_S-BASH_BUMP)gate=1;else if(b->y<=BASH_Y+2&&b->x>BASH_X+BASH_BUMP&&b->x<BASH_X+BASH_S-BASH_BUMP)gate=0;else if(b->y>=BASH_Y+BASH_S-2&&b->x>BASH_X+BASH_BUMP&&b->x<BASH_X+BASH_S-BASH_BUMP)gate=2;
        if(gate>=0){bash_score_goal(id,gate);break;}
        for(int p=0;p<4;p++){Ball*k=&bash_paddles[p];float dx=b->x-k->x,dy=b->y-k->y,d=hypotf(dx,dy);if(d<BASH_RADIUS+27){float nx=d>.01f?dx/d:(p==0?0:(p==1?-1:(p==2?0:1))),ny=d>.01f?dy/d:(p==0?1:(p==1?0:(p==2?-1:0)));b->x=k->x+nx*(BASH_RADIUS+27);b->y=k->y+ny*(BASH_RADIUS+27);float v=(b->vx-k->vx)*nx+(b->vy-k->vy)*ny;if(v<0){b->vx-=1.75f*v*nx;b->vy-=1.75f*v*ny;b->vx+=k->vx*.36f;b->vy+=k->vy*.36f;}if(v<0||fabsf(v)<12){float outward=b->vx*nx+b->vy*ny;if(outward<500){float boost=500-outward;b->vx+=nx*boost;b->vy+=ny*boost;}bash_last[id]=p;}}
        }
        float speed=hypotf(b->vx,b->vy);if(speed>650){b->vx*=650/speed;b->vy*=650/speed;}else if(speed<245&&speed>0){b->vx*=245/speed;b->vy*=245/speed;}
      }
    }
    for(int a=0;a<4;a++)if(bash_active[a])for(int b=a+1;b<4;b++)if(bash_active[b]){Ball*x=&bash_balls[a],*y=&bash_balls[b];float dx=y->x-x->x,dy=y->y-x->y,d=hypotf(dx,dy),diam=BASH_RADIUS*2;if(d<diam){float nx=d>.001f?dx/d:1,ny=d>.001f?dy/d:0,overlap=diam-d;x->x-=nx*overlap*.5f;x->y-=ny*overlap*.5f;y->x+=nx*overlap*.5f;y->y+=ny*overlap*.5f;float rel=(y->vx-x->vx)*nx+(y->vy-x->vy)*ny;if(rel<0){x->vx+=rel*nx;x->vy+=rel*ny;y->vx-=rel*nx;y->vy-=rel*ny;}}}
    elapsed+=dt;if(elapsed>=match_seconds){bash_finish=true;results_elapsed=0;int best=-999;for(int i=0;i<4;i++)if(bash_score[i]>best){best=bash_score[i];bash_winner=i;}page=RESULTS;}
}
static void draw_bash_scorecard(SDL_Renderer*r,int i){
    const char*name=ps[i].human?human_names[ps[i].slot]:"CPU";char label[40];snprintf(label,sizeof(label),"%s  %d",tr(gate_names[i]),bash_score[i]);
    int cx=i==1||i==2?1130:150,cy=i==0||i==1?235:465,w=260,h=86,x=cx-w/2,y=cy-h/2;
    box(r,x,y,w,h,18,30,42);box(r,x,y,4,h,gate_rgb[i][0],gate_rgb[i][1],gate_rgb[i][2]);
    int adv=23;while(adv>13&&text_width(name,adv)>w-18)adv--;ink_text(r,name,cx,cy-17,adv,27,235,242,250);ink_text(r,label,cx,cy+17,25,31,gate_rgb[i][0],gate_rgb[i][1],gate_rgb[i][2]);
}static void draw_bash_game(SDL_Renderer*r){
    if(!bash_cache){SDL_Surface*surface=SDL_CreateRGBSurfaceWithFormat(0,SW,SH,32,SDL_PIXELFORMAT_RGBA32);if(surface){SDL_Renderer*tmp=SDL_CreateSoftwareRenderer(surface);if(tmp){draw_bash_pitch(tmp);SDL_RenderPresent(tmp);SDL_DestroyRenderer(tmp);bash_cache=SDL_CreateTextureFromSurface(r,surface);}SDL_FreeSurface(surface);}}if(bash_cache)SDL_RenderCopy(r,bash_cache,NULL,NULL);else draw_bash_pitch(r);for(int i=0;i<4;i++){
        Ball*k=&bash_paddles[i];Player*g=&ps[i];SDL_SetRenderDrawColor(r,gate_rgb[i][0],gate_rgb[i][1],gate_rgb[i][2],255);
        circle(r,(int)k->x,(int)k->y,26);
        draw_bash_scorecard(r,i);if((g->aiming&&g->press_time<TAP_WINDOW+CONTROL_WINDOW)||g->press_flash>0){SDL_SetRenderDrawColor(r,255,255,255,255);ring(r,(int)k->x,(int)k->y,32,3);}

    }
    for(int id=0;id<4;id++)if(bash_active[id]){SDL_SetRenderDrawColor(r,245,248,252,255);circle(r,(int)bash_balls[id].x,(int)bash_balls[id].y,BASH_RADIUS);}
    char clock[12];int left=(int)ceilf(fmaxf(0,match_seconds-elapsed));snprintf(clock,sizeof(clock),"%02d:%02d",left/60,left%60);
    ink_text(r,"TIEMPO",155,77,23,29,150,175,193);ink_text(r,clock,155,117,38,47,left<=15?255:245,left<=15?75:248,left<=15?85:252);
    box(r,77,150,156,3,38,54,68);box(r,77,150,(int)(156*clampf((float)left/fmaxf(1,match_seconds),0,1)),3,left<=15?255:85,left<=15?75:219,left<=15?85:191);
    for(int i=0;i<4;i++)if(bash_goal_flash[i]>0){
        bash_goal_flash[i]=fmaxf(0,bash_goal_flash[i]-.016f);int author=bash_goal_scorer[i],color=author>=0&&author<4?author:i,cx=color==1||color==2?1130:150,cy=color<2?311:541;
        box(r,cx-130,cy-23,260,86,13,24,34);ink_text(r,"¡GOL!",cx,cy,42,51,gate_rgb[color][0],gate_rgb[color][1],gate_rgb[color][2]);
        if(author>=0&&author<4){const char*name=ps[author].human?human_names[ps[author].slot]:"CPU";int adv=27;while(text_width(name,adv)>240&&adv>14)adv--;ink_text(r,name,cx,cy+39,adv,32,238,245,252);}
    }
    draw_countdown(r);if(bash_pause){box(r,275,255,730,300,10,20,30);centered(r,"PAUSA",290,38,48,255,255,255);row(r,"VOLVER",355,bash_pause_row==0);row(r,"REINICIAR",415,bash_pause_row==1);row(r,"SALIR A EQUIPOS",475,bash_pause_row==2);bool prior=paused;paused=true;control_legend(r);paused=prior;}SDL_RenderPresent(r);
}
static void bash_results(SDL_Renderer*r){
    panel(r,"Cuatro porterías","Resultado del partido");
    box(r,180,173,920,50,17,28,41);
    text(r,"JUGADOR",221,185,21,27,165,188,210);ink_text(r,"GF",713,198,21,27,165,188,210);ink_text(r,"GC",843,198,21,27,165,188,210);ink_text(r,"PUNTOS",1000,198,21,27,165,188,210);
    for(int i=0;i<4;i++){
        int y=237+i*73;box(r,180,y,920,62,21,32,46);box(r,180,y,6,62,gate_rgb[i][0],gate_rgb[i][1],gate_rgb[i][2]);
        const char*name=ps[i].human?human_names[ps[i].slot]:"CPU";int adv=27;while(text_width(name,adv)>370&&adv>16)adv--;
        text(r,name,220,y+4,adv,32,240,246,252);text(r,gate_names[i],220,y+37,16,20,gate_rgb[i][0],gate_rgb[i][1],gate_rgb[i][2]);
        char value[16];snprintf(value,sizeof(value),"%d",bash_for[i]);ink_text(r,value,713,y+31,27,33,231,239,248);snprintf(value,sizeof(value),"%d",bash_against[i]);ink_text(r,value,843,y+31,27,33,231,239,248);
        box(r,945,y+9,110,44,15,25,37);snprintf(value,sizeof(value),"%d",bash_score[i]);ink_text(r,value,1000,y+31,29,36,gate_rgb[i][0],gate_rgb[i][1],gate_rgb[i][2]);
    }
    char winner[64];int ties=0,win=bash_winner<0?0:bash_winner;for(int i=0;i<4;i++)if(bash_score[i]==bash_score[win])ties++;
    if(ties>1)snprintf(winner,sizeof(winner),tr("EMPATE: %d JUGADORES"),ties);else snprintf(winner,sizeof(winner),tr("GANA %s"),ps[win].human?human_names[ps[win].slot]:"CPU");
    centered(r,winner,555,32,41,gate_rgb[win][0],gate_rgb[win][1],gate_rgb[win][2]);
    int humans=connected_players(),need=humans?(humans+1)/2:1,votes=0;for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])&&(vote_mask&(1u<<i)))votes++;
    char voting[80];if(results_elapsed<5)snprintf(voting,sizeof(voting),english_ui?"REMATCH IN %d":"REVANCHA EN %d",(int)ceilf(5-results_elapsed));else snprintf(voting,sizeof(voting),english_ui?"Rematch: %d / %d votes":"Revancha: %d / %d votos",votes,need);centered(r,voting,611,30,38,255,220,112);control_legend(r);SDL_RenderPresent(r);
}

static void update_pages(SDL_Renderer*r){
    u64 b=0;for(int nav=0;nav<MAX_PADS;nav++)if(padIsConnected(&pads[nav])&&(page==MENU||page==TEAM_SELECT||(page==OPTIONS&&!paused_options)||side[nav])&&navigation(nav)){b=navigation(nav);break;}
    if(b)tone(600,60);
    if(page==MENU){
        if(b&(HidNpadButton_Up|HidNpadButton_Down))menu_row=1-menu_row;
        if(b&HidNpadButton_A){reset_lobby_ready();page=menu_row?OPTIONS:TEAM_SELECT;}
        if(b&(HidNpadButton_Plus|HidNpadButton_Minus))running=false;
        draw_menu(r);
    } else if(page==OPTIONS){
        if(b&HidNpadButton_Up)option_row=(option_row+4)%5;
        if(b&HidNpadButton_Down)option_row=(option_row+1)%5;
        if(option_row<4&&(b&(HidNpadButton_Left|HidNpadButton_Right|HidNpadButton_A))){
            if(option_row==0)cfg.volume=cfg.volume?0:65;
            if(option_row==1)cfg.effects=!cfg.effects;
            if(option_row==2)cfg.names=!cfg.names;
            if(option_row==3)cfg.music=!cfg.music;
            
            if(audio&&!cfg.volume)SDL_ClearQueuedAudio(audio);
            save_cfg();
        }
        if((b&HidNpadButton_B)||((b&HidNpadButton_A)&&option_row==4)){page=paused_options?MATCH:MENU;paused_options=false;}        draw_options(r);
    } else if(page==TEAM_SELECT){
        if(pair_in_lobby()){draw_team_select(r);return;}
        for(int i=0;i<MAX_PADS;i++){if(!padIsConnected(&pads[i])){confirmed[i]=0;continue;}u64 k=navigation(i);if(k&HidNpadButton_Y){connect_controllers();return;}if(k&HidNpadButton_Minus){reset_lobby_ready();save_people();page=MENU;return;}if(k&HidNpadButton_B){if(confirmed[i])confirmed[i]=0;else if(side[i])side[i]=0;else{reset_lobby_ready();save_people();page=MENU;return;}continue;}
        if(!confirmed[i]){if(k&HidNpadButton_Left){int dest=side[i]>-1?side[i]-1:-1;if(dest==0||dest==side[i]||human_count(dest)<MAX_TEAM)side[i]=dest;}if(k&HidNpadButton_Right){int dest=side[i]<1?side[i]+1:1;if(dest==0||dest==side[i]||human_count(dest)<MAX_TEAM)side[i]=dest;}if((k&HidNpadButton_A)&&side[i])confirmed[i]=++order_counter;}}
        cfg.bh=human_count(1);cfg.rh=human_count(-1);u32 m=requested_ready_mask(),ok=0;for(int i=0;i<MAX_PADS;i++)if(confirmed[i])ok|=1u<<i;
        if(m&&(m&ok)==m){save_cfg();save_people();page=ALL_READY;}draw_team_select(r);
    } else if(page==ALL_READY){
        for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])){u64 k=navigation(i);if(k&HidNpadButton_X){edit_person(i);break;}if((k&HidNpadButton_Y)&&captain(side[i])==i){name_entry(side[i]>0);break;}if((k&HidNpadButton_R)&&captain(side[i])==i&&human_count(-side[i])==0){name_entry(side[i]<0);break;}if(k&HidNpadButton_A){page=MATCH_CONFIG;config_row=0;}if(k&HidNpadButton_B){reset_lobby_ready();page=TEAM_SELECT;}}
        draw_all_ready(r);    } else if(page==MATCH_CONFIG){
        if(b&HidNpadButton_Up)config_row=(config_row+4)%5;
        if(b&HidNpadButton_Down)config_row=(config_row+1)%5;
        int d=(b&HidNpadButton_Right)?1:((b&HidNpadButton_Left)?-1:0);
        if(d&&config_row==0)cfg.arena=(cfg.arena+5+d)%5;
        if(d&&config_row==1)cfg.duration=(cfg.duration+3+d)%3;
        if(d&&config_row==2)cfg.difficulty=(cfg.difficulty+3+d)%3;
        if(d)save_cfg();
        if(d&&config_row==3&&cfg.arena!=BASH)cfg.size=(int)clampf(cfg.size+d,1,4);
        if((b&HidNpadButton_A)&&config_row==4&&(cfg.arena==BASH?human_count(1)+human_count(-1)<=4:cfg.size>=human_count(1)&&cfg.size>=human_count(-1)))begin_match();
        if(b&HidNpadButton_B)page=ALL_READY;
        draw_match_config(r);
    }
}
// Keep recent stalls in memory; write only after leaving the game loop.
typedef struct {double total,audio;int screen,arena;} FrameStall;
static FrameStall frame_stalls[128];static unsigned stall_count;
static void save_frame_stalls(void){
    if(!sd_ready||!stall_count)return;
    FILE*f=fopen("sdmc:/switch/HaxBall-NX/performance.csv","w");if(!f)return;
    fprintf(f,"frame_ms,audio_ms,screen,arena\n");
    unsigned first=stall_count>128?stall_count-128:0;
    for(unsigned i=first;i<stall_count;i++){FrameStall*p=&frame_stalls[i%128];fprintf(f,"%.3f,%.3f,%d,%d\n",p->total,p->audio,p->screen,p->arena);}fclose(f);
}
int main(int argc,char**argv){(void)argc;(void)argv;padConfigureInput(EXTERNAL_PADS,HidNpadStyleSet_NpadStandard);detect_console();hidSetNpadJoyHoldType(HidNpadJoyHoldType_Horizontal);for(int i=0;i<MAX_PADS;i++){u64 m=1ULL<<(i==EXTERNAL_PADS?HidNpadIdType_Handheld:i);padInitializeWithMask(&pads[i],m);}if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO)!=0)return 1;SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");SDL_Window*w=SDL_CreateWindow("HaxBall NX",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,SW,SH,SDL_WINDOW_FULLSCREEN);if(!w){SDL_Quit();return 1;}SDL_Renderer*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!r)r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);ui_renderer=r;if(!r){SDL_DestroyWindow(w);SDL_Quit();return 1;}SDL_RenderSetLogicalSize(r,SW,SH);warm_round_masks(r);padConfigureInput(EXTERNAL_PADS,HidNpadStyleSet_NpadStandard);hidSetNpadJoyHoldType(HidNpadJoyHoldType_Horizontal);u64 language_code=0;SetLanguage system_language=SetLanguage_ENUS;if(R_SUCCEEDED(setInitialize())){if(R_SUCCEEDED(setGetSystemLanguage(&language_code)))setMakeLanguage(language_code,&system_language);setExit();}english_ui=system_language!=SetLanguage_ES&&system_language!=SetLanguage_ES419;font=load_font(r);SDL_Surface*digits=SDL_LoadBMP_RW(SDL_RWFromConstMem(COUNTDOWN_BMP,sizeof(COUNTDOWN_BMP)),1);if(digits){SDL_SetColorKey(digits,SDL_TRUE,SDL_MapRGB(digits->format,0,0,0));countdown_texture=SDL_CreateTextureFromSurface(r,digits);SDL_FreeSurface(digits);}load_controller_icons(r);load_cfg();srand((unsigned)SDL_GetPerformanceCounter());for(int i=0;i<MAX_PADS;i++)snprintf(human_names[i],16,english_ui?"Player %d":"Jugador %d",i+1);FILE*pf=fopen("sdmc:/switch/HaxBall-NX/players.cfg","r");if(pf){for(int i=0;i<MAX_PADS;i++){int v=0;if(fscanf(pf,"%d\n",&v)!=1)break;side[i]=v>=-1&&v<=1?v:0;read_saved_name(pf,human_names[i]);char automatic_name[24];snprintf(automatic_name,sizeof(automatic_name),english_ui?"Jugador %d":"Player %d",i+1);if(!strcmp(human_names[i],automatic_name))snprintf(human_names[i],16,english_ui?"Player %d":"Jugador %d",i+1);}fclose(pf);}SDL_AudioSpec want;SDL_zero(want);want.freq=22050;want.format=AUDIO_S16SYS;want.channels=1;want.samples=512;audio=SDL_OpenAudioDevice(NULL,0,&want,NULL,0);if(audio)SDL_PauseAudioDevice(audio,0);Uint64 previous=SDL_GetPerformanceCounter();const double timer_frequency=(double)SDL_GetPerformanceFrequency();
    while(running&&appletMainLoop()){Uint64 frame_start=SDL_GetPerformanceCounter();double frequency=timer_frequency;for(int i=0;i<MAX_PADS;i++)padUpdate(&pads[i]);Uint64 now=SDL_GetPerformanceCounter();float dt=clampf((float)((now-previous)/timer_frequency),0,.04f);previous=now;update_input(dt);Uint64 audio_start=SDL_GetPerformanceCounter();pump_audio();double audio_ms=(SDL_GetPerformanceCounter()-audio_start)*1000.0/frequency;kick_flash=fmaxf(0,kick_flash-dt);if(page<=MATCH_CONFIG){update_pages(r);SDL_RenderPresent(r);}else if(page==MATCH){update_match(dt);if(page==RESULTS){results_elapsed=0;vote_mask=0;}int left=(int)ceilf(fmaxf(0,(float)match_seconds-elapsed));draw_game(r,left);}else if(page==RESULTS){results_elapsed+=dt;draw_results(r);for(int i=0;i<MAX_PADS;i++)if(results_elapsed>=5&&side[i]&&padIsConnected(&pads[i])&&(down(i)&HidNpadButton_B)){reset_lobby_ready();page=TEAM_SELECT;break;}for(int i=0;i<MAX_PADS;i++)if(results_elapsed>=5&&side[i]&&padIsConnected(&pads[i])&&(down(i)&HidNpadButton_A))vote_mask|=1u<<i;int n=connected_players(),need=n?(n+1)/2:1,v=0;for(int i=0;i<MAX_PADS;i++)if(side[i]&&padIsConnected(&pads[i])&&(vote_mask&(1u<<i)))v++;if(page==RESULTS&&results_elapsed>=5&&v>=need)begin_match();}double frame_ms=(SDL_GetPerformanceCounter()-frame_start)*1000.0/frequency;if(frame_ms>25.0)frame_stalls[stall_count++%128]=(FrameStall){frame_ms,audio_ms,page,cfg.arena};}
    save_frame_stalls();stop_rumble();save_cfg();if(audio)SDL_CloseAudioDevice(audio);for(int i=0;i<4;i++)if(controller_textures[i])SDL_DestroyTexture(controller_textures[i]);if(countdown_texture)SDL_DestroyTexture(countdown_texture);if(pitch_cache)SDL_DestroyTexture(pitch_cache);for(int team=0;team<2;team++)for(int phase=0;phase<2;phase++)if(crowd_poses[team][phase])SDL_DestroyTexture(crowd_poses[team][phase]);for(int rad=0;rad<96;rad++){if(disk_masks[rad])SDL_DestroyTexture(disk_masks[rad]);for(int t=1;t<5;t++)if(ring_masks[rad][t])SDL_DestroyTexture(ring_masks[rad][t]);}if(bash_cache)SDL_DestroyTexture(bash_cache);if(font)SDL_DestroyTexture(font);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;}











