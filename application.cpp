#include "application.h"

#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

using namespace std;
enum { NONE, AMBIENT, DIFFUSE, SPECULAR, NUM_MODES };

//color code red: #FF 00 00
const vec3 RED(1.0f, 0.0f, 0.0f);
const vec3 BLACK(0.0f, 0.0f, 0.0f);
const vec3 WHITE(1.0f, 1.0f, 1.0f);
//yellow #FFFF00
const vec3 YELLOW(1.0f, 1.0f, 0.0f);
//grey #808080
const vec3 GRAY(0.5f, 0.5f, 0.5f);
//cyan #00FFFF
const vec3 CYAN(0.0f, 1.0f, 1.0f);

//returns a range between low and high
float draw_line(float duration, float x_old, float x, float y_old, float y) {
    return (duration - x_old) / (x - x_old) * (y - y_old) + y_old;
}

//returns a random float between low and high
float random(float low, float high) {
    return draw_line(rand(), 0.0f, static_cast<float> (RAND_MAX), low, high);
}

struct Particle {

    vec3 position;
    vec3 velocity;
    float mass;
    vec3 color;
    vec3 force;
    float duration;

    void reset() {
        // initial position
        position[0] = random(-0.2f, 0.2f);
        position[1] = 0.05f;
        position[2] = random(-0.2f, 0.2f);

        // initial velocity
        velocity[0] = 10 * position[0];
        velocity[1] = random(1.0f, 10.0f);
        velocity[2] = 10 * position[2];

        // initial color
        color = YELLOW;

        // constant mass?
        mass = 1.0f;
        force.make_zero();
        duration = 0.0f;
    }

    // update v and x with a Foward Euler Step
    void Euler_Step(float h) {
        position += h * velocity;
        velocity += force * (h / mass);
    }

    // reset force to 0 vector
    void Reset_Forces() {
        force.make_zero();
    }

    // reflect particle on ground,
    // apply damping and restitution
    void Handle_Collision(float damping, float coeff_restitution) {
        // reflect particle on ground if y_position < 0
        if (position[1] < 0) {
            position[1] = 0;
            //reflect and apply damping
            if (velocity[1] < 0) {
                velocity[0] *= damping;
                velocity[1] *= -coeff_restitution;
                velocity[2] *= damping;

            }
        }
    }

};

vector<Particle> particles;

// Generates n particles and appends
void Add_Particles(size_t n) {

    for (size_t i = 0; i < n; i++) {
        Particle p;
        p.reset();
        particles.push_back(p);
    }
}

vec3 Get_Particle_Color(float duration) {
    vec3 interpolated_color;

    if (0.0f <= duration && duration <= 0.05f) {
        return CYAN;
    }
    else if (0.05f <= duration && duration <= 0.1f) {
        interpolated_color = vec3(
            draw_line(duration, 0.05f, 0.1f, CYAN[0], WHITE[0]),
            draw_line(duration, 0.05f, 0.1f, CYAN[1], WHITE[1]),
            draw_line(duration, 0.05f, 0.1f, CYAN[2], WHITE[2]));
        return interpolated_color;
    }
    else if (0.1f <= duration && duration <= 0.7f) {
        interpolated_color = vec3(
            draw_line(duration, 0.1f, 0.7f, WHITE[0], YELLOW[0]),
            draw_line(duration, 0.1f, 0.7f, WHITE[1], YELLOW[1]),
            draw_line(duration, 0.1f, 0.7f, WHITE[2], YELLOW[2]));
        return interpolated_color;
    }
    else if (0.7f <= duration && duration <= 1.5f) {
        interpolated_color = vec3(
            draw_line(duration, 0.7f, 1.5f, YELLOW[0], RED[0]),
            draw_line(duration, 0.7f, 1.5f, YELLOW[1], RED[1]),
            draw_line(duration, 0.7f, 1.5f, YELLOW[2], RED[2]));
        return interpolated_color;
    }
    else if (1.5f <= duration && duration <= 2.0f) {
        return RED;
    }
    else if (2.0f <= duration && duration <= 3.0f) {
        interpolated_color = vec3(
            draw_line(duration, 2.0f, 3.0f, RED[0], GRAY[0]),
            draw_line(duration, 2.0f, 3.0f, RED[1], GRAY[1]),
            draw_line(duration, 2.0f, 3.0f, RED[2], GRAY[2]));
        return interpolated_color;
    }
    else {
        assert(3.0f <= duration);
        return GRAY;
    }
}








void draw_grid(int dim);
void draw_obj(obj *o, const gl_image_texture_map& textures);

