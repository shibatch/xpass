#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

static uint64_t xseed = 1234567890123456789ULL;

static uint32_t xrand() {
  xseed = xseed * 6364136223846793005ULL + 1;
  return xseed >> 32;
}

static double randm11() {
  double r;
  do {
    r = xrand() / (double)(1ULL << 32) * 2 - 1;
  } while(fabs(r) < 1e-3);
  return r;
}

#ifdef DOUBLE
#define TEST(x) do {						\
    extern bool f ## x ## c(double, double, double, double);	\
    extern bool f ## x ## t(double, double, double, double);	\
    if (f ## x ## c(a, b, c, d) != f ## x ## t(a, b, c, d)) {	\
      printf(#x "(double) : %g %g %g %g\n", a, b, c, d);	\
      nfail++;							\
      if (nfail > 20) {						\
	printf("Tester exiting(1)\n");				\
	exit(-1);						\
      }								\
    }								\
  } while(0)
#endif

#ifdef FLOAT
#define TEST(x) do {						\
    extern bool f ## x ## c(float, float, float, float);	\
    extern bool f ## x ## t(float, float, float, float);	\
    if (f ## x ## c(a, b, c, d) != f ## x ## t(a, b, c, d)) {	\
      printf(#x "(float) : %g %g %g %g\n", a, b, c, d);		\
      nfail++;							\
      if (nfail > 20) {						\
	printf("Tester exiting(2)\n");				\
	exit(-1);						\
      }								\
    }								\
  } while(0)
#endif

#ifdef DOUBLE2
typedef double double2 __attribute__((ext_vector_type(2)));
typedef long int2 __attribute__((ext_vector_type(2)));

#define TEST(x) do {							\
    extern int2 f ## x ## c(double2, double2, double2, double2);	\
    extern int2 f ## x ## t(double2, double2, double2, double2);	\
    double2 va = (double2) { randm11(), randm11() };			\
    double2 vb = (double2) { randm11(), randm11() };			\
    double2 vc = (double2) { randm11(), randm11() };			\
    double2 vd = (double2) { randm11(), randm11() };			\
    int index = rand() & 1;						\
    va[index] = a; vb[index] = b; vc[index] = c; vd[index] = d;		\
    if (f ## x ## c(va, vb, vc, vd)[index] != f ## x ## t(va, vb, vc, vd)[index]) { \
	printf(#x "(double2) : %g %g %g %g\n", va[index], vb[index], vc[index], vd[index]); \
	nfail++;							\
	if (nfail > 20) {						\
	  printf("Tester exiting(3)\n");				\
	  exit(-1);							\
	}								\
      }									\
  } while(0)
#endif

#ifdef FLOAT4
typedef float float4 __attribute__((ext_vector_type(4)));
typedef int int4 __attribute__((ext_vector_type(4)));

#define TEST(x) do {							\
    extern int4 f ## x ## c(float4, float4, float4, float4);		\
    extern int4 f ## x ## t(float4, float4, float4, float4);		\
    float4 va = (float4) { randm11(), randm11(), randm11(), randm11() }; \
    float4 vb = (float4) { randm11(), randm11(), randm11(), randm11() }; \
    float4 vc = (float4) { randm11(), randm11(), randm11(), randm11() }; \
    float4 vd = (float4) { randm11(), randm11(), randm11(), randm11() }; \
    int index = rand() & 3;						\
    va[index] = a; vb[index] = b; vc[index] = c; vd[index] = d;		\
    if (f ## x ## c(va, vb, vc, vd)[index] != f ## x ## t(va, vb, vc, vd)[index]) { \
	printf(#x "(float4) : %g %g %g %g\n", va[index], vb[index], vc[index], vd[index]); \
	nfail++;							\
	if (nfail > 20) {						\
	  printf("Tester exiting(4)\n");				\
	  exit(-1);							\
	}								\
      }									\
  } while(0)
#endif

int main(int argc, char **argv) {
  int nfail = 0;
  for(int i=0;i<500000;i++) {
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

  if (nfail > 10) {
    printf("Tester exiting(0)\n");
    exit(-1);
  }

  exit(0);
}
