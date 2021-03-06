/*******************************************************************************
* Function Name  : main
* Description    : Driver for LCD HY28A-LCDB using:
*                  ILI9320 for LCD & ADS7843 for Touch Panel
* Input          : None
* Output         : None
* Return         : None
* Compile/link   : gcc -o fblcd -lrt main.c -lbcm2835 -lqdbmp -lm -mfloat-abi=hard -Wall
* Execute        : sudo ./fblcd /dev/fb1 /dev/input/event2
*******************************************************************************/
/* Includes */
#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/input.h>
#include <termios.h>
#include "fonts.h"
#include "qdbmp.h"


/* Defines */
#define White 0xFFFF
#define Black 0x0000
#define Grey 0xF7DE
#define Blue 0x001F
#define Blue2 0x051F
#define Red 0xF800
#define Magenta 0xF81F
#define Green 0x07E0
#define Cyan 0x7FFF
#define Yellow 0xFFE0

#define TRUE 1
#define FALSE 0

#define RGB565CONVERT(red, green, blue)\
(unsigned short)( (( red   >> 3 ) << 11 ) | \
(( green >> 2 ) << 5  ) | \
( blue  >> 3 ))

#define THRESHOLD 2   /* threshold */

#ifndef EV_SYN
#define EV_SYN 0
#endif

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)


/* Types */
typedef enum { DISABLE = 0, ENABLE = !DISABLE } FunctionalState;

typedef	struct POINT
{
   unsigned short x;
   unsigned short y;
} Coordinate;

typedef struct Matrix
{
long double An,
            Bn,
            Cn,
            Dn,
            En,
            Fn,
            Divider;
} Matrix;

typedef struct Button
{
unsigned short exist,
               x0,
               y0,
               x1,
               y1,
               xo,
               yo,
               col,
               fcol,
               pressed;
char		   text[50];
} Button;


/* Global variables */
char *events[EV_MAX + 1] = {
        [0 ... EV_MAX] = NULL,
        [EV_SYN] = "Sync",                      [EV_KEY] = "Key",
        [EV_REL] = "Relative",                  [EV_ABS] = "Absolute",
        [EV_MSC] = "Misc",                      [EV_LED] = "LED",
        [EV_SND] = "Sound",                     [EV_REP] = "Repeat",
        [EV_FF] = "ForceFeedback",              [EV_PWR] = "Power",
        [EV_FF_STATUS] = "ForceFeedbackStatus",
};

