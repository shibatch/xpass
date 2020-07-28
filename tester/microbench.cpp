#include <iostream>
#include <cmath>

#if !defined(LATENCY) && !defined(THROUGHPUT)
#error Either LATENCY or THROUGHPUT has to be defined
#endif

typedef double double2 __attribute__((ext_vector_type(2)));
typedef long long2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef long long4 __attribute__((ext_vector_type(4)));


static __attribute__((always_inline)) double4 sqrt4(double4 x) {
  return (double4) { sqrt(x[0]), sqrt(x[1]), sqrt(x[2]), sqrt(x[3]) };
}

static __attribute__((always_inline)) double2 sqrt2(double2 x) {
  return (double2) { sqrt(x[0]), sqrt(x[1]) };
}

static __attribute__((always_inline)) double4 ternary4(long4 b, double4 x, double4 y) {
  return (double4) {
    b[0] ? x[0] : y[0],
    b[1] ? x[1] : y[1],
    b[2] ? x[2] : y[2],
    b[3] ? x[3] : y[3],
  };
}

static __attribute__((always_inline)) double2 ternary2(long2 b, double2 x, double2 y) {
  return (double2) {
    b[0] ? x[0] : y[0],
    b[1] ? x[1] : y[1],
  };
}

static bool func0(double w, double x, double y, double z) {
  return w * sqrt(x) + y > z;
}

bool func0loop(double *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    double *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = func0(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= func0(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static bool func1(double a, double b, double c, double d) {
  return a / b + c > d;
}

bool func1loop(double *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    double *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = func1(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= func1(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static bool func2(double a, double b, double c, double d) {
  return 1.2 * sqrt(a / (2.3 * b)) + c < d;
}

bool func2loop(double *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    double *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = func2(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= func2(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static bool func3(double a0, double a1, double a2, double a3) {
  return 1.1 / a0 + a1 / a2 < 1.2 / a3;
}

bool func3loop(double *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    double *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = func3(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= func3(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long2 v2func0(double2 w, double2 x, double2 y, double2 z) {
  return w * sqrt2(x) + y > z;
}

long2 v2func0loop(double *a0, long n, long m) {
  long2 b = { 0, 0 };
  for(long j=m;j>0;j--) {
    double2 *a = (double2 *)a0;
    for(long i=n/2;i>0;i--) {
#ifdef LATENCY
      b = v2func0(ternary2(b, a[0], a[1]),
		  ternary2(b, a[2], a[3]),
		  ternary2(b, a[4], a[5]),
		  ternary2(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v2func0(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long2 v2func1(double2 a, double2 b, double2 c, double2 d) {
  return a / b + c > d;
}

long2 v2func1loop(double *a0, long n, long m) {
  long2 b = { 0, 0 };
  for(long j=m;j>0;j--) {
    double2 *a = (double2 *)a0;
    for(long i=n/2;i>0;i--) {
#ifdef LATENCY
      b = v2func0(ternary2(b, a[0], a[1]),
		  ternary2(b, a[2], a[3]),
		  ternary2(b, a[4], a[5]),
		  ternary2(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v2func1(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long2 v2func2(double2 a, double2 b, double2 c, double2 d) {
  return 1.2 * sqrt2(a / (2.3 * b)) + c < d;
}

long2 v2func2loop(double *a0, long n, long m) {
  long2 b = false;
  for(long j=m;j>0;j--) {
    double2 *a = (double2 *)a0;
    for(long i=n/2;i>0;i--) {
#ifdef LATENCY
      b = v2func2(ternary2(b, a[0], a[1]),
		  ternary2(b, a[2], a[3]),
		  ternary2(b, a[4], a[5]),
		  ternary2(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v2func2(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long2 v2func3(double2 a0, double2 a1, double2 a2, double2 a3) {
  return 1.1 / a0 + a1 / a2 < 1.2 / a3;
}

long2 v2func3loop(double *a0, long n, long m) {
  long2 b = false;
  for(long j=m;j>0;j--) {
    double2 *a = (double2 *)a0;
    for(long i=n/2;i>0;i--) {
#ifdef LATENCY
      b = v2func3(ternary2(b, a[0], a[1]),
		  ternary2(b, a[2], a[3]),
		  ternary2(b, a[4], a[5]),
		  ternary2(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v2func3(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long4 v4func0(double4 w, double4 x, double4 y, double4 z) {
  return w * sqrt4(x) + y > z;
}

long4 v4func0loop(double *a0, long n, long m) {
  long4 b = { 0, 0, 0, 0 };
  for(long j=m;j>0;j--) {
    double4 *a = (double4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4func0(ternary4(b, a[0], a[1]),
		  ternary4(b, a[2], a[3]),
		  ternary4(b, a[4], a[5]),
		  ternary4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4func0(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long4 v4func1(double4 a, double4 b, double4 c, double4 d) {
  return a / b + c > d;
}

long4 v4func1loop(double *a0, long n, long m) {
  long4 b = { 0, 0, 0, 0 };
  for(long j=m;j>0;j--) {
    double4 *a = (double4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4func1(ternary4(b, a[0], a[1]),
		  ternary4(b, a[2], a[3]),
		  ternary4(b, a[4], a[5]),
		  ternary4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4func1(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long4 v4func2(double4 a, double4 b, double4 c, double4 d) {
  return 1.2 * sqrt4(a / (2.3 * b)) + c < d;
}

long4 v4func2loop(double *a0, long n, long m) {
  long4 b = false;
  for(long j=m;j>0;j--) {
    double4 *a = (double4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4func2(ternary4(b, a[0], a[1]),
		  ternary4(b, a[2], a[3]),
		  ternary4(b, a[4], a[5]),
		  ternary4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4func2(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static long4 v4func3(double4 a0, double4 a1, double4 a2, double4 a3) {
  return 1.1 / a0 + a1 / a2 < 1.2 / a3;
}

long4 v4func3loop(double *a0, long n, long m) {
  long4 b = false;
  for(long j=m;j>0;j--) {
    double4 *a = (double4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4func3(ternary4(b, a[0], a[1]),
		  ternary4(b, a[2], a[3]),
		  ternary4(b, a[4], a[5]),
		  ternary4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4func3(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}


#include <chrono>
#include <cstdlib>

#pragma clang optimize off

#define N (1024L)
#define M (1024L*256)

static double rand1() { return rand() / (double)RAND_MAX; }

__attribute__ ((__aligned__(64))) double a[N * 8];

int main(int argc, char **argv) {
  for(long i=0;i<N*8;i++) a[i] = rand1();

  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> elapsed_seconds;
  bool b;
  long2 l2;
  long4 l4;

#ifdef LATENCY
  std::cout << "Latency of executing each function\n";
#endif
#ifdef THROUGHPUT
  std::cout << "Reciprocal throughput of executing each function\n";
#endif

  start = std::chrono::system_clock::now();
  b = func0loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = func1loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar div : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = func2loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = func3loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l2 = v2func0loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l2 = v2func1loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 div : " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l2 = v2func2loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 combination 1: " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l2 = v2func3loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

#if defined(__AVX__)
  start = std::chrono::system_clock::now();
  l4 = v4func0loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[4]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l4 = v4func1loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[4]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 div : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l4 = v4func2loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[4]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l4 = v4func3loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[4]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";
#endif
}