void set_pixel(int x, int y, float col[3])
{
    // write a 1x1 block of pixels of color col to framebuffer
    // coordinates (x, y)
    //glRasterPos2i(x, y);
    //glDrawPixels(1, 1, GL_RGB, GL_FLOAT, col);

    // use glVertex instead of glDrawPixels (faster)
    glBegin(GL_POINTS);
    glColor3fv(col);
    glVertex2f(x, y);
    glEnd();
}

application::application()
    : raytrace(false), rendmode(SPECULAR), paused(false), sim_t(0.0),draw_volcano(true),h(0.015)
{
}

application::~application()
{
}

// triggered once after the OpenGL context is initialized
void application::init_event()
{

    cout << "CAMERA CONTROLS: \n  LMB: Rotate \n  MMB: Move \n  RMB: Zoom" << endl;
    cout << "KEYBOARD CONTROLS: \n";
    cout << "  'p': Pause simulation\n";
    cout << "  'v': Toggle draw volcano" << endl;

    const GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    const GLfloat diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    const GLfloat specular[] = { 1.0, 1.0, 1.0, 1.0 };

    // enable a light
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, specular);
    glEnable(GL_LIGHT1);

    // set global ambient lighting
    GLfloat amb[] = { 0.4, 0.4, 0.4, 1.0 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

    // enable depth-testing, colored materials, and lighting
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    // normalize normals so lighting calculations are correct
    // when using GLUT primitives
    glEnable(GL_RESCALE_NORMAL);

    // enable smooth shading
    glShadeModel(GL_SMOOTH);

    glClearColor(0,0,0,0);

    set_camera_for_box(vec3(-3,-2,-3),vec3(3,5,3));

    t.reset();
    o.load("crater.obj");

    // loads up all the textures referenced by the .mtl file
    const std::map<std::string, obj::material>& mats = o.get_materials();
    for (std::map<std::string, obj::material>::const_iterator i = mats.begin();
        i != mats.end(); ++i
        )
    {
        if (!i->second.map_kd.empty()) {
            string filename = i->second.map_kd;

            // add texture if we have not already loaded it
            if (texs.find(filename) == texs.end()) {
                gl_image_texture *tex = new gl_image_texture();
                if (tex->load_texture(filename) != SUCCESS) {
                    cout << "could not load texture file: " << filename << endl;
                    exit(0);
                }
                texs[filename] = tex;
            }
        }
    }
}

// triggered each time the application needs to redraw
void application::draw_event()
{
    apply_gl_transform();

    const GLfloat light_pos1[] = { 0.0, 10.0, 0.0, 1 };
    glLightfv(GL_LIGHT1, GL_POSITION, light_pos1);

    if (!paused) {
        //
        //ADD NEW PARTICLES
        //
        if (particles.size() <= 7500) {
            Add_Particles(20);
        }

        //
        // SIMULATE YOUR PARTICLE HERE.
        //
        //
        for (size_t i = 0; i < particles.size(); i++) {

            //update duration of particle
            particles[i].duration += h;

            //reset partcle if duration is too long
            if (particles[i].duration >= 4.0f){
                particles[i].reset();
            }

            //update position and velocity
            particles[i].Euler_Step(h);

            //set force
            particles[i].force[0] = 0.0f;
            particles[i].force[1] = -9.8f * particles[i].mass;// y force is gravity
            particles[i].force[2] = 0.0f;

            //collision with ground
            particles[i].Handle_Collision(0.5, 0.5);

        //
        // UPDATE THE COLOR OF THE PARTICLE DYNAMICALLY
        //
            particles[i].color = Get_Particle_Color(particles[i].duration);
        }
    }

    glLineWidth(2.0);
    glEnable(GL_COLOR_MATERIAL);
    glBegin(GL_LINES);
        //
        //
        // DRAW YOUR PARTICLE USING GL_LINES HERE
        //
        // glVertex3f(...) endpoint 1
        // glVertex3f(...) endpoint 2
        //
        //
    for (size_t i = 0; i < particles.size(); i++) {

        // Particle& this_particle = particles[i];
        float delta_t = 0.04f;

        glColor3f(
            particles[i].color[0],
            particles[i].color[1],
            particles[i].color[2]);

        glVertex3f(
            particles[i].position[0],    // x0
            particles[i].position[1],    // y0
            particles[i].position[2]);   // z0

        glVertex3f(
            particles[i].position[0] + (delta_t * particles[i].velocity[0]),
            particles[i].position[1] + (delta_t * particles[i].velocity[1]),
            particles[i].position[2] + (delta_t * particles[i].velocity[2]));

    }

    glEnd();

    // draw the volcano
    if(draw_volcano){
        glEnable(GL_LIGHTING);
        glPushMatrix();
        glScalef(0.2,0.3,0.2);
        draw_obj(&o, texs);
        glPopMatrix();
        glDisable(GL_LIGHTING);
    }


    glColor3f(0.15, 0.15, 0.15);
    draw_grid(40);

    //
    // This makes sure that the frame rate is locked to close to 60 fps.
    // For each call to draw_event you will want to run your integrate for 0.015s
    // worth of time.
    //
    float elap = t.elapsed();
    if (elap < h) {
        usleep(1e6*(h-elap));
		//sleep(h-elap);
    }
    t.reset();
}

