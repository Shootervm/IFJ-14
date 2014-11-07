/**
 * @file	common.h
 * @name	Interface with common functions that could be used across modules
 * @author	Albert Uchytil (xuchyt03), Vojtech Mašek (xmasek15)
 *********************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

/**
 * Debbugin macro
 * Prints out message + info about file and line.
 */
#ifdef DEBUG
#define debug(Message) fprintf(stderr, "DEBUG: %s:%d:\t" Message "\n", __FILE__, __LINE__)
#else
#define debug(Message)
#endif


/**
 * @brief Function converts char to int.
 * @param c char that's going to be converted
 * @returns integer value between 0 and 9
 *
 * Function converts char to numeric value represented by the char.
 * Returns 0 when the char is not a digit.
 */
int char_to_int(char c);

#endif //__COMMON_H__
