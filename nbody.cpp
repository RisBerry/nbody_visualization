#include <random>
#include "nbody.hpp"

void hsv2rgb(float hue, float saturation, float value, 
        float* r, float* g, float* b) {
    double      hh, p, q, t, ff;
    long        i;

    if(saturation <= 0.0) {       // < is bogus, just shuts up warnings
        *r = value;
        *g = value;
        *b = value;
        return;
    }
    hh = hue;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = value * (1.0 - saturation);
    q = value * (1.0 - (saturation * ff));
    t = value * (1.0 - (saturation * (1.0 - ff)));

    switch(i) {
    case 0:
        *r = value;
        *g = t;
        *b = p;
        break;
    case 1:
        *r = q;
        *g = value;
        *b = p;
        break;
    case 2:
        *r = p;
        *g = value;
        *b = t;
        break;

    case 3:
        *r = p;
        *g = q;
        *b = value;
        break;
    case 4:
        *r = t;
        *g = p;
        *b = value;
        break;
    case 5:
    default:
        *r = value;
        *g = p;
        *b = q;
        break;
    }
}


GSimulation::GSimulation() {
    seed = 42;
    count = 1000;
    maxMass = 1.;
    dTime = 0.02;

    maxVel = 0.;
    maxAcc = 0.;
}

void GSimulation::init() {
    _ncount = count;
    _nmaxmass = maxMass;
    _nseed = seed;

    tickCount = 0;
    elapsedTime = 0;

    particles = new Particle[get_count()];

    init_pos();
    init_mass();
    init_color();
}

void GSimulation::remove() {
    delete particles;
}

Particle* GSimulation::getPtr() {
    return particles;
}

void GSimulation::init_pos()  {
  std::random_device rd;	//random number generator
  std::mt19937 gen(_nseed);      
  std::uniform_real_distribution<real_type> unif_d(0,1.0);
  std::uniform_real_distribution<real_type> unif_r(-1., 1.);
  
  for(int i=0; i<get_count(); ++i)
  {
    particles[i].pos[0] = unif_d(gen);
    particles[i].pos[1] = unif_d(gen);
    particles[i].pos[2] = unif_d(gen);

    particles[i].vel[0] = unif_r(gen) * maxVel;
    particles[i].vel[1] = unif_r(gen) * maxVel;
    particles[i].vel[2] = unif_r(gen) * maxVel;

    particles[i].acc[0] = unif_r(gen) * maxAcc;
    particles[i].acc[1] = unif_r(gen) * maxAcc;
    particles[i].acc[2] = unif_r(gen) * maxAcc;
  }
}

void GSimulation::init_mass()  {
  std::random_device rd;	//random number generator
  std::mt19937 gen(_nseed);      
  std::uniform_real_distribution<real_type> unif_d(0, _nmaxmass);
  
  for(int i=0; i<get_count(); ++i)
  {
    particles[i].mass = unif_d(gen);
  }
}

void GSimulation::init_color()  {
    real_type mass = _nmaxmass;
    for(int i=0; i<get_count(); ++i) {
        float r,g,b;
        hsv2rgb((float)i*360./get_count() , 1.0, (mass-particles[i].mass)/mass + 0.2, &r, &g, &b);
        particles[i].color[0] = r;
        particles[i].color[1] = g;
        particles[i].color[2] = b;
    }
}

void GSimulation::tick() {
    elapsedTime += dTime;
    tickCount++;

    int n = get_count();
    real_type dt = get_dt();

    const double softeningSquared = 1e-6;
    // prevents explosion in the case the particles are really close to each other 
    const double G = 6.67259e-11;

    kEnergy = 0.;
    pEnergy = 0.;

    for (int i = 0; i < n; i++) { // update acceleration
        particles[i].acc[0] = 0.;
        particles[i].acc[1] = 0.;
        particles[i].acc[2] = 0.;

        particles[i].pEnergy = 0.;
        for (int j = 0; j < n; j++) {
            if (i == j)
                continue;

            real_type dx, dy, dz;
            real_type distanceSqr = 0.0;
            real_type distanceInv = 0.0;

            dx = particles[j].pos[0] - particles[i].pos[0];	//1flop
            dy = particles[j].pos[1] - particles[i].pos[1];	//1flop	
            dz = particles[j].pos[2] - particles[i].pos[2];	//1flop

            distanceSqr = dx*dx + dy*dy + dz*dz + softeningSquared;	//6flops
            distanceInv = 1.0 / sqrt(distanceSqr);			//1div+1sqrt

            real_type force = G * particles[j].mass * distanceInv * distanceInv * distanceInv;
            particles[i].acc[0] += dx * force;
            particles[i].acc[1] += dy * force;
            particles[i].acc[2] += dz * force;

            particles[i].pEnergy += force * particles[i].mass * (dx*dx + dy*dy + dz*dz);

        }
        pEnergy += particles[i].pEnergy;
    }

    for (int i = 0; i < n; ++i) { // update position and velocity 
        particles[i].vel[0] += particles[i].acc[0] * dt;	//2flops
        particles[i].vel[1] += particles[i].acc[1] * dt;	//2flops
        particles[i].vel[2] += particles[i].acc[2] * dt;	//2flops

        particles[i].pos[0] += particles[i].vel[0] * dt;	//2flops
        particles[i].pos[1] += particles[i].vel[1] * dt;	//2flops
        particles[i].pos[2] += particles[i].vel[2] * dt;	//2flops

        particles[i].kEnergy = particles[i].mass * ( 
                particles[i].vel[0] * particles[i].vel[0] +
                particles[i].vel[1] * particles[i].vel[1] +
                particles[i].vel[2] * particles[i].vel[2]
                ) * .5;

        kEnergy += particles[i].kEnergy;
    }
}

GSimulation::~GSimulation() {
    delete particles;
}