char *keys[KEY_MAX + 1] = {
        [0 ... KEY_MAX] = NULL,
        [KEY_RESERVED] = "Reserved",            [KEY_ESC] = "Esc",
        [KEY_1] = "1",                          [KEY_2] = "2",
        [KEY_3] = "3",                          [KEY_4] = "4",
        [KEY_5] = "5",                          [KEY_6] = "6",
        [KEY_7] = "7",                          [KEY_8] = "8",
        [KEY_9] = "9",                          [KEY_0] = "0",
        [KEY_MINUS] = "Minus",                  [KEY_EQUAL] = "Equal",
        [KEY_BACKSPACE] = "Backspace",          [KEY_TAB] = "Tab",
        [KEY_Q] = "Q",                          [KEY_W] = "W",
        [KEY_E] = "E",                          [KEY_R] = "R",
        [KEY_T] = "T",                          [KEY_Y] = "Y",
        [KEY_U] = "U",                          [KEY_I] = "I",
        [KEY_O] = "O",                          [KEY_P] = "P",
        [KEY_LEFTBRACE] = "LeftBrace",          [KEY_RIGHTBRACE] = "RightBrace",
        [KEY_ENTER] = "Enter",                  [KEY_LEFTCTRL] = "LeftControl",
        [KEY_A] = "A",                          [KEY_S] = "S",
        [KEY_D] = "D",                          [KEY_F] = "F",
        [KEY_G] = "G",                          [KEY_H] = "H",
        [KEY_J] = "J",                          [KEY_K] = "K",
        [KEY_L] = "L",                          [KEY_SEMICOLON] = "Semicolon",
        [KEY_APOSTROPHE] = "Apostrophe",        [KEY_GRAVE] = "Grave",
        [KEY_LEFTSHIFT] = "LeftShift",          [KEY_BACKSLASH] = "BackSlash",
        [KEY_Z] = "Z",                          [KEY_X] = "X",
        [KEY_C] = "C",                          [KEY_V] = "V",
        [KEY_B] = "B",                          [KEY_N] = "N",
        [KEY_M] = "M",                          [KEY_COMMA] = "Comma",
        [KEY_DOT] = "Dot",                      [KEY_SLASH] = "Slash",
        [KEY_RIGHTSHIFT] = "RightShift",        [KEY_KPASTERISK] = "KPAsterisk",
        [KEY_LEFTALT] = "LeftAlt",              [KEY_SPACE] = "Space",
        [KEY_CAPSLOCK] = "CapsLock",            [KEY_F1] = "F1",
        [KEY_F2] = "F2",                        [KEY_F3] = "F3",
        [KEY_F4] = "F4",                        [KEY_F5] = "F5",
        [KEY_F6] = "F6",                        [KEY_F7] = "F7",
        [KEY_F8] = "F8",                        [KEY_F9] = "F9",
        [KEY_F10] = "F10",                      [KEY_NUMLOCK] = "NumLock",
        [KEY_SCROLLLOCK] = "ScrollLock",        [KEY_KP7] = "KP7",
        [KEY_KP8] = "KP8",                      [KEY_KP9] = "KP9",
        [KEY_KPMINUS] = "KPMinus",              [KEY_KP4] = "KP4",
        [KEY_KP5] = "KP5",                      [KEY_KP6] = "KP6",
        [KEY_KPPLUS] = "KPPlus",                [KEY_KP1] = "KP1",
        [KEY_KP2] = "KP2",                      [KEY_KP3] = "KP3",
        [KEY_KP0] = "KP0",                      [KEY_KPDOT] = "KPDot",
        [KEY_ZENKAKUHANKAKU] = "Zenkaku/Hankaku", [KEY_102ND] = "102nd",
        [KEY_F11] = "F11",                      [KEY_F12] = "F12",
        [KEY_RO] = "RO",                        [KEY_KATAKANA] = "Katakana",
        [KEY_HIRAGANA] = "HIRAGANA",            [KEY_HENKAN] = "Henkan",
        [KEY_KATAKANAHIRAGANA] = "Katakana/Hiragana", [KEY_MUHENKAN] = "Muhenkan",
        [KEY_KPJPCOMMA] = "KPJpComma",          [KEY_KPENTER] = "KPEnter",
        [KEY_RIGHTCTRL] = "RightCtrl",          [KEY_KPSLASH] = "KPSlash",
        [KEY_SYSRQ] = "SysRq",                  [KEY_RIGHTALT] = "RightAlt",
        [KEY_LINEFEED] = "LineFeed",            [KEY_HOME] = "Home",
        [KEY_UP] = "Up",                        [KEY_PAGEUP] = "PageUp",
        [KEY_LEFT] = "Left",                    [KEY_RIGHT] = "Right",
        [KEY_END] = "End",                      [KEY_DOWN] = "Down",
        [KEY_PAGEDOWN] = "PageDown",            [KEY_INSERT] = "Insert",
        [KEY_DELETE] = "Delete",                [KEY_MACRO] = "Macro",
        [KEY_MUTE] = "Mute",                    [KEY_VOLUMEDOWN] = "VolumeDown",
        [KEY_VOLUMEUP] = "VolumeUp",            [KEY_POWER] = "Power",
        [KEY_KPEQUAL] = "KPEqual",              [KEY_KPPLUSMINUS] = "KPPlusMinus",
        [KEY_PAUSE] = "Pause",                  [KEY_KPCOMMA] = "KPComma",
        [KEY_HANGUEL] = "Hanguel",              [KEY_HANJA] = "Hanja",
        [KEY_YEN] = "Yen",                      [KEY_LEFTMETA] = "LeftMeta",
        [KEY_RIGHTMETA] = "RightMeta",          [KEY_COMPOSE] = "Compose",
        [KEY_STOP] = "Stop",                    [KEY_AGAIN] = "Again",
        [KEY_PROPS] = "Props",                  [KEY_UNDO] = "Undo",
        [KEY_FRONT] = "Front",                  [KEY_COPY] = "Copy",
        [KEY_OPEN] = "Open",                    [KEY_PASTE] = "Paste",
        [KEY_FIND] = "Find",                    [KEY_CUT] = "Cut",
        [KEY_HELP] = "Help",                    [KEY_MENU] = "Menu",
        [KEY_CALC] = "Calc",                    [KEY_SETUP] = "Setup",
        [KEY_SLEEP] = "Sleep",                  [KEY_WAKEUP] = "WakeUp",
        [KEY_FILE] = "File",                    [KEY_SENDFILE] = "SendFile",
        [KEY_DELETEFILE] = "DeleteFile",        [KEY_XFER] = "X-fer",
        [KEY_PROG1] = "Prog1",                  [KEY_PROG2] = "Prog2",
        [KEY_WWW] = "WWW",                      [KEY_MSDOS] = "MSDOS",
        [KEY_COFFEE] = "Coffee",                [KEY_DIRECTION] = "Direction",
        [KEY_CYCLEWINDOWS] = "CycleWindows",    [KEY_MAIL] = "Mail",
        [KEY_BOOKMARKS] = "Bookmarks",          [KEY_COMPUTER] = "Computer",
        [KEY_BACK] = "Back",                    [KEY_FORWARD] = "Forward",
        [KEY_CLOSECD] = "CloseCD",              [KEY_EJECTCD] = "EjectCD",
        [KEY_EJECTCLOSECD] = "EjectCloseCD",    [KEY_NEXTSONG] = "NextSong",
        [KEY_PLAYPAUSE] = "PlayPause",          [KEY_PREVIOUSSONG] = "PreviousSong",
        [KEY_STOPCD] = "StopCD",                [KEY_RECORD] = "Record",
        [KEY_REWIND] = "Rewind",                [KEY_PHONE] = "Phone",
        [KEY_ISO] = "ISOKey",                   [KEY_CONFIG] = "Config",
        [KEY_HOMEPAGE] = "HomePage",            [KEY_REFRESH] = "Refresh",
        [KEY_EXIT] = "Exit",                    [KEY_MOVE] = "Move",
        [KEY_EDIT] = "Edit",                    [KEY_SCROLLUP] = "ScrollUp",
        [KEY_SCROLLDOWN] = "ScrollDown",        [KEY_KPLEFTPAREN] = "KPLeftParenthesis",
        [KEY_KPRIGHTPAREN] = "KPRightParenthesis", [KEY_F13] = "F13",
        [KEY_F14] = "F14",                      [KEY_F15] = "F15",
        [KEY_F16] = "F16",                      [KEY_F17] = "F17",
        [KEY_F18] = "F18",                      [KEY_F19] = "F19",
        [KEY_F20] = "F20",                      [KEY_F21] = "F21",
        [KEY_F22] = "F22",                      [KEY_F23] = "F23",
        [KEY_F24] = "F24",                      [KEY_PLAYCD] = "PlayCD",
        [KEY_PAUSECD] = "PauseCD",              [KEY_PROG3] = "Prog3",
        [KEY_PROG4] = "Prog4",                  [KEY_SUSPEND] = "Suspend",
        [KEY_CLOSE] = "Close",                  [KEY_PLAY] = "Play",
        [KEY_FASTFORWARD] = "Fast Forward",     [KEY_BASSBOOST] = "Bass Boost",
        [KEY_PRINT] = "Print",                  [KEY_HP] = "HP",
        [KEY_CAMERA] = "Camera",                [KEY_SOUND] = "Sound",
        [KEY_QUESTION] = "Question",            [KEY_EMAIL] = "Email",
        [KEY_CHAT] = "Chat",                    [KEY_SEARCH] = "Search",
        [KEY_CONNECT] = "Connect",              [KEY_FINANCE] = "Finance",
        [KEY_SPORT] = "Sport",                  [KEY_SHOP] = "Shop",
        [KEY_ALTERASE] = "Alternate Erase",     [KEY_CANCEL] = "Cancel",
        [KEY_BRIGHTNESSDOWN] = "Brightness down", [KEY_BRIGHTNESSUP] = "Brightness up",
        [KEY_MEDIA] = "Media",                  [KEY_UNKNOWN] = "Unknown",
        [BTN_0] = "Btn0",                       [BTN_1] = "Btn1",
        [BTN_2] = "Btn2",                       [BTN_3] = "Btn3",
        [BTN_4] = "Btn4",                       [BTN_5] = "Btn5",
        [BTN_6] = "Btn6",                       [BTN_7] = "Btn7",
        [BTN_8] = "Btn8",                       [BTN_9] = "Btn9",
        [BTN_LEFT] = "LeftBtn",                 [BTN_RIGHT] = "RightBtn",
        [BTN_MIDDLE] = "MiddleBtn",             [BTN_SIDE] = "SideBtn",
        [BTN_EXTRA] = "ExtraBtn",               [BTN_FORWARD] = "ForwardBtn",
        [BTN_BACK] = "BackBtn",                 [BTN_TASK] = "TaskBtn",
        [BTN_TRIGGER] = "Trigger",              [BTN_THUMB] = "ThumbBtn",
        [BTN_THUMB2] = "ThumbBtn2",             [BTN_TOP] = "TopBtn",
        [BTN_TOP2] = "TopBtn2",                 [BTN_PINKIE] = "PinkieBtn",
        [BTN_BASE] = "BaseBtn",                 [BTN_BASE2] = "BaseBtn2",
        [BTN_BASE3] = "BaseBtn3",               [BTN_BASE4] = "BaseBtn4",
        [BTN_BASE5] = "BaseBtn5",               [BTN_BASE6] = "BaseBtn6",
        [BTN_DEAD] = "BtnDead",                 [BTN_A] = "BtnA",
        [BTN_B] = "BtnB",                       [BTN_C] = "BtnC",
        [BTN_X] = "BtnX",                       [BTN_Y] = "BtnY",
        [BTN_Z] = "BtnZ",                       [BTN_TL] = "BtnTL",
        [BTN_TR] = "BtnTR",                     [BTN_TL2] = "BtnTL2",
        [BTN_TR2] = "BtnTR2",                   [BTN_SELECT] = "BtnSelect",
        [BTN_START] = "BtnStart",               [BTN_MODE] = "BtnMode",
        [BTN_THUMBL] = "BtnThumbL",             [BTN_THUMBR] = "BtnThumbR",
        [BTN_TOOL_PEN] = "ToolPen",             [BTN_TOOL_RUBBER] = "ToolRubber",
        [BTN_TOOL_BRUSH] = "ToolBrush",         [BTN_TOOL_PENCIL] = "ToolPencil",
        [BTN_TOOL_AIRBRUSH] = "ToolAirbrush",   [BTN_TOOL_FINGER] = "ToolFinger",
        [BTN_TOOL_MOUSE] = "ToolMouse",         [BTN_TOOL_LENS] = "ToolLens",
        [BTN_TOUCH] = "Touch",                  [BTN_STYLUS] = "Stylus",
        [BTN_STYLUS2] = "Stylus2",              [BTN_TOOL_DOUBLETAP] = "Tool Doubletap",
        [BTN_TOOL_TRIPLETAP] = "Tool Tripletap", [BTN_GEAR_DOWN] = "WheelBtn",
        [BTN_GEAR_UP] = "Gear up",              [KEY_OK] = "Ok",
        [KEY_SELECT] = "Select",                [KEY_GOTO] = "Goto",
        [KEY_CLEAR] = "Clear",                  [KEY_POWER2] = "Power2",
        [KEY_OPTION] = "Option",                [KEY_INFO] = "Info",
        [KEY_TIME] = "Time",                    [KEY_VENDOR] = "Vendor",
        [KEY_ARCHIVE] = "Archive",              [KEY_PROGRAM] = "Program",
        [KEY_CHANNEL] = "Channel",              [KEY_FAVORITES] = "Favorites",
        [KEY_EPG] = "EPG",                      [KEY_PVR] = "PVR",
        [KEY_MHP] = "MHP",                      [KEY_LANGUAGE] = "Language",
        [KEY_TITLE] = "Title",                  [KEY_SUBTITLE] = "Subtitle",
        [KEY_ANGLE] = "Angle",                  [KEY_ZOOM] = "Zoom",
        [KEY_MODE] = "Mode",                    [KEY_KEYBOARD] = "Keyboard",
        [KEY_SCREEN] = "Screen",                [KEY_PC] = "PC",
        [KEY_TV] = "TV",                        [KEY_TV2] = "TV2",
        [KEY_VCR] = "VCR",                      [KEY_VCR2] = "VCR2",
        [KEY_SAT] = "Sat",                      [KEY_SAT2] = "Sat2",
        [KEY_CD] = "CD",                        [KEY_TAPE] = "Tape",
        [KEY_RADIO] = "Radio",                  [KEY_TUNER] = "Tuner",
        [KEY_PLAYER] = "Player",                [KEY_TEXT] = "Text",
        [KEY_DVD] = "DVD",                      [KEY_AUX] = "Aux",
        [KEY_MP3] = "MP3",                      [KEY_AUDIO] = "Audio",
        [KEY_VIDEO] = "Video",                  [KEY_DIRECTORY] = "Directory",
        [KEY_LIST] = "List",                    [KEY_MEMO] = "Memo",
        [KEY_CALENDAR] = "Calendar",            [KEY_RED] = "Red",
        [KEY_GREEN] = "Green",                  [KEY_YELLOW] = "Yellow",
        [KEY_BLUE] = "Blue",                    [KEY_CHANNELUP] = "ChannelUp",
        [KEY_CHANNELDOWN] = "ChannelDown",      [KEY_FIRST] = "First",
        [KEY_LAST] = "Last",                    [KEY_AB] = "AB",
        [KEY_NEXT] = "Next",                    [KEY_RESTART] = "Restart",
        [KEY_SLOW] = "Slow",                    [KEY_SHUFFLE] = "Shuffle",
        [KEY_BREAK] = "Break",                  [KEY_PREVIOUS] = "Previous",
        [KEY_DIGITS] = "Digits",                [KEY_TEEN] = "TEEN",
        [KEY_TWEN] = "TWEN",                    [KEY_DEL_EOL] = "Delete EOL",
        [KEY_DEL_EOS] = "Delete EOS",           [KEY_INS_LINE] = "Insert line",
        [KEY_DEL_LINE] = "Delete line",
};

