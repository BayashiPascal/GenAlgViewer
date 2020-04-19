#include "pbmath.h"
#include "genbrush.h"

int main() {
  VecShort2D dim = VecShortCreateStatic2D();
  VecSet(&dim, 0, 100);
  VecSet(&dim, 1, 100);
  GenBrush* gb = GBCreateImage(&dim);
  GBSetFileName(gb, "./genealogy.tga");
  GBRender(gb);
  GBFree(&gb);
  return 0;
}