// triggered when mouse is clicked
void application::mouse_click_event(int button, int button_state, int x, int y)
{
}

// triggered when mouse button is held down and the mouse is
// moved
void application::mouse_move_event(int x, int y)
{
}

// triggered when a key is pressed on the keyboard
void application::keyboard_event(unsigned char key, int x, int y)
{

    if (key == 'r') {
        sim_t = 0;
    } else if (key == 'p') {
        paused = !paused;
    } else if (key == 'v') {
        draw_volcano=!draw_volcano;
    } else if (key == 'q') {
        exit(0);
    }
}

void draw_grid(int dim)
{
    glLineWidth(2.0);


    //
    // Draws a grid along the x-z plane
    //
    glLineWidth(1.0);
    glBegin(GL_LINES);

    int ncells = dim;
    int ncells2 = ncells/2;

    for (int i= 0; i <= ncells; i++)
    {
        int k = -ncells2;
        k +=i;
        glVertex3f(ncells2,0,k);
        glVertex3f(-ncells2,0,k);
        glVertex3f(k,0,ncells2);
        glVertex3f(k,0,-ncells2);
    }
    glEnd();

    //
    // Draws the coordinate frame at origin
    //
    glPushMatrix();
    glScalef(1.0, 1.0, 1.0);
    glBegin(GL_LINES);

    // x-axis
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(1.0, 0.0, 0.0);

    // y-axis
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 1.0, 0.0);

    // z-axis
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 1.0);
    glEnd();
    glPopMatrix();
}

void draw_obj(obj *o, const gl_image_texture_map& textures)
{
    glDisable(GL_COLOR_MATERIAL);

    // draw each polygon of the mesh
    size_t nfaces = o->get_face_count();
    for (size_t i = 0; i < nfaces; ++i)
    {
        const obj::face& f = o->get_face(i);

        // sets the material properties of the face
        const obj::material& mat = o->get_material(f.mat);
        if (!mat.map_kd.empty()) {
            gl_image_texture_map::const_iterator it = textures.find(mat.map_kd);
            if (it != textures.end()) {
                gl_image_texture* tex = it->second;
                tex->bind();
            }
            GLfloat mat_amb[] = { 1, 1, 1, 1 };
            GLfloat mat_dif[] = { mat.kd[0], mat.kd[1], mat.kd[2], 1 };
            GLfloat mat_spec[] = { mat.ks[0], mat.ks[1], mat.ks[2], 1 };
            glMaterialfv(GL_FRONT, GL_AMBIENT, mat_amb);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_dif);
            glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
        } else {
            GLfloat mat_amb[] = { mat.ka[0], mat.ka[1], mat.ka[2], 1 };
            GLfloat mat_dif[] = { mat.kd[0], mat.kd[1], mat.kd[2], 1 };
            GLfloat mat_spec[] = { mat.ks[0], mat.ks[1], mat.ks[2], 1 };
            glMaterialfv(GL_FRONT, GL_AMBIENT, mat_amb);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_dif);
            glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
        }
        glMaterialf(GL_FRONT, GL_SHININESS, mat.ns);

        if (!glIsEnabled(GL_TEXTURE_2D)) glEnable(GL_TEXTURE_2D);

        // draws a single polygon
        glBegin(GL_POLYGON);
        for (size_t j = 0; j < f.vind.size(); ++j)
        {
            // vertex normal
            if (f.nind.size() == f.vind.size()) {
                const float *norm = o->get_normal(f.nind[j]);
                glNormal3fv(norm);
            }

            // vertex UV coordinate
            if (f.tex.size() > 0) {
                const float* tex = o->get_texture_indices(f.tex[j]);
                glTexCoord2fv(tex);
            }

            // vertex coordinates
            const float *vert = o->get_vertex(f.vind[j]);
            glVertex3fv(vert);
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
}