char *absval[5] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat " };

char *relatives[REL_MAX + 1] = {
        [0 ... REL_MAX] = NULL,
        [REL_X] = "X",                  [REL_Y] = "Y",
        [REL_Z] = "Z",                  [REL_HWHEEL] = "HWheel",
        [REL_DIAL] = "Dial",            [REL_WHEEL] = "Wheel",
        [REL_MISC] = "Misc",    
};

char *absolutes[ABS_MAX + 1] = {
        [0 ... ABS_MAX] = NULL,
        [ABS_X] = "X",                  [ABS_Y] = "Y",
        [ABS_Z] = "Z",                  [ABS_RX] = "Rx",
        [ABS_RY] = "Ry",                [ABS_RZ] = "Rz",
        [ABS_THROTTLE] = "Throttle",    [ABS_RUDDER] = "Rudder",
        [ABS_WHEEL] = "Wheel",          [ABS_GAS] = "Gas",
        [ABS_BRAKE] = "Brake",          [ABS_HAT0X] = "Hat0X",
        [ABS_HAT0Y] = "Hat0Y",          [ABS_HAT1X] = "Hat1X",
        [ABS_HAT1Y] = "Hat1Y",          [ABS_HAT2X] = "Hat2X",
        [ABS_HAT2Y] = "Hat2Y",          [ABS_HAT3X] = "Hat3X",
        [ABS_HAT3Y] = "Hat 3Y",         [ABS_PRESSURE] = "Pressure",
        [ABS_DISTANCE] = "Distance",    [ABS_TILT_X] = "XTilt",
        [ABS_TILT_Y] = "YTilt",         [ABS_TOOL_WIDTH] = "Tool Width",
        [ABS_VOLUME] = "Volume",        [ABS_MISC] = "Misc",
};

char *misc[MSC_MAX + 1] = {
        [ 0 ... MSC_MAX] = NULL,
        [MSC_SERIAL] = "Serial",        [MSC_PULSELED] = "Pulseled",
        [MSC_GESTURE] = "Gesture",      [MSC_RAW] = "RawData",
        [MSC_SCAN] = "ScanCode",
};

char *leds[LED_MAX + 1] = {
        [0 ... LED_MAX] = NULL,
        [LED_NUML] = "NumLock",         [LED_CAPSL] = "CapsLock",
        [LED_SCROLLL] = "ScrollLock",   [LED_COMPOSE] = "Compose",
        [LED_KANA] = "Kana",            [LED_SLEEP] = "Sleep",
        [LED_SUSPEND] = "Suspend",      [LED_MUTE] = "Mute",
        [LED_MISC] = "Misc",
};

