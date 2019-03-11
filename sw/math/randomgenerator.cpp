#include "randomgenerator.h"
#include <random>
#include <iostream>
#include "stdint.h"
#include "auxiliary.h"

random_generator::random_generator()
{
  int temp;
  uintptr_t t = (uintptr_t)&temp;
  srand(t);
};

float random_generator::uniform_float(float min, float max)
{
  random_device rd;  //Will be used to obtain a seed for the random number engine
  mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()v
  uniform_real_distribution<> dis(min, max);
  return dis(gen);
  // return min + ((float)rand() / (RAND_MAX / (max - min))) ;
};

int random_generator::uniform_int(int min, int max)
{
  return min + (rand() / (RAND_MAX / (max - min))) ;
};

// <<complete Box-Muller function>>=
// based on:
// http://en.literateprograms.org/Box-Muller_transform_%28C%29#chunk%20def:scale%20and%20translate%20to%20get%20desired%20mean%20and%20standard%20deviation
//
// Polar form of Box-Muller transform:
// http://www.design.caltech.edu/erik/Misc/Gaussian.html
float random_generator::gaussian_float(float mean, float stddev)
{
  static float n2 = 0.0;
  static int n2_cached = 0;
  if (!n2_cached) {
    float x, y, r;
    do {
      x = 2.0 * rand() / RAND_MAX - 1;
      y = 2.0 * rand() / RAND_MAX - 1;
      r = x * x + y * y;
    }  //
    while (r == 0.0 || r > 1.0); {
      float d = sqrt(-2.0 * log(r) / r);
      float n1 = x * d;
      n2 = y * d;
      float result = n1 * stddev + mean;
      n2_cached = 1;
      return result;
    }
  } else {
    n2_cached = 0;
    return n2 * stddev + mean;
  }

};

vector<float> random_generator::gaussian_float_vector(const int &length, const float &mean, const float &std)
{
  // Generate the random vector
  vector<float> v(length, 0);
  for (uint8_t i = 0; i < length; i++) {
    v[i] = gaussian_float(mean, std);
  }

  return v;
}

vector<float> random_generator::uniform_float_vector(const int &length, const float &min, const float &max)
{
  // Generate the random vector
  vector<float> v(length, 0);
  for (uint8_t i = 0; i < length; i++) {
    v[i] = uniform_float(min, max);
  }

  return v;
}
