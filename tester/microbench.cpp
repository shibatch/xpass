#include <iostream>
#include <cmath>

#if !defined(LATENCY) && !defined(THROUGHPUT)
#error Either LATENCY or THROUGHPUT has to be defined
#endif

//

typedef double double2 __attribute__((ext_vector_type(2)));
typedef long long2 __attribute__((ext_vector_type(2)));

static __attribute__((always_inline)) double2 sqrt2(double2 x) {
  return (double2) { sqrt(x[0]), sqrt(x[1]) };
}

static __attribute__((always_inline)) double2 ternary2(long2 b, double2 x, double2 y) {
  return (double2) {
    b[0] ? x[0] : y[0],
    b[1] ? x[1] : y[1],
  };
}

//

typedef double double4 __attribute__((ext_vector_type(4)));
typedef long long4 __attribute__((ext_vector_type(4)));

static __attribute__((always_inline)) double4 sqrt4(double4 x) {
  return (double4) { sqrt(x[0]), sqrt(x[1]), sqrt(x[2]), sqrt(x[3]) };
}

static __attribute__((always_inline)) double4 ternary4(long4 b, double4 x, double4 y) {
  return (double4) {
    b[0] ? x[0] : y[0],
    b[1] ? x[1] : y[1],
    b[2] ? x[2] : y[2],
    b[3] ? x[3] : y[3],
  };
}

//

typedef float float4 __attribute__((ext_vector_type(4)));
typedef int int4 __attribute__((ext_vector_type(4)));

static __attribute__((always_inline)) float4 sqrtf4(float4 x) {
  return (float4) { sqrtf(x[0]), sqrtf(x[1]), sqrtf(x[2]), sqrtf(x[3]) };
}

static __attribute__((always_inline)) float4 ternaryf4(int4 b, float4 x, float4 y) {
  return (float4) {
    b[0] ? x[0] : y[0],
    b[1] ? x[1] : y[1],
    b[2] ? x[2] : y[2],
    b[3] ? x[3] : y[3],
  };
}

//

typedef float float8 __attribute__((ext_vector_type(8)));
typedef int int8 __attribute__((ext_vector_type(8)));

static __attribute__((always_inline)) float8 sqrtf8(float8 x) {
  return (float8) {
    sqrtf(x[0]), sqrtf(x[1]), sqrtf(x[2]), sqrtf(x[3]),
    sqrtf(x[4]), sqrtf(x[5]), sqrtf(x[6]), sqrtf(x[7]),
  };
}

static __attribute__((always_inline)) float8 ternaryf8(int8 b, float8 x, float8 y) {
  return (float8) {
    b[0] ? x[0] : y[0],
    b[1] ? x[1] : y[1],
    b[2] ? x[2] : y[2],
    b[3] ? x[3] : y[3],
    b[4] ? x[4] : y[4],
    b[5] ? x[5] : y[5],
    b[6] ? x[6] : y[6],
    b[7] ? x[7] : y[7],
  };
}

//