char *repeats[REP_MAX + 1] = {
        [0 ... REP_MAX] = NULL,
        [REP_DELAY] = "Delay",          [REP_PERIOD] = "Period"
};

char *sounds[SND_MAX + 1] = {
        [0 ... SND_MAX] = NULL,
        [SND_CLICK] = "Click",          [SND_BELL] = "Bell",
        [SND_TONE] = "Tone"
};

char **names[EV_MAX + 1] = {
        [0 ... EV_MAX] = NULL,
        [EV_SYN] = events,                      [EV_KEY] = keys,
        [EV_REL] = relatives,                   [EV_ABS] = absolutes,
        [EV_MSC] = misc,                        [EV_LED] = leds,
        [EV_SND] = sounds,                      [EV_REP] = repeats,
};


/* Function declarations */
Coordinate *Read_Ads7846(void);
void TP_Init(char*);
void TP_Cal(void);
int TP_Button(void);
void DrawCross(unsigned short Xpos, unsigned short Ypos);
void TP_DrawPoint(unsigned short Xpos, unsigned short Ypos);
FunctionalState setCalibrationMatrix( Coordinate * displayPtr,Coordinate * screenPtr,Matrix * matrixPtr);
FunctionalState getDisplayPoint(Coordinate * displayPtr,Coordinate * screenPtr,Matrix * matrixPtr );
Coordinate *Read_Ads7846(void);
void LCD_Init(char*);
void LCD_Button(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short, int, unsigned short, unsigned short, char*, unsigned short);
int LCD_PutImage(unsigned short, unsigned short, char*);
void LCD_Clear(unsigned short);
void LCD_Text(unsigned short, unsigned short, char *, unsigned short, unsigned short);
void PutChar(unsigned short, unsigned short, unsigned char, unsigned short, unsigned short);
int sgn(int);
void LCD_DrawLine(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short);
void LCD_DrawBox(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short, int);
void LCD_DrawCircle(unsigned short, unsigned short, unsigned short, unsigned short);
void LCD_DrawCircleFill(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short);
void LCD_SetPoint(unsigned short, unsigned short, unsigned short);
short LCD_GetPoint(unsigned short, unsigned short);
void LCD_SetCursor(unsigned short, unsigned short);
void DelayMicrosecondsNoSleep(int delay_us);
void draw(void);


/* global variables to store screen info */
char *fbp = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
static Matrix matrix;
static Coordinate display;
static Coordinate ScreenSample[3];
//static Coordinate DisplaySample[3] = { {45, 45}, {45, 270}, {190, 190} };
static Coordinate DisplaySample[3] = { {45, 45}, {45, 195}, {190, 190} };
static Coordinate Screen;
static Button Butt[20] = {
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         };

int fbfd = 0;
struct fb_var_screeninfo orig_vinfo;
long int screensize = 0;

int fd, rd, i, j, k;
struct input_event ev[64];
int version;
unsigned short id[4];
unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
char name[256] = "Unknown";
int abs1[5];

int main(int argc, char *argv[])
{
    int l;

	if (argc < 3) {
		printf("Usage: [/dev/fbX] [/dev/input/eventX]\n");
		exit(1);
	}
    
    TP_Init(argv[2]);
    LCD_Init(argv[1]);

    if ((int)fbp == -1) {
        printf("Failed to mmap\n");
    } else {
        LCD_Clear(Black);
        draw();
    }

    TP_Cal();
    if (!bcm2835_init()) printf("Error open BCM2835\n");

    while(1)
    {
        getDisplayPoint(&display, Read_Ads7846(), &matrix);

        if ( ((l = TP_Button()) != -1) )
        {
            printf("Pressed button%2d\n", l);

            switch (l) {
            case 0:
            	// your code for button 0 pressed here
                //LCD_PutImage(100, 100, "test2.bmp");
                break;
            case 1:
            	// your code for button 1 pressed here
                //LCD_PutImage(0, 0, "full_l.bmp");
				// you can also reload background & buttons
                draw();
                break;
            case 2:
            	// your code for button 1 pressed here
                //LCD_PutImage(5, 5, "foto.bmp");
                break;
            case 4: // Up
            	// your code for button 1 pressed here
                //LCD_PutImage(5, 5, "foto.bmp");
                break;
            case 5: // Down
            	// your code for button 1 pressed here
                //LCD_PutImage(5, 5, "foto.bmp");
                break;
            case 3: 
            	// your code besor exit here
				LCD_Clear(Black);
				// cleanup
				munmap(fbp, screensize);
				if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
					printf("Error re-setting variable information\n");
				}
				close(fbfd);
				close(fd);
				bcm2835_close();
				exit(0);
                break;
            default:
                // Code
                break;
            }
        }
        //TP_DrawPoint(display.x, display.y);
    }     
}


/*******************************************************************************
* Function Name  : draw
* Description    : Sub of main
* Input          : None
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void draw() 
{
    LCD_Button(260,10,55,30,Yellow,Blue,3,7,"Image",0);
    LCD_Button(260,50,55,30,Yellow,Blue,3,7,"On",1);
    LCD_Button(260,90,55,30,Yellow,Blue,3,7,"Off",2);
    LCD_Button(260,140,55,30,Yellow,Blue,3,7,"esci",3);

    LCD_Button(60,10,55,30,Yellow,Blue,3,7,"Up",4);
    LCD_Button(60,50,55,30,Yellow,Blue,3,7,"Down",5);
}


/*******************************************************************************
* Function Name  : TP_Init
* Description    : Initialize TP Controller.
* Input          : /dev/input/eventX
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void TP_Init(char* inputb)
{
    if ((fd = open(inputb, O_RDONLY)) == -1) {
        printf("Error: cannot open pointing device\n");
        exit(1);
    }
    
	if (ioctl(fd, EVIOCGVERSION, &version)) {
		printf("Error: cannot get version\n");
		exit(1);
	}

	printf("Input driver version is %d.%d.%d\n", version >> 16, (version >> 8) & 0xff, version & 0xff);

	ioctl(fd, EVIOCGID, id);
	printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n", id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Input device name: \"%s\"\n", name);

	memset(bit, 0, sizeof(bit));
	ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
	
	/*printf("Supported events:\n");

	for (i = 0; i < EV_MAX; i++) {
		if (test_bit(i, bit[0]))
		{
			printf("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
			if (!i) continue;
			ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
			for (j = 0; j < KEY_MAX; j++) 
			{
				if (test_bit(j, bit[i])) 
				{
					printf("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
					if (i == EV_ABS) 
					{
						ioctl(fd, EVIOCGABS(j), abs1);
						for (k = 0; k < 5; k++)
							if ((k < 3) || abs1[k])
								printf("      %s %6d\n", absval[k], abs1[k]);
					}
				}
			}
		}
	}*/
}


