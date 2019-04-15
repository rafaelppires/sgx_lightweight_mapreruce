#include <cmath>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#define N 7
#define PERCIRCLE 1000
#define YMAX 1000
#define XMAX 700
#define RADIUSMIN 100
#define RADIUSMAX 800

void set_seed() {
  std::ifstream rand("/dev/urandom");
  unsigned int seed;
  rand.read((char *)&seed, sizeof(seed));
  srand(seed);
}

double torad(int deg) {
    return M_PI * deg / 180.;
}

int main() {
  set_seed();
  double centers[N][3];
  for (int i = 0; i < N; ++i) {
    double offsetx, offsety;

    if(i == 0) {
        offsetx = 700; offsety = 1000;
    } else if(i == N) {
        offsetx = -700; offsety = -1000;
    } else if(i==N/2) {
        offsetx = 0; offsety = 1000;
    } else {
        offsetx = offsety = 0;
    }

    centers[i][0] = offsetx + rand() % XMAX;
    centers[i][1] = offsety + rand() % YMAX;
    centers[i][2] = RADIUSMIN + (rand() % (RADIUSMAX - RADIUSMIN));
    printf("R: %f  C: (%f, %f)\n", centers[i][2], centers[i][0], centers[i][1]);
  }

  std::ofstream fcentres("c.dat"), fpoints("p.dat");
  for (int i = 0; i < N; ++i) {
    fcentres << (rand() % XMAX) << " " << (rand() % YMAX) << "\n";
    for (int j = 0; j < PERCIRCLE; ++j) {
      double ang = torad(rand() % 360),
             rds = (0.5+(rand()%50)/100.) * (rand() % 10000) / 1e4;
      fpoints << int(centers[i][0] + (rds*centers[i][2]) * cos(ang)) << " "
              << int(centers[i][1] + (rds*centers[i][2]) * sin(ang)) << "\n";
    }
  }
}