static __attribute__((always_inline)) bool func0(double w, double x, double y, double z) {
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

static __attribute__((always_inline)) bool func1(double a, double b, double c, double d) {
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

static __attribute__((always_inline)) bool func2(double a, double b, double c, double d) {
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

static __attribute__((always_inline)) bool func3(double a0, double a1, double a2, double a3) {
  return a0 / sqrt(a1) + a2 < a3;
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

//

static __attribute__((always_inline)) long2 v2func0(double2 w, double2 x, double2 y, double2 z) {
  return w * sqrt2(x) + y > z;
}

long2 v2func0loop(double *a0, long n, long m) {
  long2 b = 0;
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

static __attribute__((always_inline)) long2 v2func1(double2 a, double2 b, double2 c, double2 d) {
  return a / b + c > d;
}

long2 v2func1loop(double *a0, long n, long m) {
  long2 b = 0;
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

static __attribute__((always_inline)) long2 v2func2(double2 a, double2 b, double2 c, double2 d) {
  return 1.2 * sqrt2(a / (2.3 * b)) + c < d;
}

long2 v2func2loop(double *a0, long n, long m) {
  long2 b = 0;
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

static __attribute__((always_inline)) long2 v2func3(double2 a0, double2 a1, double2 a2, double2 a3) {
  return a0 / sqrt2(a1) + a2 < a3;
}

long2 v2func3loop(double *a0, long n, long m) {
  long2 b = 0;
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

//

static __attribute__((always_inline)) long4 v4func0(double4 w, double4 x, double4 y, double4 z) {
  return w * sqrt4(x) + y > z;
}

long4 v4func0loop(double *a0, long n, long m) {
  long4 b = 0;
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

static __attribute__((always_inline)) long4 v4func1(double4 a, double4 b, double4 c, double4 d) {
  return a / b + c > d;
}

long4 v4func1loop(double *a0, long n, long m) {
  long4 b = 0;
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

static __attribute__((always_inline)) long4 v4func2(double4 a, double4 b, double4 c, double4 d) {
  return 1.2 * sqrt4(a / (2.3 * b)) + c < d;
}

long4 v4func2loop(double *a0, long n, long m) {
  long4 b = 0;
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

static __attribute__((always_inline)) long4 v4func3(double4 a0, double4 a1, double4 a2, double4 a3) {
  return a0 / sqrt4(a1) + a2 < a3;
}

long4 v4func3loop(double *a0, long n, long m) {
  long4 b = 0;
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

//

static __attribute__((always_inline)) bool funcf0(float w, float x, float y, float z) {
  return w * sqrtf(x) + y > z;
}

bool funcf0loop(float *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    float *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = funcf0(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= funcf0(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) bool funcf1(float a, float b, float c, float d) {
  return a / b + c > d;
}

bool funcf1loop(float *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    float *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = funcf1(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= funcf1(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) bool funcf2(float a, float b, float c, float d) {
  return 1.2f * sqrtf(a / (2.3f * b)) + c < d;
}

bool funcf2loop(float *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    float *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = funcf2(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= funcf2(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) bool funcf3(float a0, float a1, float a2, float a3) {
  return a0 / sqrtf(a1) + a2 < a3;
}

bool funcf3loop(float *a0, long n, long m) {
  bool b = false;
  for(long j=m;j>0;j--) {
    float *a = a0;
    for(long i=n;i>0;i--) {
#ifdef LATENCY
      b = funcf3(b ? a[0] : a[1], b ? a[2] : a[3], b ? a[4] : a[5], b ? a[6] : a[7]);
#endif
#ifdef THROUGHPUT
      b ^= funcf3(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

//

static __attribute__((always_inline)) int4 v4funcf0(float4 w, float4 x, float4 y, float4 z) {
  return w * sqrtf4(x) + y > z;
}

int4 v4funcf0loop(float *a0, long n, long m) {
  int4 b = 0;
  for(long j=m;j>0;j--) {
    float4 *a = (float4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4funcf0(ternaryf4(b, a[0], a[1]),
		   ternaryf4(b, a[2], a[3]),
		   ternaryf4(b, a[4], a[5]),
		   ternaryf4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4funcf0(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) int4 v4funcf1(float4 a, float4 b, float4 c, float4 d) {
  return a / b + c > d;
}

int4 v4funcf1loop(float *a0, long n, long m) {
  int4 b = 0;
  for(long j=m;j>0;j--) {
    float4 *a = (float4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4funcf0(ternaryf4(b, a[0], a[1]),
		   ternaryf4(b, a[2], a[3]),
		   ternaryf4(b, a[4], a[5]),
		   ternaryf4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4funcf1(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) int4 v4funcf2(float4 a, float4 b, float4 c, float4 d) {
  return 1.2f * sqrtf4(a / (2.3f * b)) + c < d;
}

int4 v4funcf2loop(float *a0, long n, long m) {
  int4 b = 0;
  for(long j=m;j>0;j--) {
    float4 *a = (float4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4funcf2(ternaryf4(b, a[0], a[1]),
		   ternaryf4(b, a[2], a[3]),
		   ternaryf4(b, a[4], a[5]),
		   ternaryf4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4funcf2(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) int4 v4funcf3(float4 a0, float4 a1, float4 a2, float4 a3) {
  return a0 / sqrtf4(a1) + a2 < a3;
}

int4 v4funcf3loop(float *a0, long n, long m) {
  int4 b = 0;
  for(long j=m;j>0;j--) {
    float4 *a = (float4 *)a0;
    for(long i=n/4;i>0;i--) {
#ifdef LATENCY
      b = v4funcf3(ternaryf4(b, a[0], a[1]),
		   ternaryf4(b, a[2], a[3]),
		   ternaryf4(b, a[4], a[5]),
		   ternaryf4(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v4funcf3(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

//

static __attribute__((always_inline)) int8 v8funcf0(float8 w, float8 x, float8 y, float8 z) {
  return w * sqrtf8(x) + y > z;
}

int8 v8funcf0loop(float *a0, long n, long m) {
  int8 b = 0;
  for(long j=m;j>0;j--) {
    float8 *a = (float8 *)a0;
    for(long i=n/8;i>0;i--) {
#ifdef LATENCY
      b = v8funcf0(ternaryf8(b, a[0], a[1]),
		   ternaryf8(b, a[2], a[3]),
		   ternaryf8(b, a[4], a[5]),
		   ternaryf8(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v8funcf0(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) int8 v8funcf1(float8 a, float8 b, float8 c, float8 d) {
  return a / b + c > d;
}

int8 v8funcf1loop(float *a0, long n, long m) {
  int8 b = 0;
  for(long j=m;j>0;j--) {
    float8 *a = (float8 *)a0;
    for(long i=n/8;i>0;i--) {
#ifdef LATENCY
      b = v8funcf1(ternaryf8(b, a[0], a[1]),
		   ternaryf8(b, a[2], a[3]),
		   ternaryf8(b, a[4], a[5]),
		   ternaryf8(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v8funcf1(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) int8 v8funcf2(float8 a, float8 b, float8 c, float8 d) {
  return 1.2f * sqrtf8(a / (2.3f * b)) + c < d;
}

int8 v8funcf2loop(float *a0, long n, long m) {
  int8 b = 0;
  for(long j=m;j>0;j--) {
    float8 *a = (float8 *)a0;
    for(long i=n/8;i>0;i--) {
#ifdef LATENCY
      b = v8funcf2(ternaryf8(b, a[0], a[1]),
		   ternaryf8(b, a[2], a[3]),
		   ternaryf8(b, a[4], a[5]),
		   ternaryf8(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v8funcf2(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

static __attribute__((always_inline)) int8 v8funcf3(float8 a0, float8 a1, float8 a2, float8 a3) {
  return a0 / sqrtf8(a1) + a2 < a3;
}

int8 v8funcf3loop(float *a0, long n, long m) {
  int8 b = 0;
  for(long j=m;j>0;j--) {
    float8 *a = (float8 *)a0;
    for(long i=n/8;i>0;i--) {
#ifdef LATENCY
      b = v8funcf3(ternaryf8(b, a[0], a[1]),
		   ternaryf8(b, a[2], a[3]),
		   ternaryf8(b, a[4], a[5]),
		   ternaryf8(b, a[6], a[7]));
#endif
#ifdef THROUGHPUT
      b ^= v8funcf3(a[0], a[2], a[4], a[6]);
#endif
      a += 8;
    }
  }
  return b;
}

//

#include <chrono>
#include <cstdlib>

#pragma clang optimize off

#define N (1024L)
#define M (1024L*256)

static double rand1() { return (rand() + 1) / (double)RAND_MAX; }

__attribute__ ((__aligned__(64))) double a[N *  8];
__attribute__ ((__aligned__(64))) float  f[N * 16];

int main(int argc, char **argv) {
  for(long i=0;i<N*8;i++)  a[i] = rand1();
  for(long i=0;i<N*16;i++) f[i] = rand1();

  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> elapsed_seconds;
  bool b;
  long2 l2;
  long4 l4;
  int4 i4;
  int8 i8;

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
  std::cout << "Scalar double sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = func1loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar double div : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = func2loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar double combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = func3loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar double combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  //

  start = std::chrono::system_clock::now();
  b = funcf0loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar float sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = funcf1loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar float div : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = funcf2loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar float combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  b = funcf3loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << b << "\r";
  elapsed_seconds = end - start;
  std::cout << "Scalar float combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M)) << " nano sec.\n";

  //

  start = std::chrono::system_clock::now();
  l2 = v2func0loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 double sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l2 = v2func1loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 double div : " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l2 = v2func2loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 double combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l2 = v2func3loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l2[0] ^ l2[1]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V2 double combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M/2)) << " nano sec.\n";

  //

  start = std::chrono::system_clock::now();
  i4 = v4funcf0loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i4[0] ^ i4[1] ^ i4[2] ^ i4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 float sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  i4 = v4funcf1loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i4[0] ^ i4[1] ^ i4[2] ^ i4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 float div : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  i4 = v4funcf2loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i4[0] ^ i4[1] ^ i4[2] ^ i4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 float combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  i4 = v4funcf3loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i4[0] ^ i4[1] ^ i4[2] ^ i4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 float combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  //

#if defined(__AVX__)
  start = std::chrono::system_clock::now();
  l4 = v4func0loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 double sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l4 = v4func1loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 double div : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l4 = v4func2loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 double combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  l4 = v4func3loop(a, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (l4[0] ^ l4[1] ^ l4[2] ^ l4[3]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V4 double combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M/4)) << " nano sec.\n";

  //

  start = std::chrono::system_clock::now();
  i8 = v8funcf0loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i8[0] ^ i8[1] ^ i8[2] ^ i8[3] ^ i8[4] ^ i8[5] ^ i8[6] ^ i8[7]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V8 float sqrt : " << (1e+9 * elapsed_seconds.count() / (N*M/8)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  i8 = v8funcf1loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i8[0] ^ i8[1] ^ i8[2] ^ i8[3] ^ i8[4] ^ i8[5] ^ i8[6] ^ i8[7]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V8 float div : " << (1e+9 * elapsed_seconds.count() / (N*M/8)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  i8 = v8funcf2loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i8[0] ^ i8[1] ^ i8[2] ^ i8[3] ^ i8[4] ^ i8[5] ^ i8[6] ^ i8[7]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V8 float combination 1 : " << (1e+9 * elapsed_seconds.count() / (N*M/8)) << " nano sec.\n";

  start = std::chrono::system_clock::now();
  i8 = v8funcf3loop(f, N, M);
  end = std::chrono::system_clock::now();
  std::cout << (i8[0] ^ i8[1] ^ i8[2] ^ i8[3] ^ i8[4] ^ i8[5] ^ i8[6] ^ i8[7]) << "\r";
  elapsed_seconds = end - start;
  std::cout << "V8 float combination 2 : " << (1e+9 * elapsed_seconds.count() / (N*M/8)) << " nano sec.\n";
#endif
}
