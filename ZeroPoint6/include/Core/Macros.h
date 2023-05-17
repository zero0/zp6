#ifndef ZP_MACROS_H
#define ZP_MACROS_H

#define ZP_UNUSED(v)      (void)v

#define ZP_CONCAT_(a, b)  a ## b
#define ZP_CONCAT(a, b)   ZP_CONCAT_( a, b )

#define ZP_NAMEOF(x)      #x
#define ZP_STR(x)         #x

#define ZP_ARRAY_SIZE(a)  ( sizeof( a ) / sizeof( a[ 0 ] ) )

#define ZP_ALIGN_SIZE(s,a)  ( ( (s) + ( (a) - 1 ) ) & -(a) )

#define ZP_FG_BLACK     "30"
#define ZP_FG_RED       "31"
#define ZP_FG_GREEN     "32"
#define ZP_FG_YELLOW    "33"
#define ZP_FG_BLUE      "34"
#define ZP_FG_MAGENTA   "35"
#define ZP_FG_CYAN      "36"
#define ZP_FG_WHITE     "37"

#define ZP_FG_GRAY             "90"
#define ZP_FG_BRIGHT_RED       "91"
#define ZP_FG_BRIGHT_GREEN     "92"
#define ZP_FG_BRIGHT_YELLOW    "93"
#define ZP_FG_BRIGHT_BLUE      "94"
#define ZP_FG_BRIGHT_MAGENTA   "95"
#define ZP_FG_BRIGHT_CYAN      "96"
#define ZP_FG_BRIGHT_WHITE     "97"

#define ZP_BG_BLACK     "40"
#define ZP_BG_RED       "41"
#define ZP_BG_GREEN     "42"
#define ZP_BG_YELLOW    "43"
#define ZP_BG_BLUE      "44"
#define ZP_BG_MAGENTA   "45"
#define ZP_BG_CYAN      "46"
#define ZP_BG_WHITE     "47"

#define ZP_BG_GRAY             "100"
#define ZP_BG_BRIGHT_RED       "101"
#define ZP_BG_BRIGHT_GREEN     "102"
#define ZP_BG_BRIGHT_YELLOW    "103"
#define ZP_BG_BRIGHT_BLUE      "104"
#define ZP_BG_BRIGHT_MAGENTA   "105"
#define ZP_BG_BRIGHT_CYAN      "106"
#define ZP_BG_BRIGHT_WHITE     "107"

#define ZP_FT_NORMAL        "0"
#define ZP_FT_BOLD          "1"
#define ZP_FT_FAINT         "2"
#define ZP_FT_ITALIC        "3"
#define ZP_FT_UNDERLINE     "4"
#define ZP_FT_SLOW_BLINK    "5"
#define ZP_FT_FAST_BLINK    "6"
#define ZP_FT_STRIKE        "9"

#if ZP_USE_CONSOLE_COLORS
#define ZP_CC_(t, f, b) "\033[" t ";" f ";" b "m"
#define ZP_CC_RESET     "\033[0m"
#else
#define ZP_CC_(...)     ""
#define ZP_CC_RESET     ""
#endif

#define ZP_CC(t, f, b)  ZP_CC_(ZP_CONCAT(ZP_FT_, t), ZP_CONCAT(ZP_FG_, f), ZP_CONCAT(ZP_BG_, b))
#define ZP_CC_N(f, b)   ZP_CC(NORMAL, f, b)
#define ZP_CC_B(f, b)   ZP_CC(BOLD, f, b)
#define ZP_CC_I(f, b)   ZP_CC(ITALIC, f, b)

#define ZP_NONCOPYABLE(x)           \
private:                            \
    x( const x&) = delete;          \
    x( x&& ) = delete;              \
    x& operator=(x const&) = delete;

#endif //ZP_MACROS_H