/*******************************************************************************
* Function Name  : LCD_Init
* Description    : Initialize TFT Controller.
* Input          : /dev/fbX
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void LCD_Init(char* frameb)
{
    // Open the file for reading and writing
    fbfd = open(frameb, O_RDWR);
    if (!fbfd) {
        printf("Error: cannot open framebuffer device\n");
        exit(1);
    }
    printf("The framebuffer/pointing device was opened successfully\n");

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information\n");
    }
    printf("Original %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );

    // Store for reset (copy vinfo to vinfo_orig)
    memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
    }

    // map fb to user mem
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp = (char*)mmap(0,
              screensize,
              PROT_READ | PROT_WRITE,
              MAP_SHARED,
              fbfd,
              0);
}


/*******************************************************************************
* Function Name  : TP_Button
* Description    : Return te button pressed
* Input          : None
* Output         : None
* Return         : Button pressed -1 if no button pressed
* Attention      : None
*******************************************************************************/
int TP_Button()
{
    int i;

    for (i=0; i<20; i++)
    {
        if (Butt[i].exist)
        {
            if (Butt[i].pressed == 1)
            {
               Butt[i].pressed = 0;
               return i;
            }
        }
    }
    return -1;
}


/******************************************************************************
* Function Name  : LCD_Button
* Description    : Make button
* Input          : - x0: point x from upper left corner
*                  - y0: point y from upper left corner
*                  - x1: height
*                  - y1: width
*                  - col: Line color
*                  - fcol: fill color -1 means no fill
*                  - xo: text x origin of button
*                  - yo: text y origin of button
*                  - text: the text
*                  - buttn: number of button
* Output         : None
* Return         : None
******************************************************************************/
void LCD_Button(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1, unsigned short col, int fcol, unsigned short xo, unsigned short yo, char* text, unsigned short buttn)
{
    LCD_DrawBox(x0, y0, x0 + x1, y0 + y1 , col, fcol);
    LCD_Text(x0 + xo, y0 + yo, text, col, fcol);
    Butt[buttn].exist = 1;
    Butt[buttn].x0 = x0;
    Butt[buttn].y0 = y0;
    Butt[buttn].x1 = x0 + x1;
    Butt[buttn].y1 = y0 + y1;
    Butt[buttn].xo = x0 + xo;
    Butt[buttn].yo = y0 + yo;
    Butt[buttn].col = col;
    Butt[buttn].fcol = fcol;
    strcpy(Butt[buttn].text, text);
    Butt[buttn].pressed = 0;
}


/*******************************************************************************
* Function Name  : LCD_PutImage
* Description    : Show BMP
* Input          : x upper left corner image start
*                  y upper left corner image start
*                  file filename full qualified path
* Output         : None
* Return         : None
* Attention      : The image must be 8 or 24 bits RGB (sub will convert to 16 bits)
*******************************************************************************/
int LCD_PutImage(unsigned short x, unsigned short y, char* file)
{
    UCHAR red, green, blue;
    UINT width, height;
    UINT r, c;
    BMP* bmp;

    /* Read an image file */
    bmp = BMP_ReadFile(file);
    if (BMP_GetError() != BMP_OK)
    {
       /* Print error info */
       printf( "An error has occurred: %s (code %d)\n", BMP_GetErrorDescription(), BMP_GetError() );
    }

    /* Get image's dimensions */
    width = BMP_GetWidth(bmp);
    height = BMP_GetHeight(bmp);


    /* Iterate through all the image's pixels */
    for (c=0; c<height; ++c)
    {
        for (r=0; r<width; ++r)
        {
            BMP_GetPixelRGB(bmp, r, c, &red, &green, &blue);
            LCD_SetPoint(x+r, y+c, RGB565CONVERT(red, green, blue));
        }
    }

    return 0;
}


/******************************************************************************
* Function Name  : LCD_SetPoint
* Description    : Drawn at a specified point coordinates
* Input          : - Xpos: Row Coordinate
*                  - Ypos: Line Coordinate
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void LCD_SetPoint( unsigned short x, unsigned short y, unsigned short point)
{
    if( x >= vinfo.xres || y >= vinfo.yres )
    {
        return;
    } else {
        // calculate the pixel's byte offset inside the buffer
        // note: x * 2 as every pixel is 2 consecutive bytes
        unsigned int pix_offset = x * 2 + y * finfo.line_length;

        // now this is about the same as 'fbp[pix_offset] = value'
        // but a bit more complicated for RGB565
        //unsigned short c = ((r / 8) << 11) + ((g / 4) << 5) + (b / 8);
        //unsigned short c = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8);
        // write 'two bytes at once'
        //*((unsigned short*)(fbp + pix_offset)) = c;
        *((unsigned short*)(fbp + pix_offset)) = point;
    }
}


/*******************************************************************************
* Function Name  : DelayMicrosecondsNoSleep
* Description    : Delay n microseconds
* Input          : delay_us: specifies the n microseconds
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void DelayMicrosecondsNoSleep (int delay_us)
{
    long int start_time;
    long int time_difference;
    struct timespec gettime_now;

    clock_gettime(CLOCK_REALTIME, &gettime_now);
    start_time = gettime_now.tv_nsec;		 //Get nS value

    while (1)
    {
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        time_difference = gettime_now.tv_nsec - start_time;
        if (time_difference < 0)
	    time_difference += 1000000000;	 //(Rolls over every 1 second)
	if (time_difference > (delay_us * 1000)) //Delay for # nS
	    break;
    }
}


/*******************************************************************************
* Function Name  : GetASCIICode
* Description    : get ASCII code data
* Input          : - ASCII: Input ASCII code
* Output         : - *pBuffer: Store data pointer
* Return         : None
* Attention	 	 : None
*******************************************************************************/
void GetASCIICode(unsigned char* pBuffer,unsigned char ASCII)
{
    memcpy(pBuffer,AsciiLib[(ASCII - 32)] ,16);
}


/*******************************************************************************
* Function Name  : LCD_Clear
* Description    : Fill the screen as the specified color
* Input          : - Color: Screen Color
* Output         : None
* Return         : None
* Attention	 	 : None
*******************************************************************************/
void LCD_Clear(unsigned short Color)
{
    memset((unsigned short*)fbp, Color, vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8);
}


/******************************************************************************
* Function Name  : LCD_GetPoint
* Description    : Get color value for the specified coordinates
* Input          : - Xpos: Row Coordinate
*                  - Xpos: Line Coordinate
* Output         : None
* Return         : Screen Color - -1 out of coordinate
* Attention	 	 : None
*******************************************************************************/
short LCD_GetPoint( unsigned short x, unsigned short y)
{
    if( x >= vinfo.xres || y >= vinfo.yres )
    {
        return -1;
    } else {
        // calculate the pixel's byte offset inside the buffer
        // note: x * 2 as every pixel is 2 consecutive bytes
        unsigned int pix_offset = x * 2 + y * finfo.line_length;

        // now this is about the same as 'fbp[pix_offset] = value'
        // but a bit more complicated for RGB565
        //unsigned short c = ((r / 8) << 11) + ((g / 4) << 5) + (b / 8);
        //unsigned short c = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8);
        // write 'two bytes at once'
        //*((unsigned short*)(fbp + pix_offset)) = c;
        return *((unsigned short*)(fbp + pix_offset));
    }
}


