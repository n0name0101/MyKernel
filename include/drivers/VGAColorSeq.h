#ifndef __VGACOLORSEQ_H
#define __VGACOLORSEQ_H

#define ESC "\x1B" // Escape character

#define RESET        ESC"[0m"
#define BLACK        ESC"[30m"
#define RED          ESC"[31m"
#define GREEN        ESC"[32m"
#define YELLOW       ESC"[33m"
#define BLUE         ESC"[34m"
#define MAGENTA      ESC"[35m"
#define CYAN         ESC"[36m"
#define WHITE        ESC"[37m"

#define BRIGHT_BLACK   ESC"[90m"
#define BRIGHT_RED     ESC"[91m"
#define BRIGHT_GREEN   ESC"[92m"
#define BRIGHT_YELLOW  ESC"[93m"
#define BRIGHT_BLUE    ESC"[94m"
#define BRIGHT_MAGENTA ESC"[95m"
#define BRIGHT_CYAN    ESC"[96m"
#define BRIGHT_WHITE   ESC"[97m"

#define BG_BLACK       ESC"[40m"
#define BG_RED         ESC"[41m"
#define BG_GREEN       ESC"[42m"
#define BG_YELLOW      ESC"[43m"
#define BG_BLUE        ESC"[44m"
#define BG_MAGENTA     ESC"[45m"
#define BG_CYAN        ESC"[46m"
#define BG_WHITE       ESC"[47m"

#define BOLD           ESC"[1m"
#define UNDERLINE      ESC"[4m"

#endif         //__VGACOLORSEQ_H
