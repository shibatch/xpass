#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

extern bool comparison1c(double v, double w, double x, double y, double z);
extern bool comparison2c(double v, double w, double x, double y, double z);
extern bool comparison3c(double v, double w, double x, double y, double z);
extern bool comparison4c(double v, double w, double x, double y, double z);
extern bool comparison5c(double v, double w, double x, double y, double z);
extern bool comparison6c(double a, double b, double c, double d, double e);

extern bool comparison1t(double v, double w, double x, double y, double z);
extern bool comparison2t(double v, double w, double x, double y, double z);
extern bool comparison3t(double v, double w, double x, double y, double z);
extern bool comparison4t(double v, double w, double x, double y, double z);
extern bool comparison5t(double v, double w, double x, double y, double z);
extern bool comparison6t(double a, double b, double c, double d, double e);

#define TEST(x) do {							\
  extern bool f ## x ## c(double, double, double, double);		\
  extern bool f ## x ## t(double, double, double, double);		\
  if (f ## x ## c(a, b, c, d) != f ## x ## t(a, b, c, d)) {		\
  printf(#x " : %g %g %g %g\n", a, b, c, d);				\
  exit(-1);								\
  }									\
  } while(0)
  
double randm11() {
  double r;
  do {
    r = rand() / (double)RAND_MAX * 2 - 1;
  } while(fabs(r) < 1e-3);
  return r;
}

int main(int argc, char **argv) {
  srand(time(NULL));

  for(int i=0;i<1000000;i++) {
    double a = randm11();
    double b = randm11();
    double c = randm11();
    double d = randm11();

    TEST( 0); TEST( 1); TEST( 2); TEST( 3); TEST( 4); TEST( 5); TEST( 6); TEST( 7); TEST( 8); TEST( 9);
    TEST(10); TEST(11); TEST(12); TEST(13); TEST(14); TEST(15); TEST(16); TEST(17); TEST(18); TEST(19);
    TEST(20); TEST(21); TEST(22); TEST(23); TEST(24); TEST(25); TEST(26); TEST(27); TEST(28); TEST(29);
    TEST(30); TEST(31); TEST(32); TEST(33); TEST(34); TEST(35); TEST(36); TEST(37); TEST(38); TEST(39);
    TEST(40); TEST(41); TEST(42); TEST(43); TEST(44); TEST(45); TEST(46); TEST(47); TEST(48); TEST(49);
    TEST(50); TEST(51); TEST(52); TEST(53); TEST(54); TEST(55); TEST(56); TEST(57); TEST(58); TEST(59);
    TEST(60); TEST(61); TEST(62); TEST(63); TEST(64); TEST(65); TEST(66); TEST(67); TEST(68); TEST(69);
    TEST(70); TEST(71); TEST(72); TEST(73); TEST(74); TEST(75); TEST(76); TEST(77); TEST(78); TEST(79);
    TEST(80); TEST(81); TEST(82); TEST(83); TEST(84); TEST(85); TEST(86); TEST(87); TEST(88); TEST(89);
    TEST(90); TEST(91); TEST(92); TEST(93); TEST(94); TEST(95); TEST(96); TEST(97); TEST(98); TEST(99);
  }

  printf("Passed\n");
  exit(0);
}
