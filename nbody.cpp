#include <random>
#include "nbody.hpp"

//#define advisorAnnotations

#ifdef advisorAnnotations
#include <advisor-annotate.h>
#endif

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

    update_energy();

    _initialEnergy = fEnergy;
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
  std::uniform_real_distribution<real_type> unif_d(0. , 1.);
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
        hsv2rgb((float)i*360./get_count() , 1.0, (mass-particles[i].mass)*.8/mass + .2, &r, &g, &b);
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

    //const double softeningSquared = 1e-3;
    //prevents explosion in the case the particles are really close to each other 
    //const double G = 6.67259e-11;

    kEnergy = 0.;
    pEnergy = 0.;

    double _tKe = 0.;
    double _tPe = 0.;

#ifdef advisorAnnotations
    ANNOTATE_SITE_BEGIN(pos_update);
#else
	#pragma omp parallel for reduction(+ : _tKe)
#endif
    for (int i = 0; i < n; ++i) { // update position and velocity 
#ifdef advisorAnnotations
        ANNOTATE_ITERATION_TASK(pos_task);
        {
#endif
            real_type coef = 1.5;

            particles[i].vel[0] += particles[i].acc[0] * dt * coef;	//2flops
            particles[i].vel[1] += particles[i].acc[1] * dt * coef;	//2flops
            particles[i].vel[2] += particles[i].acc[2] * dt * coef;	//2flops

            particles[i].pos[0] += particles[i].vel[0] * dt * coef;	//2flops
            particles[i].pos[1] += particles[i].vel[1] * dt * coef;	//2flops
            particles[i].pos[2] += particles[i].vel[2] * dt * coef;	//2flops

            particles[i].kEnergy = particles[i].mass * (
                particles[i].vel[0] * particles[i].vel[0] +
                particles[i].vel[1] * particles[i].vel[1] +
                particles[i].vel[2] * particles[i].vel[2]
                ) * .5;

            _tKe += particles[i].kEnergy;
#ifdef advisorAnnotations
        }
        //ANNOTATE_TASK_END();
#endif
    }
#ifdef advisorAnnotations
    ANNOTATE_SITE_END();
#endif

#ifdef advisorAnnotations
    ANNOTATE_SITE_BEGIN(acc_update);
#else
	#pragma omp parallel for reduction(+ : _tPe)
#endif
    for (int i = 0; i < n; i++) { // update acceleration
#ifdef advisorAnnotations
        ANNOTATE_ITERATION_TASK(acc_task);
        {
#endif
            particles[i].acc[0] = 0.;
            particles[i].acc[1] = 0.;
            particles[i].acc[2] = 0.;

            particles[i].pEnergy = 0.;

            for (int j = 0; j < n; j++) {
                if (i == j)
                    continue;

                real_type dx, dy, dz;
                real_type _dx, _dy, _dz;
                real_type distanceSqr = 0.0;
                real_type distanceInv = 0.0;

                dx = particles[j].pos[0] - particles[i].pos[0];	//1flop
                dy = particles[j].pos[1] - particles[i].pos[1];	//1flop	
                dz = particles[j].pos[2] - particles[i].pos[2];	//1flop

                distanceSqr = dx * dx + dy * dy + dz * dz;	//6flops

                double _distanceSqr = distanceSqr;
                //FAILSAFE part 1
                if (distanceSqr <= softeningSquared)
                    _distanceSqr = softeningSquared;	//6flops

                distanceInv = 1.0 / sqrt(_distanceSqr);			//1div+1sqrt
                real_type force1 = G * particles[j].mass * distanceInv * distanceInv * distanceInv; //* 0.5;
                particles[i].pEnergy -= .5 * force1 * particles[i].mass * (_distanceSqr);

                //2nd part
                real_type tmpAcc[3];
                real_type tmpVel[3];
                real_type tmpPos[3];

                tmpAcc[0] = dx * (force1);
                tmpAcc[1] = dy * (force1);
                tmpAcc[2] = dz * (force1);

                tmpVel[0] = particles[i].vel[0] + tmpAcc[0] * dt;// *.5;
                tmpVel[1] = particles[i].vel[1] + tmpAcc[1] * dt;// * .5;
                tmpVel[2] = particles[i].vel[2] + tmpAcc[2] * dt;// * .5;

                tmpPos[0] = particles[i].pos[0] + tmpVel[0] * dt;// * .5;
                tmpPos[1] = particles[i].pos[1] + tmpVel[1] * dt;// * .5;
                tmpPos[2] = particles[i].pos[2] + tmpVel[2] * dt;// *.5;

                _dx = particles[j].pos[0] - tmpPos[0];	//1flop
                _dy = particles[j].pos[1] - tmpPos[1];	//1flop	
                _dz = particles[j].pos[2] - tmpPos[2];	//1flop

                _distanceSqr = _dx * _dx + _dy * _dy + _dz * _dz;	//6flops

                //FAILSAFE part 2
                if (_distanceSqr <= softeningSquared)
                    _distanceSqr = softeningSquared;	//6flops

                //real_type force2 = 0.;
                //_distanceSqr *= 4.;
                distanceInv = 1.0 / sqrt(_distanceSqr);			//1div+1sqrt
                real_type force2 = G * particles[j].mass * distanceInv * distanceInv * distanceInv;
                particles[i].pEnergy -= .5 * force2 * particles[i].mass * (_distanceSqr);

                //FAILSAFE part 3
                if (distanceSqr <= softeningSquared && _distanceSqr <= softeningSquared)
                    continue;

                particles[i].acc[0] += (dx * force1 + _dx * force2) * .5;
                particles[i].acc[1] += (dy * force1 + _dy * force2) * .5;
                particles[i].acc[2] += (dz * force1 + _dz * force2) * .5;
            }
            _tPe += particles[i].pEnergy * .5;
#ifdef advisorAnnotations
        }
        //ANNOTATE_TASK_END();
#endif
    }
