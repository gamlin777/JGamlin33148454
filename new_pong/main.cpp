////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2011
//
// Example of drawing a triangle using OpenGL ES 2

#ifdef WIN32
  // This is the "GL extension wrangler"
  // Note: you will need to use the right version of glew32s.lib (32 bit vs 64 bit)
  #define GLEW_STATIC
  #include "GL/glew.h"

  // This is the "GL utilities" framework for drawing with OpenGL
  #define FREEGLUT_STATIC
  #define FREEGLUT_LIB_PRAGMAS 0
  #include "GL/glut.h"
#else // (osx?)
  #include "GLUT/glut.h"
#endif

// standard C headers
#include <stdio.h>
#include <math.h>

// math support
#include "include/vector.h"
#include "include/matrix.h"

// shader wrapper
#include "include/shader.h"

// box class - holds shape and color of a box on the screen.
class box {
  vec4 center_;
  vec4 half_extents_;
  vec4 color_;
public:
  // we don't have a constructor, but we do have an 'init' function
  void init(float cx, float cy, float hx, float hy) {
    // note that it is a convention for positions and colors
    // to have "1" in the w, distances to have "0" in the w.
    center_ = vec4(cx, cy, 0, 1);
    half_extents_ = vec4(hx, hy, 0, 0);
    color_ = vec4(1, 1, 1, 1);
  }

  // draw the box using a triangle fan.
  void draw(shader &shader) {
    // set the uniforms
    shader.render(color_);

    // set the attributes    
    float vertices[4*2] = {
      center_[0] - half_extents_[0], center_[1] - half_extents_[1],
      center_[0] + half_extents_[0], center_[1] - half_extents_[1],
      center_[0] + half_extents_[0], center_[1] + half_extents_[1],
      center_[0] - half_extents_[0], center_[1] + half_extents_[1],
    };
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)vertices );
    glEnableVertexAttribArray(0);

    // kick the draw
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }
  
  // move the box
  void move(const vec4 &dir) {
    center_ += dir;
  }
  
  // the 'pos' property
  vec4 pos() const { return center_; }
  void set_pos(vec4 v) { center_ = v; }
  
  // return true if two boxes intersect
  bool intersects(const box &rhs) {
    vec4 diff = (rhs.pos() - pos()).abs();
    vec4 min_distance = rhs.half_extents_ + half_extents_;
    return diff[0] < min_distance[0] && diff[1] < min_distance[1];
  }
};

// the game
class NewPongGame
{
  // game state: always uses enums for int constants
  enum state_t {
    state_serving,
    state_playing,
    state_end,
  };
  
  state_t state;
  int server;

  box bats[2];
  box ball;
  vec4 ball_velocity;
  int scores[2];
  
  // rendering  
  shader colour_shader_;
  GLint viewport_width_;
  GLint viewport_height_;
  
  // input
  char keys[256];
  
  // constants: always use functions for floats!
  float ball_speed() { return 0.01f; }
  float court_size() { return 0.6f; }

  void draw_world(shader &shader) {
    ball.draw(shader);
    
    for (int player = 0; player != 2; ++player) {
      // draw the bat
      bats[player].draw(shader);
      
      box blob;
      float blob_size = 0.02f;
      float blob_spacing = 0.05f;
      float blob_offset = player == 0 ? -0.9f : 0.5f;
      
      // draw the scores as blobs
      for (int i = 0; i != scores[player]; ++i ) {
        blob.init(blob_offset + blob_spacing * i, 0.7f, blob_size, blob_size);
        blob.draw(shader);
      }
    }
  }
  
  void move_bats() {
    // look at the keys and move the bats
    vec4 bat_up(0, 0.02f, 0, 0);
    if (keys['w']) {
      bats[0].move(bat_up);
    }
    if (keys['s']) {
      bats[0].move(-bat_up);
    }
    if (keys['o']) {
      bats[1].move(bat_up);
    }
    if (keys['l']) {
      bats[1].move(-bat_up);
    }
  }
  
  // called when someone scores
  void adjust_score(int player) {
    scores[player]++;
    if (scores[player] > 10) {
      state = state_end;
    } else
    {
      server = 1 - player;
      state = state_serving;
    }
  }
  
