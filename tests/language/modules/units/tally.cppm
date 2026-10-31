// modules.md "The global and private module fragments" (Clang only: GCC 15/16 don't implement it).
export module tally;

export int next();

module :private;
int value = 0;
int next() { return ++value; }
