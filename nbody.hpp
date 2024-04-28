#ifndef GSIMULATION_HPP_
#define GSIMULATION_HPP_

typedef double real_type;

struct Particle {
  public:
    Particle() { init();}
    void init() {
      pos[0] = 0.; pos[1] = 0.; pos[2] = 0.;
      vel[0] = 0.; vel[1] = 0.; vel[2] = 0.;
      acc[0] = 0.; acc[1] = 0.; acc[2] = 0.;
      mass   = 0.;
      color[0] = 1.; color[1] = 0.; color[2] = 0.;
      kEnergy = 0;
      pEnergy = 0;
    }
    real_type pos[3];
    real_type vel[3];
    real_type acc[3];  
    real_type mass;
    float color[3]; 

    real_type kEnergy;
    real_type pEnergy;
};

class GSimulation {
    public:
        int seed;
        int count;
        real_type maxMass;

        int tickCount;
        real_type dTime;
        real_type elapsedTime;

        real_type maxVel;
        real_type maxAcc;

        real_type kEnergy;
        real_type pEnergy;

        GSimulation();
        ~GSimulation();
        void remove();
        void init();
        void init_color();

        void tick();

        Particle* getPtr();
        int get_count() const {return _ncount;}
        real_type get_dt() const {return dTime;}
        real_type get_mass() const {return _nmaxmass;}
    private:
        int _ncount;
        int _nseed;
        real_type _nmaxmass;
        Particle *particles;
        void init_pos();
        void init_mass();
        //real_type max_mass();
};

#endif
