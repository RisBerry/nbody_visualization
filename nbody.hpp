#ifndef GSIMULATION_HPP_
#define GSIMULATION_HPP_

#include <omp.h>

typedef double real_type;
#define ImGuiDataType_Real ImGuiDataType_Double

//typedef float real_type;
//#define ImGuiDataType_Real ImGuiDataType_Float

#pragma pack(push, 1)
struct Particle {
    real_type pos[3];
    real_type vel[3];
    real_type acc[3];  
    real_type mass;
    float color[3]; 

    real_type kEnergy;
    real_type pEnergy;
};
#pragma pack(pop)

class GSimulation {
    public:
        int32_t seed;
        int32_t count;
        real_type maxMass;

        int32_t tickCount;
        real_type dTime;
        real_type elapsedTime;

        real_type maxVel;
        real_type maxAcc;

        real_type kEnergy;
        real_type pEnergy;
        real_type fEnergy;

        double computeTime;

        GSimulation();
        ~GSimulation();
        void remove();
        void init();
        void init_color();

        void tick();

        void rewrite_initialEnergy() { update_energy(); _initialEnergy = fEnergy; }

        void tickTimed() {
            double startTime = omp_get_wtime();
            tick();
            double endTime = omp_get_wtime();
            computeTime = endTime - startTime;
        }

        Particle* getPtr();
        int get_count()      {return _ncount;}
        real_type get_dt()   {return dTime;}
        real_type get_mass() {return _nmaxmass;}

        real_type energy_deviation() { return abs((_initialEnergy - fEnergy) * -100. / _initialEnergy); }

        void save_state();
        void read_state();
    private:
        int _ncount;
        int _nseed;
        real_type _nmaxmass;
        real_type _initialEnergy;
        Particle *particles;
        void init_pos();
        void init_mass();
        void update_energy();
        //void update_acc(real_type dTime);
        //void update_vel(real_type dTime);
        //void update_pos(real_type dTime);
        //real_type max_mass();
		const double softeningSquared = 1e-3;
		const double G = 6.67259e-11;
};

#endif