/******************************************************************************
* Function Name  : PutChar
* Description    : Lcd screen displays a character
* Input          : - Xpos: Horizontal coordinate
*                  - Ypos: Vertical coordinate
*				   - ASCI: Displayed character
*				   - charColor: Character color
*				   - bkColor: Background color
* Output         : None
* Return         : None
* Attention	 	 : None
*******************************************************************************/
void PutChar(unsigned short Xpos, unsigned short Ypos, unsigned char ASCI, unsigned short charColor, unsigned short bkColor )
{
    unsigned short i, j;
    unsigned char buffer[16], tmp_char;

    GetASCIICode(buffer,ASCI);  /* get font data */

    for( i=0; i<16; i++ )
    {
        tmp_char = buffer[i];
        for( j=0; j<8; j++ )
        {
            if( ((tmp_char >> (7 - j)) & 0x01) == 0x01 )
            {
                LCD_SetPoint( Xpos + j, Ypos + i, charColor ); /* Character color */
            }
            else
            {
                LCD_SetPoint( Xpos + j, Ypos + i, bkColor );   /* Background color */
            }
        }
    }
}


/******************************************************************************
* Function Name  : LCD_Text
* Description    : Displays the string
* Input          : - Xpos: Horizontal coordinate
*                  - Ypos: Vertical coordinate
*		   - str: Displayed string
*		   - charColor: Character color
*		   - bkColor: Background color
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void LCD_Text(unsigned short Xpos, unsigned short Ypos, char *str, unsigned short Color, unsigned short bkColor)
{
    unsigned short TempChar;
    do
    {
        TempChar = *str++;
        PutChar( Xpos, Ypos, TempChar, Color, bkColor );
        if( Xpos < vinfo.xres - 8 )
        {
            Xpos += 8;
        }
        else if ( Ypos < vinfo.yres - 16 )
        {
            Xpos = 0;
            Ypos += 16;
        }
        else
        {
            Xpos = 0;
            Ypos = 0;
        }
    }
    while ( *str != 0 );
}


/******************************************************************************
* Function Name  : sgn
* Description    : return the sign of number
* Input          : - nu: the number
* Output         : None
* Return         : 1 if > 0; -1 if < 0; 0 of = 0
* Attention      : None
*******************************************************************************/
int sgn(int nu)
{
    if (nu > 0) return 1;
    if (nu < 0) return -1;
    if (nu == 0) return 0;

    return 0;
}


/******************************************************************************
* Function Name  : LCD_DrawLine
* Description    : Bresenham's line algorithm
* Input          : - x1: A point line coordinates
*                  - y1: A point column coordinates
*                  - x2: B point line coordinates
*                  - y2: B point column coordinates
*                  - col: Line color
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void LCD_DrawLine(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short col)
{
    unsigned short n, deltax, deltay, sgndeltax, sgndeltay, deltaxabs, deltayabs, x, y, drawx, drawy;

    deltax = x2 - x1;
    deltay = y2 - y1;
    deltaxabs = abs(deltax);
    deltayabs = abs(deltay);
    sgndeltax = sgn(deltax);
    sgndeltay = sgn(deltay);
    x = deltayabs >> 1;
    y = deltaxabs >> 1;
    drawx = x1;
    drawy = y1;

    LCD_SetPoint(drawx, drawy, col);

    if (deltaxabs >= deltayabs){
        for (n = 0; n < deltaxabs; n++){
            y += deltayabs;
            if (y >= deltaxabs){
                y -= deltaxabs;
                drawy += sgndeltay;
            }
            drawx += sgndeltax;
            LCD_SetPoint(drawx, drawy, col);
        }
    } else {
        for (n = 0; n < deltayabs; n++){
            x += deltaxabs;
            if (x >= deltayabs){
                 x -= deltayabs;
                 drawx += sgndeltax;
            }
            drawy += sgndeltay;
            LCD_SetPoint(drawx, drawy, col);
        }
    }
}


/******************************************************************************
* Function Name  : LCD_DrawBox
* Description    : Multiple line  makes box
* Input          : - x1: A point line coordinates upper left corner
*                  - y1: A point column coordinates
*                  - x2: B point line coordinates lower right corner
*                  - y2: B point column coordinates
*                  - col: Line color
* Output         : None
* Return         : None
******************************************************************************/
void LCD_DrawBox(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1 , unsigned short col, int fcol )
{
    unsigned short i, xx0, xx1, yy0;

    LCD_DrawLine(x0, y0, x1, y0, col);
    LCD_DrawLine(x1, y0, x1, y1, col);
    LCD_DrawLine(x0, y0, x0, y1, col);
    LCD_DrawLine(x0, y1, x1, y1, col);

    if  (fcol!=-1)
    {
        for (i=0; i<y1-y0-1; i++)
        {
            xx0=x0+1;
            yy0=y0+1+i;
            xx1=x1-1;

            LCD_DrawLine(xx0, yy0, xx1, yy0, (unsigned short)fcol);
        }
    }
}

/******************************************************************************
* Function Name  : drawCircle
* Description    : Sub for LCD_DrawCircle
* Input          : - xc:
*                  - yc:
*                  - x:
*                  - y:
*                  - col: Line color
* Output         : None
* Return         : None
******************************************************************************/
void drawCircle(unsigned short xc, unsigned short yc, unsigned short x, unsigned short y, unsigned short col)
{
    LCD_SetPoint(xc+x, yc+y, col);
    LCD_SetPoint(xc-x, yc+y, col);
    LCD_SetPoint(xc+x, yc-y, col);
    LCD_SetPoint(xc-x, yc-y, col);
    LCD_SetPoint(xc+y, yc+x, col);
    LCD_SetPoint(xc-y, yc+x, col);
    LCD_SetPoint(xc+y, yc-x, col);
    LCD_SetPoint(xc-y, yc-x, col);
}


/******************************************************************************
* Function Name  : LCD_DrawCircle
* Description    : Draw a circle
* Input          : - xc: A point line coordinates center
*                  - yc: A point column coordinates center
*                  - r: radius of circle
*                  - col: Line color
* Output         : None
* Return         : None
******************************************************************************/
void LCD_DrawCircle(unsigned short xc, unsigned short yc, unsigned short r, unsigned short col)
{
    int x = 0, y = r;
    int p = 1 - r;

    while (x < y)
    {
        drawCircle(xc, yc, x, y, col);
        x++;

        if (p < 0)
            p = p + 2 * x + 1;
        else
        {
            y--;
            p = p + 2 * (x-y) + 1;
        }
        drawCircle(xc, yc, x, y, col);
    }
}


/******************************************************************************
* Function Name  : LCD_DrawCircleFill
* Description    : Draw a circle filled
* Input          : - xc: A point line coordinates center
*                  - yc: A point column coordinates center
*                  - r: radius of circle
*                  - bcol: border color
*                  - col: fill color
* Output         : None
* Return         : None
******************************************************************************/
void LCD_DrawCircleFill(unsigned short x, unsigned short y, unsigned short r, unsigned short bcol, unsigned short col) {
    int xc, yc;
    double testRadius;
    double rsqMin = (double)(r-1)*(r-1);
    double rsqMax = (double)r*r;

    int fillFlag = 1;

    /* Ensure radius is positive */
    if (r < 0) {
        r = -r;
    }

    for (yc = -r; yc < r; yc++) {
        for (xc = -r; xc < r; xc++) {
            testRadius = (double)(xc*xc + yc*yc);
            if (((rsqMin < testRadius)&&(testRadius <= rsqMax))
                || ((fillFlag)&&(testRadius <= rsqMax))) {
                LCD_SetPoint(x + xc, y + yc, col);
            }
        }
    }
    if (col != bcol) LCD_DrawCircle(x, y, r, bcol);
}


