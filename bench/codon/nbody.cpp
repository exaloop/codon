#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

const double PI = 3.14159265358979323;
const double SOLAR_MASS = 4 * PI * PI;
const double DAYS_PER_YEAR = 365.24;

struct Body {
  std::vector<double> r, v;
  double m;
};

std::unordered_map<std::string, Body> BODIES = {
    {"sun", {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, SOLAR_MASS}},
    {"jupiter",
     {{4.84143144246472090e+00, -1.16032004402742839e+00, -1.03622044471123109e-01},
      {1.66007664274403694e-03 * DAYS_PER_YEAR, 7.69901118419740425e-03 * DAYS_PER_YEAR,
       -6.90460016972063023e-05 * DAYS_PER_YEAR},
      9.54791938424326609e-04 * SOLAR_MASS}},
    {"saturn",
     {{8.34336671824457987e+00, 4.12479856412430479e+00, -4.03523417114321381e-01},
      {-2.76742510726862411e-03 * DAYS_PER_YEAR,
       4.99852801234917238e-03 * DAYS_PER_YEAR,
       2.30417297573763929e-05 * DAYS_PER_YEAR},
      2.85885980666130812e-04 * SOLAR_MASS}},
    {"uranus",
     {{1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01},
      {2.96460137564761618e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR,
       -2.96589568540237556e-05 * DAYS_PER_YEAR},
      4.36624404335156298e-05 * SOLAR_MASS}},
    {"neptune",
     {{1.53796971148509165e+01, -2.59193146099879641e+01, 1.79258772950371181e-01},
      {2.68067772490389322e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR,
       -9.51592254519715870e-05 * DAYS_PER_YEAR},
      5.15138902046611451e-05 * SOLAR_MASS}},
};

template <typename K, typename V> auto values(std::unordered_map<K, V> &m) {
  std::vector<V *> v;
  v.reserve(m.size());
  for (auto &e : m)
    v.push_back(&e.second);
  return v;
}

template <typename T> auto combinations(const std::vector<T> &v) {
  std::vector<std::pair<T, T>> p;
  auto n = v.size();
  p.reserve(n);
  for (auto i = 0; i < n - 1; i++)
    for (auto j = i + 1; j < n; j++)
      p.push_back({v[i], v[j]});
  return p;
}

std::vector<Body *> SYSTEM = values(BODIES);
auto PAIRS = combinations(SYSTEM);

void advance(double dt, int n, std::vector<Body *> &bodies = SYSTEM,
             std::vector<std::pair<Body *, Body *>> &pairs = PAIRS) {
  for (int i = 0; i < n; i++) {
    for (auto &pair : pairs) {
      double x1 = pair.first->r[0], y1 = pair.first->r[1], z1 = pair.first->r[2];
      auto &v1 = pair.first->v;
      double m1 = pair.first->m;
      double x2 = pair.second->r[0], y2 = pair.second->r[1], z2 = pair.second->r[2];
      auto &v2 = pair.second->v;
      double m2 = pair.second->m;
      double dx = x1 - x2, dy = y1 - y2, dz = z1 - z2;
      double mag = dt * std::pow((dx * dx + dy * dy + dz * dz), -1.5);
      double b1m = m1 * mag;
      double b2m = m2 * mag;
      v1[0] -= dx * b2m;
      v1[1] -= dy * b2m;
      v1[2] -= dz * b2m;
      v2[0] += dx * b1m;
      v2[1] += dy * b1m;
      v2[2] += dz * b1m;
    }

    for (auto *body : bodies) {
      auto &r = body->r;
      double vx = body->v[0], vy = body->v[1], vz = body->v[2];
      r[0] += dt * vx;
      r[1] += dt * vy;
      r[2] += dt * vz;
    }
  }
}

void report_energy(std::vector<Body *> &bodies = SYSTEM,
                   std::vector<std::pair<Body *, Body *>> &pairs = PAIRS,
                   double e = 0.0) {
  for (auto &pair : pairs) {
    double x1 = pair.first->r[0], y1 = pair.first->r[1], z1 = pair.first->r[2];
    auto &v1 = pair.first->v;
    double m1 = pair.first->m;
    double x2 = pair.second->r[0], y2 = pair.second->r[1], z2 = pair.second->r[2];
    auto &v2 = pair.second->v;
    double m2 = pair.second->m;
    double dx = x1 - x2, dy = y1 - y2, dz = z1 - z2;
    e -= (m1 * m2) / std::pow((dx * dx + dy * dy + dz * dz), 0.5);
  }

  for (auto *body : bodies) {
    double vx = body->v[0], vy = body->v[1], vz = body->v[2];
    double m = body->m;
    e += m * (vx * vx + vy * vy + vz * vz) / 2.;
  }

  std::cout << e << std::endl;
}

void offset_momentum(Body &ref, std::vector<Body *> &bodies = SYSTEM, double px = 0.0,
                     double py = 0.0, double pz = 0.0) {
  for (auto *body : bodies) {
    double vx = body->v[0], vy = body->v[1], vz = body->v[2];
    double m = body->m;
    px -= vx * m;
    py -= vy * m;
    pz -= vz * m;
  }

  auto &v = ref.v;
  double m = ref.m;
  v[0] = px / m;
  v[1] = py / m;
  v[2] = pz / m;
}

} // namespace

int main(int argc, char *argv[]) {
  using clock = std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto t = clock::now();
  std::string ref = "sun";
  offset_momentum(BODIES[ref]);
  report_energy();
  advance(0.01, std::atoi(argv[1]));
  report_energy();
  std::cout << (duration_cast<milliseconds>(clock::now() - t).count() / 1e3)
            << std::endl;
}