#ifdef advisorAnnotations
    ANNOTATE_SITE_END();
#endif

    pEnergy = _tPe;
    kEnergy = _tKe;
	fEnergy = pEnergy + kEnergy;
}

void GSimulation::update_energy() {
    int n = get_count();
	real_type dt = get_dt();

	// prevents explosion in the case the particles are really close to each other 
	kEnergy = 0.;
	pEnergy = 0.;

    double _tKe = 0.;
    double _tPe = 0.;

	#pragma omp parallel for reduction(+ : _tKe, _tPe)
	for (int i = 0; i < n; ++i) { // update position and velocity 
		particles[i].kEnergy = particles[i].mass * (
			particles[i].vel[0] * particles[i].vel[0] +
			particles[i].vel[1] * particles[i].vel[1] +
			particles[i].vel[2] * particles[i].vel[2]
			) * .5;

		_tKe += particles[i].kEnergy;

		particles[i].pEnergy = 0.;

		for (int j = 0; j < n; j++) {
			if (i == j)
				continue;

			real_type dx, dy, dz;
            real_type _dx, _dy, _dz;
			real_type distanceSqr = 0.0;
			real_type distanceInv = 0.0;

			dx = particles[j].pos[0] - particles[i].pos[0];	//1flop
			dy = particles[j].pos[1] - particles[i].pos[1];	//1flop	
			dz = particles[j].pos[2] - particles[i].pos[2];	//1flop

			distanceSqr = dx * dx + dy * dy + dz * dz;	//6flops

			double _distanceSqr = distanceSqr;
			//FAILSAFE part 1
			if (distanceSqr <= softeningSquared)
				_distanceSqr = softeningSquared;	//6flops

			distanceInv = 1.0 / sqrt(_distanceSqr);			//1div+1sqrt
            real_type force1 = G * particles[j].mass * distanceInv * distanceInv * distanceInv; //* 0.5;
			particles[i].pEnergy -= force1 * particles[i].mass * (_distanceSqr); //* 0.5;

            /*
            //2nd part
            real_type tmpAcc[3];
            real_type tmpVel[3];
            real_type tmpPos[3];

            tmpAcc[0] = dx * (force1);
            tmpAcc[1] = dy * (force1);
            tmpAcc[2] = dz * (force1);

            tmpVel[0] = particles[i].vel[0] + tmpAcc[0] * dt;// *.5;
            tmpVel[1] = particles[i].vel[1] + tmpAcc[1] * dt;// * .5;
            tmpVel[2] = particles[i].vel[2] + tmpAcc[2] * dt;// * .5;

            tmpPos[0] = particles[i].pos[0] + tmpVel[0] * dt;// * .5;
            tmpPos[1] = particles[i].pos[1] + tmpVel[1] * dt;// * .5;
            tmpPos[2] = particles[i].pos[2] + tmpVel[2] * dt;// *.5;

            _dx = particles[j].pos[0] - tmpPos[0];	//1flop
            _dy = particles[j].pos[1] - tmpPos[1];	//1flop	
            _dz = particles[j].pos[2] - tmpPos[2];	//1flop

            _distanceSqr = _dx*_dx + _dy*_dy + _dz*_dz;	//6flops

            //FAILSAFE part 2
            if (_distanceSqr <= softeningSquared)
				_distanceSqr = softeningSquared;	//6flops

            //real_type force2 = 0.;
            //_distanceSqr *= 4.;
            distanceInv = 1.0 / sqrt(_distanceSqr);			//1div+1sqrt
            real_type force2 = G * particles[j].mass * distanceInv * distanceInv * distanceInv;
            particles[i].pEnergy -= .5 * force2 * particles[i].mass * (_distanceSqr);
            */
		}
		_tPe += particles[i].pEnergy * .5;
    }

    kEnergy = _tKe;
    pEnergy = _tPe;
	fEnergy = pEnergy + kEnergy;
}

void GSimulation::save_state() {
    auto fptr = fopen("state.bin", "wb");

    if (fptr == NULL)
        return;

    fwrite(&_ncount, sizeof(int32_t), 1, fptr);
    fwrite(&tickCount, sizeof(int32_t), 1, fptr);
    fwrite(&elapsedTime, sizeof(real_type), 1, fptr);
    fwrite(&_initialEnergy, sizeof(real_type), 1, fptr);

    for (int i = 0; i < get_count(); i++) {
        fwrite(&particles[i], sizeof(struct Particle), 1, fptr);
    }

    fclose(fptr);
}

void GSimulation::read_state() {
    auto fptr = fopen("state.bin", "rb");

    if (fptr == NULL)
        return;

    fread(&_ncount, sizeof(int32_t), 1, fptr);
    fread(&tickCount, sizeof(int32_t), 1, fptr);
    fread(&elapsedTime, sizeof(real_type), 1, fptr);
    fread(&_initialEnergy, sizeof(real_type), 1, fptr);

    remove();
    particles = new Particle[get_count()];

    for (int i = 0; i < get_count(); i++) {
        fread(&particles[i], sizeof(struct Particle), 1, fptr);
    }

    fclose(fptr);

    update_energy();
    _initialEnergy = fEnergy;
}

GSimulation::~GSimulation() {
    delete particles;
}
