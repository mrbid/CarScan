/*
    James William Fletcher (github.com/mrbid)
        December 2022

    Most people don't detect their delta time step
    they just use the current frame delta.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/file.h>
#include <stdint.h>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wunused-result"

#define uint GLushort
#define sint GLshort
#define f32 GLfloat

#include "inc/gl.h"
#define GLFW_INCLUDE_NONE
#include "inc/glfw3.h"

#ifndef __x86_64__
    #define NOSSE
#endif

#define GL_DEBUG
//#define REGULAR_PHONG   // or Blinn-Phong by default

#include "inc/esAux3.h"
#include "inc/res.h"
#include "car.h"

//*************************************
// globals
//*************************************
GLFWwindow* window;
uint winw = 1024;
uint winh = 768;
double t = 0;   // time
f32 dt = 0;     // delta time
double fc = 0;  // frame count
double lfct = 0;// last frame count time
f32 aspect;
double x,y,lx,ly;
double rww, ww, rwh, wh, ww2, wh2;
double uw, uh, uw2, uh2; // normalised pixel dpi
double maxfps = 144.0;

// render state id's
GLint projection_id;
GLint modelview_id;
GLint normalmat_id = -1;
GLint position_id;
GLint lightpos_id;
GLint solidcolor_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;

// render state matrices
mat projection;
mat view;

// models
ESModel mdlCar;

// camera vars
#define FAR_DISTANCE 333.f
uint focus_cursor = 0;
double sens = 0.001f;
f32 xrot = 0.f;
f32 yrot = 1.5f; // face on until [1]
f32 zoom = -4.0f; // -6.0f / -26.0f

// sim vars
vec lightpos = {0.f, 0.f, 0.f};

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
// float urandf()
// {
//     static const float RECIP_FLOAT_UINT64_MAX = 1.f/(float)UINT64_MAX;
//     int f = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
//     uint64_t s = 0;
//     read(f, &s, sizeof(uint64_t));
//     close(f);
//     return ((float)s) * RECIP_FLOAT_UINT64_MAX;
// }
// float urandfc()
// {
//     static const float RECIP_FLOAT_UINT64_MAX = 2.f/(float)UINT64_MAX;
//     int f = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
//     uint64_t s = 0;
//     read(f, &s, sizeof(uint64_t));
//     close(f);
//     return (((float)s) * RECIP_FLOAT_UINT64_MAX)-1.f;
// }

//*************************************
// update & render
//*************************************
void main_loop(uint dotick)
{
//*************************************
// camera
//*************************************
    if(focus_cursor == 1)
    {
        glfwGetCursorPos(window, &x, &y);

        xrot += (ww2-x)*sens;
        yrot += (wh2-y)*sens;

        if(yrot > 1.5f)
            yrot = 1.5f;
        if(yrot < 0.5f)
            yrot = 0.5f;

        glfwSetCursorPos(window, ww2, wh2);
    }

    mIdent(&view);
    mTranslate(&view, 0.f, -1.f, zoom);
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 0.f, 1.f);

//*************************************
// render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (GLfloat*) &view.m[0][0]);
    if(normalmat_id != -1)
    {
        mat inverted, normalmat;
        mInvert(&inverted.m[0][0], &view.m[0][0]);
        mTranspose(&normalmat, &inverted);
        glUniformMatrix4fv(normalmat_id, 1, GL_FALSE, (GLfloat*) &normalmat.m[0][0]);
    }
    glDrawElements(GL_TRIANGLES, car_numind, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
}

//*************************************
// Input Handelling
//*************************************
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_F)
        {
            if(t-lfct > 2.0)
            {
                char strts[16];
                timestamp(&strts[0]);
                const double nfps = fc/(t-lfct);
                printf("[%s] FPS: %g\n", strts, nfps);
                maxfps = nfps;
                dt = 1.0f / (float)maxfps;
                lfct = t;
                fc = 0;
            }
        }
        else if(key == GLFW_KEY_Z)
        {
            shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
            glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
            glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
            glUniform1f(opacity_id, 1.0f);
            normalmat_id = -1;
        }
        else if(key == GLFW_KEY_X)
        {
            shadePhong3(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
            glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
            glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
            glUniform1f(opacity_id, 1.0f);
        }
        else if(key == GLFW_KEY_A)
            glDisable(GL_BLEND);
        else if(key == GLFW_KEY_S)
            glEnable(GL_BLEND);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(yoffset < 0)
        zoom += 0.06f * zoom;
    else if(yoffset > 0)
        zoom -= 0.06f * zoom;
    
    if(zoom > -3.f){zoom = -3.f;}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
        {
            focus_cursor = 1 - focus_cursor;
            if(focus_cursor == 0)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            glfwSetCursorPos(window, ww2, wh2);
            glfwGetCursorPos(window, &ww2, &wh2);
        }
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width;
    winh = height;

    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = winw;
    wh = winh;
    rww = 1/ww;
    rwh = 1/wh;
    ww2 = ww/2;
    wh2 = wh/2;
    uw = (double)aspect / ww;
    uh = 1 / wh;
    uw2 = (double)aspect / ww2;
    uh2 = 1 / wh2;

    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 0.01f, FAR_DISTANCE);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // allow framerate cap
    if(argc >= 3){maxfps = atof(argv[2]);}

    // help
    printf("----\n");
    printf("3D Scan of Car Wreck\nhttps://sketchfab.com/3d-models/green-car-wreck-a5b233dfe0024ff0b9d33f5469b10dc8\n");
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("----\n");
    printf("Argv(2): msaa, maxfps\n");
    printf("e.g; ./uc 16 60\n");
    printf("----\n");
    printf("Left Click = Focus toggle camera control\n");
    printf("F = FPS to console.\n");
    printf("A = Opaque.\n");
    printf("S = Transparent.\n");
    printf("Z = Lambertian Shading.\n");
    printf("X = Phong Shading.\n");
    printf("----\n");

    // init glfw
    if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    window = glfwCreateWindow(winw, winh, "Detecting frame rate...", NULL, NULL);
    if(!window)
    {
        printf("glfwCreateWindow() failed.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(0); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});

//*************************************
// projection
//*************************************

    window_size_callback(window, winw, winh);

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND MODEL *****
    esBind(GL_ARRAY_BUFFER, &mdlCar.vid, car_vertices, sizeof(car_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCar.nid, car_normals, sizeof(car_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCar.cid, car_colors, sizeof(car_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlCar.iid, car_indices, sizeof(car_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader programs
//*************************************

    //makeAllShaders();
    makeLambert3();
    makePhong3();

//*************************************
// configure render options
//*************************************

    // standard stuff
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.13f, 0.13f, 0.13f, 0.0f);

    // setup shader
    shadePhong3(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 0.5f);
    
    // bind to render
    glBindBuffer(GL_ARRAY_BUFFER, mdlCar.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlCar.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlCar.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlCar.iid);

    // enable debugging
    esDebug(1);

//*************************************
// execute update / render loop
//*************************************

    // init
    t = glfwGetTime();
    lfct = t;
    dt = 1.0f / (float)maxfps; // fixed timestep delta-time

    // detect frame rate
    time_t ac = time(0) + 1;
    uint fct = 0;
    
    // fps accurate event loop
    useconds_t wait_interval = 1000000 / maxfps; // fixed timestep
    if(wait_interval == 0){wait_interval = 100;} // limited to 10,000 FPS maximum
    useconds_t wait = wait_interval;
    while(!glfwWindowShouldClose(window))
    {
        usleep(wait);
        t = glfwGetTime();

        // auto correct max fps
        if(time(0) > ac)
        {
            const double nfps = fc/(t-lfct);
            if(fabs(nfps - maxfps) > 6.f)
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] maxfps auto corrected from %.2f to %.2f.\n", strts, maxfps, nfps);
                char titlestr[256];
                sprintf(titlestr, "FPS: %.2f", nfps);
                glfwSetWindowTitle(window, titlestr);
            }
            maxfps = nfps;
            char titlestr[256];
            sprintf(titlestr, "FPS: %.2f", nfps);
            glfwSetWindowTitle(window, titlestr);
            dt = 1.0f / (float)maxfps;
            ac = time(0) + 6;
            fct = 1;
        }
        
        // don't tick our internal state until we know we have a decent delta-time [dt]
        glfwPollEvents();
        main_loop(fct);

        // accurate fps
        wait = wait_interval - (useconds_t)((glfwGetTime() - t) * 1000000.0);
        if(wait > wait_interval){wait = wait_interval;}
        //printf("%u: %u - %u\n", wait_interval, wait, (useconds_t)((glfwGetTime() - t) * 1000000.0));
        fc++;
    }

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