/*******************************************************************************
* Function Name  : TP_GetAdXY
* Description    : Read ADS7843 ADC value of X + Y + channel
* Input          : None
* Output         : None
* Return         : return X + Y + channel ADC value
* Attention	 	 : None
*******************************************************************************/
void TP_GetAdXY(int *x,int *y)
{
    int xx, yy;
    int catch = 0;
		   
	rd = read(fd, ev, sizeof(struct input_event) * 64);

	if (rd < (int) sizeof(struct input_event)) 
	{
		printf("Error reading\n");
		return;
	}

	for (i = 0; i < rd / sizeof(struct input_event); i++)
	{
        if (ev[i].type == 1 && ev[i].code == 330) catch = 1;
		if (ev[i].type == EV_SYN) 
		{
			//printf("Event: time %ld.%06ld, -------------- %s ------------\n",	ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].code ? "Config Sync" : "Report Sync" );
			//printf("x: %4u -  y: %4u\n", (unsigned int) xx, (unsigned int) yy);
			catch = 0;
		} else { 
			if (ev[i].type == EV_MSC && (ev[i].code == MSC_RAW || ev[i].code == MSC_SCAN)) {
				//printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %02x\n", ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type, events[ev[i].type] ? events[ev[i].type] : "?", ev[i].code, names[ev[i].type] ? (names[ev[i].type][ev[i].code] ? names[ev[i].type][ev[i].code] : "?") : "?", ev[i].value);
			} else {
				//printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n", ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type, events[ev[i].type] ? events[ev[i].type] : "?", ev[i].code, names[ev[i].type] ? (names[ev[i].type][ev[i].code] ? names[ev[i].type][ev[i].code] : "?") : "?", ev[i].value);
				if (catch == 1) {		
					if (ev[i].code == 0) xx = ev[i].value;
					if (ev[i].code == 1) yy = ev[i].value;
				}
			}
		}
    }
    *x = xx;
    *y = yy;
    //printf("x: %4u -  y: %4u\n", (unsigned int) xx, (unsigned int) yy);
}


/*******************************************************************************
* Function Name  : TP_DrawPoint
* Description    : Draw point Must have a LCD driver
* Input          : - Xpos: Row Coordinate
*                  - Ypos: Line Coordinate
* Output         : None
* Return         : None
* Attention	 	 : None
*******************************************************************************/
void TP_DrawPoint(unsigned short Xpos,unsigned short Ypos)
{
    LCD_SetPoint(Xpos,Ypos,0xf800);     /* Center point */
    LCD_SetPoint(Xpos+1,Ypos,0xf800);
    LCD_SetPoint(Xpos,Ypos+1,0xf800);
    LCD_SetPoint(Xpos+1,Ypos+1,0xf800);
}


/*******************************************************************************
* Function Name  : DrawCross
* Description    : specified coordinates painting crosshairs
* Input          : - Xpos: Row Coordinate
*                  - Ypos: Line Coordinate
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void DrawCross(unsigned short Xpos,unsigned short Ypos)
{
    LCD_DrawLine(Xpos-15,Ypos,Xpos-2,Ypos,0xffff);
    LCD_DrawLine(Xpos+2,Ypos,Xpos+15,Ypos,0xffff);
    LCD_DrawLine(Xpos,Ypos-15,Xpos,Ypos-2,0xffff);
    LCD_DrawLine(Xpos,Ypos+2,Xpos,Ypos+15,0xffff);
}


/*******************************************************************************
* Function Name  : Read_Ads7846
* Description    : X Y obtained after filtering
* Input          : None
* Output         : None
* Return         : Coordinate Structure address
* Attention      : None
*******************************************************************************/
Coordinate *Read_Ads7846(void)
{
    static Coordinate screen;
    int m0,m1,m2,TP_X[1],TP_Y[1],temp[3];
    unsigned char count = 0;
    int buffer[2][9] = {{0},{0}};  /* Multiple sampling coordinates X and Y */

    do  /* Loop sampling 9 times */
    {
        TP_GetAdXY(TP_X,TP_Y);
	    buffer[0][count] = TP_X[0];
	    buffer[1][count] = TP_Y[0];

        //printf("count: %u - x: %d -  y: %d\n", count, TP_X[0], TP_Y[0]);

        count++;
	}
    while( count < 9 );

    if( count == 9 )   /* Successful sampling 9, filtering */
    {
    /* In order to reduce the amount of computation, were divided into three groups averaged */
    temp[0] = ( buffer[0][0] + buffer[0][1] + buffer[0][2] ) / 3;
	temp[1] = ( buffer[0][3] + buffer[0][4] + buffer[0][5] ) / 3;
	temp[2] = ( buffer[0][6] + buffer[0][7] + buffer[0][8] ) / 3;
	/* Calculate the three groups of data */
	m0 = temp[0] - temp[1];
	m1 = temp[1] - temp[2];
	m2 = temp[2] - temp[0];
	/* Absolute value of the above difference */
	m0 = m0 > 0 ? m0 : (-m0);
        m1 = m1 > 0 ? m1 : (-m1);
	m2 = m2 > 0 ? m2 : (-m2);
	/* Judge whether the absolute difference exceeds the difference between the threshold,
	   If these three absolute difference exceeds the threshold,
       The sampling point is judged as outliers, Discard sampling points */
	if( m0 > THRESHOLD  &&  m1 > THRESHOLD  &&  m2 > THRESHOLD )
	{
	    return 0;
	}
	/* Calculating their average value */
	if( m0 < m1 )
	{
	    if( m2 < m0 )
	    {
	        screen.x = ( temp[0] + temp[2] ) / 2;
	    }
	    else
	    {
 	        screen.x = ( temp[0] + temp[1] ) / 2;
	    }
	}
	else if(m2<m1)
	{
 	    screen.x = ( temp[0] + temp[2] ) / 2;
	}
	else
	{
 	    screen.x = ( temp[1] + temp[2] ) / 2;
	}

	/* calculate the average value of Y */
    temp[0] = ( buffer[1][0] + buffer[1][1] + buffer[1][2] ) / 3;
	temp[1] = ( buffer[1][3] + buffer[1][4] + buffer[1][5] ) / 3;
	temp[2] = ( buffer[1][6] + buffer[1][7] + buffer[1][8] ) / 3;

	m0 = temp[0] - temp[1];
	m1 = temp[1] - temp[2];
	m2 = temp[2] - temp[0];

	m0 = m0 > 0 ? m0 : (-m0);
	m1 = m1 > 0 ? m1 : (-m1);
	m2 = m2 > 0 ? m2 : (-m2);
	if( m0 > THRESHOLD && m1 > THRESHOLD && m2 > THRESHOLD )
	{
	    return 0;
	}

	if( m0 < m1 )
	{
	    if( m2 < m0 )
	    {
	        screen.y = ( temp[0] + temp[2] ) / 2;
	    }
	    else
	    {
    	        screen.y = ( temp[0] + temp[1] ) / 2;
            }
        }
	else if( m2 < m1 )
	{
	    screen.y = ( temp[0] + temp[2] ) / 2;
	}
	else
	{
    	    screen.y = ( temp[1] + temp[2] ) / 2;
        }

       //printf("x: %4u -  y: %4u\n", screen.x, screen.y);
       Screen.x = screen.x;
       Screen.y = screen.y;

       return &screen;
    }

    return 0;
}


