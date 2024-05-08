// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_sdl2_opengl3/ folder**
// See imgui_impl_sdl2.cpp for details.

//#include <windows.h>

#include "SDL_timer.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <algorithm>
#include <cstdint>
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <omp.h>

#include "args.hxx"

#include "nbody.hpp"

void cube(float x, float y, float z, float size)
{
    glBegin(GL_LINE_LOOP);
    glVertex3f(x     , y , z);
    glVertex3f(x     , y , z+size);
    glVertex3f(x+size, y , z+size);
    glVertex3f(x+size, y , z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(x, y     , z);
    glVertex3f(x, y     , z+size);
    glVertex3f(x, y+size, z+size);
    glVertex3f(x, y+size, z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(x     , y     , z);
    glVertex3f(x+size, y     , z);
    glVertex3f(x+size, y+size, z);
    glVertex3f(x     , y+size, z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(x     , y+size , z);
    glVertex3f(x     , y+size , z+size);
    glVertex3f(x+size, y+size , z+size);
    glVertex3f(x+size, y+size , z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(x+size, y     , z);
    glVertex3f(x+size, y     , z+size);
    glVertex3f(x+size, y+size, z+size);
    glVertex3f(x+size, y+size, z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(x     , y     , z+size);
    glVertex3f(x+size, y     , z+size);
    glVertex3f(x+size, y+size, z+size);
    glVertex3f(x     , y+size, z+size);
    glEnd();
}

class RingBuffer {
	public:
        RingBuffer(int size) {
            _size = size;
            buffer = new float[get_size()];

            clear();
        }

        ~RingBuffer() {
            delete buffer;
        }

        void write(real_type value) {
            buffer[_bufferPtr] = (float)value;
            if (++_bufferPtr == get_size())  _bufferPtr = 0;
        }

        void clear() {
			std::fill_n(buffer, get_size(), 0.);
            _bufferPtr = 0;
        }

        inline int get_size() { return _size; };
        inline float* get_ptr() { return buffer; };
	private:
		float *buffer;
		int _bufferPtr;
        int _size;
};

// Main code
int main(int argc, char* argv[]) {
    args::ArgumentParser parser("NBody simulation tool.", "");
    args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
    args::Flag gui(parser, "GUI", "Enable GUI", {'g', "gui"});
    args::ValueFlag<int> seed(parser, "seed", "Simulation seed", { "seed" });
    args::ValueFlag<int> size(parser, "size", "Initial object count", { "size" });
    args::ValueFlag<real_type> dT(parser, "delta time", "Delta time for simulation", { "dt" });
    args::ValueFlag<real_type> maxMass(parser, "max mass", "Maximum initial mass", { 'm', "mass" });
    args::ValueFlag<real_type> maxVel(parser, "max velocity", "Maximum initial velocity", {'v', "vel"});

    args::ValueFlag<int> ticks(parser, "tick count", "Tick count for simulation", {'t', "ticks"});

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    } catch (args::ValidationError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    GSimulation simulation;

    if (seed)    simulation.seed    = args::get(seed);
    if (size)    simulation.count   = args::get(size);
    if (dT)      simulation.dTime   = args::get(dT);
    if (maxMass) simulation.maxMass = args::get(maxMass);
    if (maxVel)  simulation.maxVel  = args::get(maxVel);

    simulation.init();

    if (ticks) {
        printf("Ticks |   Time    | Potential E      | Kinetic E        | Full energy      | Deviation| Comp Time|\n");
        for (int i = 0; i < args::get(ticks); i++) {
            simulation.tickTimed();
            printf("%5d | %9.4f | %16.8f | %16.8f | %16.8f | %9.4f| %9.6f|\n", 
                simulation.tickCount,
                simulation.elapsedTime,
                simulation.pEnergy,
                simulation.kEnergy,
                simulation.fEnergy,
                simulation.energy_deviation(),
                simulation.computeTime);
        }
    }

    if (!gui)
        return 0;
    
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    int windowWidth = 1280;
    int windowHeight = 720;

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    int displaySelect = SDL_GetNumVideoDisplays() == 1 ? 0 : 1;
    SDL_Window* window = SDL_CreateWindow("GLQUAKE", SDL_WINDOWPOS_CENTERED_DISPLAY(displaySelect), 
        SDL_WINDOWPOS_CENTERED_DISPLAY(displaySelect), windowWidth, windowHeight, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.10f, 0.15f, 0.20f, 1.00f);
    ImVec4 cameraPosition = ImVec4(1.2f, 1.5f, 1.8f, 1.f);
    GLfloat lightPosition[4] = {2.f, 2.f, 2.f, 1.f};
    GLfloat yFov = 60;
    int subDivision = 10;

    GLfloat sphereSize = 0.05;

    bool volumeShow, originShow, lightShow;
    volumeShow = true;
    originShow = true;
    lightShow = false;

    bool spinCamera = true;
    unsigned int spinStart = 0;
    float spinDivider = 6.;

    bool runSimulation = false;
    int updtatesCount = 1;

    Particle* particles = simulation.getPtr();

    RingBuffer energyHistory(1000);
    RingBuffer energyKHistory(1000);
    RingBuffer energyPHistory(1000);
    RingBuffer deviation(1000);

    int lookAtObject = -1;

    // Main loop
    bool done = false;

    glDepthRange(-1., 1.);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glCullFace(GL_BACK);
    
    while (!done)
    {
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            ImGui::Begin("HomeWorkTask");

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("SDL Time : %f", SDL_GetTicks()/1000.);

            if (ImGui::CollapsingHeader("Render settings")) {
                ImGui::Checkbox("render one's volume", &volumeShow);
                ImGui::Checkbox("show origin point", &originShow);
                ImGui::SliderFloat("sphere sizes", &sphereSize, 0.001, 0.2);
                ImGui::SliderInt("sphere subdivision", &subDivision, 2, 40);
                ImGui::Separator();
                ImGui::DragFloat3("camera position", (float*)&cameraPosition, 0.1f, -2., 2.);
                ImGui::DragInt("camera lookAt", &lookAtObject, 1, -1, simulation.get_count()-1);
                ImGui::SliderFloat("yFov", &yFov, 10, 120);
                ImGui::SliderFloat("spin divider", &spinDivider, 1., 10.);
                if (ImGui::Button("start spin")) {
                    spinStart = SDL_GetTicks();
                    spinCamera = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("stop spin")) {
                    spinStart = SDL_GetTicks();
                    spinCamera = false;
                }
                ImGui::Separator();
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
                ImGui::DragFloat3("light position", (float*)&lightPosition, 0.1f, -2., 2.);
                ImGui::Checkbox("show light point", &lightShow);
            }

            if (ImGui::CollapsingHeader("Generator settings")) {
                ImGui::DragInt("seed", &simulation.seed);
                ImGui::DragInt("object count", &simulation.count, 1, 1, 100000);
                real_type minValue = 0.1;
                ImGui::DragScalar("max mass", ImGuiDataType_Real, &simulation.maxMass, 0.1f, &minValue);
                real_type minValueAcc = 0.;
                ImGui::DragScalar("max velocity", ImGuiDataType_Real, &simulation.maxVel, 0.01f, &minValueAcc);
                ImGui::DragScalar("max acceleration", ImGuiDataType_Real, &simulation.maxAcc, 0.01f, &minValueAcc);
                if (ImGui::Button("regenerate")) {
                    simulation.remove();
                    simulation.init();
                    particles = simulation.getPtr();
                }
            }

            if (ImGui::CollapsingHeader("Simulation settings")) {
                real_type mMin = 0.01;
                real_type mMax = 1.;
                ImGui::SliderScalar("delta time", ImGuiDataType_Real, &simulation.dTime, &mMin, &mMax);
                ImGui::SliderInt("updates per frame", &updtatesCount, 1, 100);
                ImGui::Checkbox("run simulation", &runSimulation);
                ImGui::SameLine();
                if (ImGui::Button("next tick")) {
                    simulation.tickTimed();
                    energyHistory.write(fabs(simulation.fEnergy));
                    energyKHistory.write(fabs(simulation.kEnergy));
                    energyPHistory.write(fabs(simulation.pEnergy));
					deviation.write(simulation.energy_deviation());
                }
                ImGui::Text("Compute time : %f", simulation.computeTime);
            }

            if (ImGui::CollapsingHeader("Simulation view")) {
                ImGui::Text("Elapsed ticks: %d", simulation.tickCount);
                ImGui::Text("Elapsed time : %f", simulation.elapsedTime);
                ImGui::Separator();
                ImGui::Text("     Full energy %f", simulation.fEnergy);
                ImGui::Text("  Kinetic energy %f", simulation.kEnergy);
                ImGui::Text("Potential energy %f", simulation.pEnergy);
                ImGui::Text("    Deviation, %% %f", simulation.energy_deviation());
                ImGui::PlotLines("Full energy", energyHistory.get_ptr(), energyHistory.get_size(), 0, NULL, FLT_MIN, FLT_MAX, ImVec2(300, 60));
                ImGui::PlotLines("Kinetic energy", energyKHistory.get_ptr(), energyKHistory.get_size(), 0, NULL, FLT_MIN, FLT_MAX, ImVec2(300, 60));
                ImGui::PlotLines("Potential energy", energyPHistory.get_ptr(), energyPHistory.get_size(), 0, NULL, FLT_MIN, FLT_MAX, ImVec2(300, 60));
                ImGui::PlotLines("Deviation", deviation.get_ptr(), deviation.get_size(), 0, NULL, FLT_MIN, FLT_MAX, ImVec2(300, 60));
                if (ImGui::Button("clear")) {
                    energyHistory.clear();
                    energyKHistory.clear();
                    energyPHistory.clear();
                    deviation.clear();
				}
                ImGui::SameLine();
                if (ImGui::Button("reset initial energy"))
                    simulation.rewrite_initialEnergy();
            }

            if (ImGui::CollapsingHeader("Object settings")) {
                if (ImGui::Button("save state"))
                    simulation.save_state();

                ImGui::SameLine();

                if (ImGui::Button("load state")) {
                    simulation.read_state();
                    particles = simulation.getPtr();
                }

                ImGui::SameLine();

                if (ImGui::Button("recalculate colors"))
                    simulation.init_color();

                for (int i = 0; i < simulation.get_count(); i++) {
                    if (ImGui::TreeNode((void *)(intptr_t)i, "Object #%d", i)) {
                        ImGui::DragScalarN("Position", ImGuiDataType_Real, particles[i].pos, 3, .05);
                        ImGui::DragScalarN("Velocity", ImGuiDataType_Real, particles[i].vel, 3, .05);
                        ImGui::DragScalarN("Acceleration", ImGuiDataType_Real, particles[i].acc, 3, .05);
                        real_type mMin = 0.;
                        real_type mMax = simulation.get_mass()*10.;
                        ImGui::DragScalar("Mass", ImGuiDataType_Real, &particles[i].mass);
                        ImGui::ColorEdit3("Color", particles[i].color);
                        ImGui::Text("     Full energy %f", particles[i].pEnergy+particles[i].kEnergy);
                        ImGui::Text("  Kinetic energy %f", particles[i].kEnergy);
                        ImGui::Text("Potential energy %f", particles[i].pEnergy);
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::End();
        }

        //Simulate
        if (runSimulation)
            for (int i = 0; i < updtatesCount; i++) {
                simulation.tickTimed();
                energyHistory.write(abs(simulation.fEnergy));
				energyKHistory.write(fabs(simulation.kEnergy));
				energyPHistory.write(fabs(simulation.pEnergy));
				deviation.write(simulation.energy_deviation());
            }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound

        GLUquadric *quad;
        quad = gluNewQuadric();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(yFov, (float)windowWidth/(float)windowHeight, 1./16., 256.);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        double time = (SDL_GetTicks()-spinStart)/1000.;

        GLdouble lookX, lookY, lookZ;
        if (lookAtObject == -1 || lookAtObject >= simulation.get_count()) {
            lookX = .5;
            lookY = .5;
            lookZ = .5;
        } else {
            Particle p = particles[lookAtObject];
            lookX = p.pos[0];
            lookY = p.pos[1];
            lookZ = p.pos[2];
        }

        if (spinCamera)
            gluLookAt(cameraPosition.x * sin(time/spinDivider), cameraPosition.y, cameraPosition.z *  cos(time/spinDivider)
                    , lookX, lookY, lookZ 
                    , 0., 1., 0.);
        else
            gluLookAt(cameraPosition.x, cameraPosition.y, cameraPosition.z
                    , lookX, lookY, lookZ 
                    , 0., 1., 0.);

        glColor3f(1,1,1);
        glDisable(GL_LIGHTING);
        if (originShow)
            gluSphere(quad,1./64.,10,10);
        if (volumeShow)
            cube(0,0,0, 1);

        glPushMatrix();
        glTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
        if (lightShow)
            gluSphere(quad,1./64.,10,10);
        glPopMatrix();

        glEnable(GL_LIGHTING);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
        GLfloat ambient[3] = {.2, .1, .1};
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        //glColor3f(1,0,0);
        GLfloat specular[3] = {1, 1, 1};
        GLfloat shiness[3] = {128};
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, shiness);

        for (int i = 0; i < simulation.get_count(); i++) {
            glPushMatrix();
            Particle obj = particles[i];
            glTranslatef(obj.pos[0], obj.pos[1], obj.pos[2]);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, obj.color);
            gluSphere(quad,sphereSize,subDivision,subDivision);
            glPopMatrix();
        }


        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
