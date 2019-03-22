/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Common structures and functions              *
 *                               used by both Client and Server               *
 *                                                                            *
 *****************************************************************************/


//
// Interface
//
#include "ap_utilities.h"


//
// Implementation
//
void error_declare(const char *msg) {
    /* Output error message */

    fprintf(stderr, "\nERROR: %s\n\n", msg);
}


void error_out(const char *msg) {
    /* Output error message and exit */

    error_declare(msg);
    exit(EXIT_FAILURE);
}


