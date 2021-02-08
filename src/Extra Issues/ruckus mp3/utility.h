//
//  utility.h
//  Miscellaneous useful things for Ruckus
//
//  Licensed under the BSD license. See LICENSE for details.
//  Copyright (c) 2006 Daniel S. Neumeyer <dan@captaindan.org>
//

#ifndef __UTILITY_H__
#define __UTILITY_H__


#define RUCKUS_VERSION_STRING ("1.0b1")


#define ASSERT(condition, failureMessage) if (! (condition)) rksUtilityOhShit(failureMessage, __FILE__, __LINE__)
void rksUtilityOhShit(const char *message, const char *file, int line);

int rksUtilitySetupCallbacks();

#define SAFE_FREE(ptr) if (ptr) { free((void *) ptr); ptr = NULL; }
#define SAFE_STR(str) (str ? str : "")


#endif // __UTILITY_H__
