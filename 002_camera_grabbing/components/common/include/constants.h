/*
defines some constants that are globally valid
*/
#ifndef CONSTANTS_H
#define CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif
#define BYTES_PER_KB  1024UL
#define BYTES_PER_MB (1024UL*1024UL)
#define BYTES_PER_GB (1024UL*1024UL*1024UL)

// floating point conversions
// conversions of general terms
#define n2u_f (1.0/1000.0f)
#define n2m_f (1.0f/1000000.0f)
#define n2b_f (1.0/1000000000.0f)
#define n2k_f (1.0f/1000000000000.0)
#define n2M_f (1.0f/1000000000000000.0)
#define n2G_f (1.0f/1000000000000000000.0)

#define u2n_f (1000.0f)
#define u2m_f (1.0f/1000.0f)
#define u2b_f (1.0f/1000000.0)
#define u2k_f (1.0f/1000000000.0)
#define u2M_f (1.0f/1000000000000.0)
#define u2G_f (1.0f/1000000000000000.0)

#define m2n_f (1000000.0f)
#define m2u_f (1000.0f)
#define m2b_f (1.0f/1000.0f)
#define m2k_f (1.0f/1000000.0f)
#define m2M_f (1.0f/1000000000.0f)
#define m2G_f (1.0f/1000000000000.0f)

#define b2n_f (1000000000.0f)
#define b2u_f (1000000.0f)
#define b2m_f (1000.0f)
#define b2k_f (1.0f/1000.0f)
#define b2M_f (1.0f/1000000.0f)
#define b2G_f (1.0f/1000000000.0f)

#define k2n_f (1000000000000.0f)
#define k2u_f (1000000000.0f)
#define k2m_f (1000000.0f)
#define k2b_f (1000.0f)
#define k2M_f (1.0f/1000.0f)
#define k2G_f (1.0f/1000000.0f)

#define M2n_f (1000000000000000.0f)
#define M2u_f (1000000000000.0f)
#define M2m_f (1000000000.0f)
#define M2b_f (1000000.0f)
#define M2k_f (1000.0f)
#define M2G_f (1.0f/1000.0f)

#define G2n_f (1000000000000000000.0f)
#define G2u_f (1000000000000000.0f)
#define G2m_f (1000000000000.0f)
#define G2b_f (1000000000.0f)
#define G2k_f (1000000.0f)
#define G2M_f (1000.0f)

// ANSI colors
#define ANSI_RESET      "\033[0m"
#define ANSI_BLACK      "\033[0;30m"
#define ANSI_WHITE      "\033[0;37m"
#define ANSI_WHITE_B    "\033[1;37m"
#define ANSI_GRAY       "\033[0;90m"

// _B means BRIGHT
#define ANSI_BLUE       "\033[0;34m"
#define ANSI_BLUE_B     "\033[1;34m"
#define ANSI_RED        "\033[0;31m"
#define ANSI_RED_B      "\033[1;31m"
#define ANSI_YELLOW     "\033[0;33m"
#define ANSI_YELLOW_B   "\033[1;33m"
#define ANSI_GREEN      "\033[0;32m"
#define ANSI_GREEN_B    "\033[1;32m"
#define ANSI_CYAN       "\033[0;36m"
#define ANSI_CYAN_B     "\033[1;36m"
#define ANSI_MAGENTA    "\033[0;35m"
#define ANSI_MAGENTA_B  "\033[1;35m"
#define ANSI_ORANGE     "\033[38;5;208m"

#define ANSI_DEBUG      "\033[1;36m"
#define ANSI_DATA      "\033[1;34m"
#define ANSI_SPECIAL   "\033[1;35m"
#ifdef __cplusplus
}
#endif
#endif // CONSTANTS_H