#include "../Starbase.h"
#include <stdbool.h>
#ifndef IMPROVE_H
#define IMPROVE_H
#define NUMMEANTHRESHOLDS 7
bool meanimprove(starreal bestmeans[],starreal newmeans[],int passtype);


bool pass(int passtype,
          struct tetcomplex* mesh,
          struct arraypoolstack* tetstack,
          starreal threshold,
          bool *minsuccess,
          bool *meansuccess,
          int passnum,
          starreal bestmeans[],
          struct behavior *behave,
          struct inputs *in,
          struct proxipool *vertexpool,
          int argc,
          char **argv);

void improveinit(struct tetcomplex *mesh,
                 struct proxipool *vertexpool,
                 struct arraypoolstack *tetstack,
                 struct behavior *behave,
                 struct inputs *in,
                 int argc, 
                 char **argv,
                 starreal bestmeans[NUMMEANTHRESHOLDS]);
void improvedeinit(struct tetcomplex *mesh,
                   struct proxipool *vertexpool,
                   struct arraypoolstack *tetstack,
                   struct behavior *behave,
                   struct inputs *in,
                   int argc, 
                   char **argv);
void staticimprove(struct behavior *behave,
                   struct inputs *in,
                   struct proxipool *vertexpool,
                   struct tetcomplex *mesh,
                   int argc,
                   char **argv);

#endif