  void do_serving() {
    // while serving, glue the ball to the server's bat
    vec4 offset = vec4(server ? -0.1f : 0.1f, 0, 0, 0);
    ball.set_pos(bats[server].pos() + offset);
    if (keys[' ']) {
      state = state_playing;
      ball_velocity = vec4(server ? -ball_speed() : ball_speed(), -ball_speed(), 0, 0);
    }
  }

  void do_playing() {
    // bounce the ball and detect collisions
    vec4 new_pos = ball.pos() + ball_velocity;
    ball.set_pos(new_pos);

    // bounce
    if (
      ball_velocity[1] > 0 && new_pos[1] > court_size() ||
      ball_velocity[1] < 0 && new_pos[1] <- court_size()
    ) {
      ball_velocity = ball_velocity * vec4(1, -1, 1, 1);
    }

    // note we don't just simply reverse the ball...
    //   this would lead to a feedback loop.
    if (ball_velocity[0] > 0) {
      // right to left
      if (new_pos[0] > 1) {
        adjust_score(0);
      }
      if (ball.intersects(bats[1]) ) {
        ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
      }
    } else {
      // left to right
      if (new_pos[0] < -1) {
        adjust_score(1);
      }
      if (ball.intersects(bats[0]) ) {
        ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
      }
    }
  }
  
  // simulation for the game
  void simulate() {
    move_bats();
    
    if (state == state_serving) {
      do_serving();
    } else if (state == state_playing) {
      do_playing();
    }
  }
  
  // simulate and draw the game world every frame
  void render() {
    simulate();

    // clear the frame buffer and the depth
    glClearColor(0, 0, 1, 1);
    glViewport(0, 0, viewport_width_, viewport_height_);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    draw_world(colour_shader_);

    // swap buffers so that the image is displayed.
    // gets a new buffer to draw a new scene.
    glutSwapBuffers();
  }

  // set up the world
  NewPongGame()
  {
    memset(keys, 0, sizeof(keys));
    
    // set up game state.
    scores[0] = 0;
    scores[1] = 0;
    state = state_serving;
    server = 0;

    // make the bats and ball
    float bat_hx = 0.02f;
    float bat_hy = 0.10f;
    float bat_cx = 1 - bat_hx * 2;
    bats[0].init(-bat_cx, 0, bat_hx, bat_hy);
    bats[1].init( bat_cx, 0, bat_hx, bat_hy);
    float ball_hx = 0.03f;
    float ball_hy = 0.03f;
    ball.init(0, 0, ball_hx, ball_hy);
    
    // set up a simple shader to render the emissve color
    colour_shader_.init(
      // just copy the position attribute to gl_Position
      "attribute vec4 pos;"
      "void main() { gl_Position = pos; }",

      // just copy the color attribute to gl_FragColor
      "uniform vec4 emissive_color;"
      "void main() { gl_FragColor = emissive_color; }"
    );
  }
  
  // The viewport defines the drawing area in the window
  void set_viewport(int w, int h) {
    viewport_width_ = w;
    viewport_height_ = h;
  }
  
  void set_key(int key, int value) {
    keys[key & 0xff] = value;
  }
public:
  // a singleton: one instance of this class only!
  static NewPongGame &get()
  {
    static NewPongGame singleton;
    return singleton;
  }

  // interface from GLUT
  static void reshape(int w, int h) { get().set_viewport(w, h); }
  static void display() { get().render(); }
  static void timer(int value) { glutTimerFunc(30, timer, 1); glutPostRedisplay(); }
  static void key_down( unsigned char key, int x, int y) { get().set_key(key, 1); }
  static void key_up( unsigned char key, int x, int y) { get().set_key(key, 0); }
};

// boilerplate to run the sample
int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
  glutInitWindowSize(500, 500);
  glutCreateWindow("new pong");
  #ifdef WIN32
    glewInit();
    if (!glewIsSupported("GL_VERSION_2_0") )
    {
      printf("OpenGL 2 is required!\n");
      return 1;
    }
  #endif
  glutDisplayFunc(NewPongGame::display);
  glutReshapeFunc(NewPongGame::reshape);
  glutKeyboardFunc(NewPongGame::key_down);
  glutKeyboardUpFunc(NewPongGame::key_up);
  glutTimerFunc(30, NewPongGame::timer, 1);
  glutMainLoop();
  return 0;
}