/*******************************************************************************
* Function Name  : setCalibrationMatrix
* Description    : Calculated K A B C D E F
* Input          : None
* Output         : None
* Return         : return 1 success , return 0 fail
* Attention		 : None
*******************************************************************************/
FunctionalState setCalibrationMatrix( Coordinate * displayPtr, Coordinate * screenPtr, Matrix * matrixPtr)
{

    FunctionalState retTHRESHOLD = ENABLE ;

    matrixPtr->Divider = ((screenPtr[0].x - screenPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
                         ((screenPtr[1].x - screenPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;
    if( matrixPtr->Divider == 0 )
    {
        retTHRESHOLD = DISABLE;
    }
    else
    {

        matrixPtr->An = ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
                        ((displayPtr[1].x - displayPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;

        matrixPtr->Bn = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].x - displayPtr[2].x)) -
                        ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].x - screenPtr[2].x)) ;

        matrixPtr->Cn = (screenPtr[2].x * displayPtr[1].x - screenPtr[1].x * displayPtr[2].x) * screenPtr[0].y +
                        (screenPtr[0].x * displayPtr[2].x - screenPtr[2].x * displayPtr[0].x) * screenPtr[1].y +
                        (screenPtr[1].x * displayPtr[0].x - screenPtr[0].x * displayPtr[1].x) * screenPtr[2].y ;

        matrixPtr->Dn = ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].y - screenPtr[2].y)) -
                        ((displayPtr[1].y - displayPtr[2].y) * (screenPtr[0].y - screenPtr[2].y)) ;

        matrixPtr->En = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].y - displayPtr[2].y)) -
                        ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].x - screenPtr[2].x)) ;

        matrixPtr->Fn = (screenPtr[2].x * displayPtr[1].y - screenPtr[1].x * displayPtr[2].y) * screenPtr[0].y +
                        (screenPtr[0].x * displayPtr[2].y - screenPtr[2].x * displayPtr[0].y) * screenPtr[1].y +
                        (screenPtr[1].x * displayPtr[0].y - screenPtr[0].x * displayPtr[1].y) * screenPtr[2].y ;
    }
    return( retTHRESHOLD ) ;
}


/*******************************************************************************
* Function Name  : getDisplayPoint
* Description    : channel XY via K A B C D E F value converted to the LCD screen coordinates
* Input          : None
* Output         : None
* Return         : return 1 success , return 0 fail
* Attention		 : None
*******************************************************************************/
FunctionalState getDisplayPoint(Coordinate * displayPtr, Coordinate * screenPtr, Matrix * matrixPtr)
{
    FunctionalState retTHRESHOLD = ENABLE ;
    long double an, bn, cn, dn, en, fn, sx, sy, md;
    int i;

    /*an = matrixPtr->An;
    bn = matrixPtr->Bn;
    cn = matrixPtr->Cn;
    dn = matrixPtr->Dn;
    en = matrixPtr->En;
    fn = matrixPtr->Fn;

    sx = screenPtr->x;
    sy = screenPtr->y;
    md = matrixPtr->Divider;*/

    an = matrix.An;
    bn = matrix.Bn;
    cn = matrix.Cn;
    dn = matrix.Dn;
    en = matrix.En;
    fn = matrix.Fn;

    sx = Screen.x;
    sy = Screen.y;
    md = matrix.Divider;

    if( md != 0 )
    {
        /* XD = AX+BY+C */
        display.x = ( (an * sx) + (bn * sy) + cn) / md;
   	    /* YD = DX+EY+F */
        display.y = ( (dn * sx) + (en * sy) + fn) / md;
        
        //printf("x: %d -  y: %d\n", display.x, display.y);
        
        for (i=0; i<20; i++)
        {
            if (Butt[i].exist)
            {
                if ( (display.x > Butt[i].x0) && (display.x < Butt[i].x1) && (display.y > Butt[i].y0) && (display.y < Butt[i].y1) ) {
                	Butt[i].pressed = 1;
                    LCD_DrawBox(Butt[i].x0, Butt[i].y0, Butt[i].x1, Butt[i].y1, Butt[i].fcol, Butt[i].col);
				    LCD_Text(Butt[i].xo, Butt[i].yo, Butt[i].text, Butt[i].fcol, Butt[i].col);
				    DelayMicrosecondsNoSleep(150000);
                    LCD_DrawBox(Butt[i].x0, Butt[i].y0, Butt[i].x1, Butt[i].y1, Butt[i].col, Butt[i].fcol);
				    LCD_Text(Butt[i].xo, Butt[i].yo, Butt[i].text, Butt[i].col, Butt[i].fcol);
                } else {
                	Butt[i].pressed = 0;
                }
            }
        }
    }
    else
    {
       retTHRESHOLD = DISABLE;
    }
    return(retTHRESHOLD);
}


/*******************************************************************************
* Function Name  : TP_Cal
* Description    : calibrate touch screen
* Input          : None
* Output         : None
* Return         : None
* Attention	 	 : None
*******************************************************************************/
void TP_Cal(void)
{
    unsigned char i;
    Coordinate * Ptr;
    FILE *fp, *fp2;

    // read the values
    if((fp=fopen("cal", "rb"))==NULL)
    {
        printf("Cannot open CAL file\n");

        for(i=0;i<3;i++)
        {
            LCD_Clear(Black);
            LCD_Text(10,10,"Touch crosshair to calibrate",White,Black);

            DrawCross(DisplaySample[i].x,DisplaySample[i].y);
            do
            {
                Ptr = Read_Ads7846();
            }
            while( Ptr == (void*)0 );

            ScreenSample[i].x = Ptr->x;
            ScreenSample[i].y = Ptr->y;
            printf("cal: %u  x: %4u y: %4u\n", i, ScreenSample[i].x, ScreenSample[i].y);
        }

        // get calibration parameters
        setCalibrationMatrix(&DisplaySample[0], &ScreenSample[0], &matrix);

        Screen.x = -1;
        Screen.y = -1;
        LCD_Clear(Black);

        // write the values
        if ((fp2=fopen("cal", "wb"))==NULL)
        {
            printf("Cannot open file\n");
        } else {
            if (fwrite(&matrix, sizeof(Matrix), 1, fp2) != 1) printf("File write error\n");
            fclose(fp2);
        }
    } else {
        if (fread(&matrix, sizeof(Matrix), 1, fp) != 1)
        {
            if (feof(fp))
                printf("Premature end of file\n");
            else
                printf("File read error\n");
        }
        fclose(fp);
    }
}


/*******************************************************************************************
      END FILE
********************************************************************************************/
