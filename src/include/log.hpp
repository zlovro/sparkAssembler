#pragma once
#include <print>

//Regular text
#define CLR_FRBLK "\x1b[0;30m"
#define CLR_FRRED "\x1b[0;31m"
#define CLR_FRGRN "\x1b[0;32m"
#define CLR_FRYEL "\x1b[0;33m"
#define CLR_FRBLU "\x1b[0;34m"
#define CLR_FRMAG "\x1b[0;35m"
#define CLR_FRCYN "\x1b[0;36m"
#define CLR_FRWHT "\x1b[0;37m"

//Regular bold text
#define CLR_FBBLK "\x1b[1;30m"
#define CLR_FBRED "\x1b[1;31m"
#define CLR_FBGRN "\x1b[1;32m"
#define CLR_FBYEL "\x1b[1;33m"
#define CLR_FBBLU "\x1b[1;34m"
#define CLR_FBMAG "\x1b[1;35m"
#define CLR_FBCYN "\x1b[1;36m"
#define CLR_FBWHT "\x1b[1;37m"

//Regular underline text
#define CLR_FUBLK "\x1b[4;30m"
#define CLR_FURED "\x1b[4;31m"
#define CLR_FUGRN "\x1b[4;32m"
#define CLR_FUYEL "\x1b[4;33m"
#define CLR_FUBLU "\x1b[4;34m"
#define CLR_FUMAG "\x1b[4;35m"
#define CLR_FUCYN "\x1b[4;36m"
#define CLR_FUWHT "\x1b[4;37m"

//Regular background
#define CLR_RBBLK "\x1b[0;40m"
#define CLR_RBRED "\x1b[0;41m"
#define CLR_RBGRN "\x1b[0;42m"
#define CLR_RBYEL "\x1b[0;43m"
#define CLR_RBBLU "\x1b[0;44m"
#define CLR_RBMAG "\x1b[0;45m"
#define CLR_RBCYN "\x1b[0;46m"
#define CLR_RBWHT "\x1b[0;47m"

//High intensty background 
#define CLR_BHBLKH "\x1b[0;100m"
#define CLR_BHREDH "\x1b[0;101m"
#define CLR_BHGRNH "\x1b[0;102m"
#define CLR_BHYELH "\x1b[0;103m"
#define CLR_BHBLUH "\x1b[0;104m"
#define CLR_BHMAGH "\x1b[0;105m"
#define CLR_BHCYNH "\x1b[0;106m"
#define CLR_BHWHTH "\x1b[0;107m"

//High intensty text
#define CLR_FHBLK "\x1b[0;90m"
#define CLR_FHRED "\x1b[0;91m"
#define CLR_FHGRN "\x1b[0;92m"
#define CLR_FHYEL "\x1b[0;93m"
#define CLR_FHBLU "\x1b[0;94m"
#define CLR_FHMAG "\x1b[0;95m"
#define CLR_FHCYN "\x1b[0;96m"
#define CLR_FHWHT "\x1b[0;97m"

//Bold high intensity text
#define CLR_FBHBLK "\x1b[1;90m"
#define CLR_FBHRED "\x1b[1;91m"
#define CLR_FBHGRN "\x1b[1;92m"
#define CLR_FBHYEL "\x1b[1;93m"
#define CLR_FBHBLU "\x1b[1;94m"
#define CLR_FBHMAG "\x1b[1;95m"
#define CLR_FBHCYN "\x1b[1;96m"
#define CLR_FBHWHT "\x1b[1;97m"

#define CLR_RESET "\x1b[0m"

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#define LOGRAW(fmt, ...) print("[" __FILE__ ":" LINE_STRING "] - " fmt, __VA_ARGS__)
#define LOGERR(fmt, ...) LOGRAW(CLR_FRRED fmt CLR_RESET, __VA_ARGS__)
#define LOGWRN(fmt, ...) LOGRAW(CLR_FRYEL fmt CLR_RESET, __VA_ARGS__)
#define LOGINF(fmt, ...) LOGRAW(CLR_FBWHT fmt CLR_RESET, __VA_ARGS__)

#ifdef DEBUG
#define LOGDBG(fmt, ...) LOGRAW(CLR_FRCYN fmt CLR_RESET, __VA_ARGS__)
#endif