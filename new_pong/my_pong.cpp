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
#include <assert.h>

// math support
#include "include/vector.h"
#include "include/matrix.h"


// shader wrapper & other graphics resources
#include "include/shader.h"
#include "include/shadertex.h"
#include <file_manager.h>
#include <texture_manager.h>
GLuint background;

// random number generation
#include <ctime>
#include <cstdlib>



// Arrow Key Setup
bool* key_special_states = new bool[246];
bool* key_states = new bool[256];
  
void keySpecial (int key, int x, int y) {  
	key_special_states[key] = true;
}
void keySpecialUp (int key, int x, int y) {   
	key_special_states[key] = false;
}

//Collision Test Setup
bool collision_1_test;
bool brick_dead[12] = {false};
int brick_num = 12;
vec4 brick_kill(0, 2.0f, 0, 0);
bool brick_onscreen[12] = {true};

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
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); //vertices of every shape
  }
  
  // move the box
  void move(const vec4 &dir) {
    center_ += dir;
  }

  // the 'pos' property
  vec4 pos() const { return center_; }
  void set_pos(vec4 v) { center_ = v; }

  // the 'sides' property
  float r_side() const {return center_[0] + half_extents_[0];}
  float l_side() const {return center_[0] - half_extents_[0];}
  float t_side() const {return center_[1] + half_extents_[1];}
  float b_side() const {return center_[1] - half_extents_[1];}

  
  // return true if two boxes intersect
  bool intersects(const box &rhs) {
    vec4 diff = (rhs.pos() - pos()).abs();
    vec4 min_distance = rhs.half_extents_ + half_extents_;
    return diff[0] < min_distance[0] && diff[1] < min_distance[1];
  }

};

			// THE GAME
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
  box obstacle;
  box brick[12];
  vec4 ball_velocity;
  vec4 bat_ai;
  vec4 move_obstacle;
  int scores[2];
  bool obstacle_switch;

  // rendering  
  shader colour_shader_;
  GLint viewport_width_;
  GLint viewport_height_;
  shadertex texture_shader_;
  
  // input
  char keys[256];
  
  // constants: always use functions for floats!
  
  float court_size() { return 0.98f; }              
  float ball_speed() { return 0.01f; }



  void draw_world(shader &shader)
  {
    ball.draw(shader);
 
    for (int player = 0; player != 2; ++player)
	{
      // draw the bat
      bats[player].draw(shader);
      obstacle.draw(shader);
	
	  for (int i = 0; i <= brick_num; ++i) {
		  brick[i].draw(shader);
		  }

      box blob;								
      float blob_size = 0.02f;
      float blob_spacing = player == 0 ? -0.05f: 0.05f;
      float blob_offset = player == 0 ? -0.1f : 0.1f;
      
      // draw the scores as blobs
      for (int i = 0; i != scores[player]; ++i )
	  {
        blob.init(blob_offset + blob_spacing / 2 * i, 0.98f, blob_size / 2, blob_size);
        blob.draw(shader);
      }
    } 
  }
  
  void move_bats() {
    // look at the keys and move the bats
	vec4 bat_up(0, 0.02f, 0, 0);

	if (key_special_states[GLUT_KEY_UP] && bats[0].t_side() <= 1) {	// test for key and paddle within viewport
		bats[0].move(bat_up);
		}
	if (key_special_states[GLUT_KEY_DOWN] && bats[0].b_side() >= -1) {
		bats[0].move(-bat_up);
		}

	// Move obstacle
  move_obstacle = vec4(0, 0.01f, 0, 0);

	if (obstacle.t_side() >= 1){
		obstacle_switch = true;
	}
	if (obstacle.b_side() <= -1){
		obstacle_switch = false;
	}
	if (obstacle_switch == true){
		obstacle.move(-move_obstacle);
	}
	if (obstacle_switch == false){
		obstacle.move(move_obstacle);
	}
  }

   // called when someone scores
  void adjust_score(int player) {
    scores[player]++;
    if (scores[player] > 5) {
      state = state_end;
	  printf("Thank you for playing!\7\7\7\n\n");
    } else
    {
      server = 1 - player;
      state = state_serving;
    }
  }
  
  void do_serving() {
	srand(static_cast<unsigned int>(time(0)));				//seed random number
	float random_n;
	int lowest = -2;
	int highest = 2;
	int range;
	range = (highest - lowest) + 1;
	for(int index = 0; index < 20; index++) {
		random_n = lowest + int(range * rand()/(RAND_MAX + 1.0));
		if (random_n == 0.0f){
			random_n += 1.0f;
		}
	}

	// while serving, glue the ball to the server's bat
    vec4 s_offset = vec4(server ? -0.1f : 0.1f, 0, 0, 0);
    ball.set_pos(bats[server].pos() + s_offset);
    if (keys[' '] && server == 0) {
		state = state_playing;
		ball_velocity = vec4(ball_speed(), -ball_speed() * random_n, 0, 0);
	} else if (server == 1) {
		state = state_playing;
		ball_velocity = vec4(-ball_speed(), -ball_speed() * random_n, 0, 0);
    }
  }

  void do_playing() 
  {
	// bounce the ball and detect collisions
    vec4 new_pos = ball.pos() + ball_velocity;
    ball.set_pos(new_pos);

   // Opponent AI (Keep)
	bat_ai = vec4(0, 0.02f, 0, 0);

	if (new_pos[1] >= bats[1].pos()[1] && bats[1].t_side() <= 1) {
		bats[1].move(bat_ai);
	}
	if (new_pos[1] <= bats[1].pos()[1] && bats[1].b_side() >= -1) {
		bats[1].move(-bat_ai);
	}
	
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
		//reset bricks
	for (int i = 0; i <= brick_num; ++i) { 
	 if (brick[i].pos()[1] > 1){
		brick[i].move(-brick_kill);
		brick_dead[i] = false;
	  }
    }
      }
      if (ball.intersects(bats[1])) {
		ball_velocity = ball_velocity * vec4(-1.1f, 1.1f, 1, 1);
      }
    } else {
      // left to right
      if (new_pos[0] < -1) {
        adjust_score(1);
		//reset bricks
	for (int i = 0; i <= brick_num; ++i) {
	 if (brick[i].pos()[1] > 1){
		brick[i].move(-brick_kill);
		brick_dead[i] = false;
	  }
    }
      }
      if (ball.intersects(bats[0])) {
		ball_velocity = ball_velocity * vec4(-1.1f, 1.1f, 1, 1);
      }
    }
	//obstacle collisions
	if (!ball.intersects(obstacle)) {
		collision_1_test = false;
	}

	// bounces on center obstacle
	if (ball.intersects(obstacle)) {
		if (obstacle.t_side() >= ball.b_side() && new_pos[1] > obstacle.t_side() && collision_1_test == false) {
			ball_velocity = ball_velocity * vec4(1, -1, 1, 1);
			collision_1_test = true;
		}
		else if (obstacle.b_side() <= ball.t_side() && new_pos[1] < obstacle.b_side() && collision_1_test == false) {
			ball_velocity = ball_velocity * vec4(1, -1, 1, 1);
			collision_1_test = true;
		}
			else if (obstacle.l_side() <= ball.r_side() && new_pos[0] < obstacle.l_side() && ball.r_side() < obstacle.t_side() - 0.02f && ball.r_side() > obstacle.b_side() + 0.02f && collision_1_test == false) {
			ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
			collision_1_test = true;
		}
		else if (obstacle.r_side() >= ball.l_side() && new_pos[0] > obstacle.r_side() && ball.l_side() < obstacle.t_side() - 0.02f && ball.l_side() > obstacle.b_side() + 0.02f && collision_1_test == false) {
			ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
			collision_1_test = true;
		}
	}

		//Obstacle Collision failsafe - checks if ball is within the 'frame' of the obstacle
	vec4 xball_fix(0.05f, 0, 0, 0);
	vec4 yball_fix(0, 0.05f, 0, 0);

		if (ball.intersects(obstacle) && collision_1_test == false) { 
			if (obstacle.t_side() - 0.03f <= ball.pos()[1] && ball.pos()[1] <= obstacle.t_side()) {
				collision_1_test = false;
				ball.move(yball_fix);
				ball_velocity = ball_velocity * vec4(1, -1, 1, 1);
			}
			else if (obstacle.b_side() + 0.03f >= ball.pos()[1] && ball.pos()[1] >= obstacle.b_side()) {
				collision_1_test = false;
				ball.move(-yball_fix);
				ball_velocity = ball_velocity * vec4(1, -1, 1, 1);
			}
			else if (obstacle.l_side() + 0.025f >= ball.pos()[0] && ball.pos()[0] >= obstacle.l_side() && ball.pos()[1] > obstacle.b_side() + 0.03f && ball.pos()[1] < obstacle.t_side() - 0.03f ){
				collision_1_test = false;
				ball.move(-xball_fix);
				ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
			}
			else if (obstacle.r_side() - 0.025f <= ball.pos()[0] && ball.pos()[0] <= obstacle.r_side() && ball.pos()[1] > obstacle.b_side() + 0.03f && ball.pos()[1] < obstacle.t_side() - 0.03f ){
				collision_1_test = false;
				ball.move(xball_fix);
				ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
			}
		}

		//Brick Collisions
	for (int i = 0; i <= brick_num; ++i) {
	if (ball.intersects(brick[i])) { 
		if (brick[i].t_side() >= ball.b_side() && ball.pos()[1] > brick[i].t_side()) {
			ball_velocity = ball_velocity * vec4(1, -1, 1, 1);
			brick_dead[i] = true;

		}
		else if (brick[i].b_side() <= ball.t_side() && ball.pos()[1] < brick[i].b_side()) {
			ball_velocity = ball_velocity * vec4(1, -1, 1, 1);
			brick_dead[i] = true;
		}
		else if (brick[i].l_side() <= ball.r_side() && ball.pos()[0] < brick[i].l_side()) {
			ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
			brick_dead[i] = true;
		}
		else if (brick[i].r_side() >= ball.l_side() && ball.pos()[0] > brick[i].r_side()) {
			ball_velocity = ball_velocity * vec4(-1, 1, 1, 1);
			brick_dead[i] = true;
		}
	}
	}
	for (int i = 0; i <= brick_num; ++i) {
	if (brick[i].pos()[1] < 1){
		brick_onscreen[i] = true;
	}
	}
	for (int i = 0; i <= brick_num; ++i) {
	if (brick[i].pos()[1] > 1){
		brick_onscreen[i] = false;
	}
	}

	for (int i = 0; i <= brick_num; ++i) {
	if (brick_dead[i] == true && brick_onscreen[i] == true){
		brick[i].move(brick_kill);
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
    glClearColor(0, 0, 0, 1);
    glViewport(0, 0, viewport_width_, viewport_height_);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, background);
	texture_shader_.render();

	float vertices[4*4] = {
      -10.0f,-10.0f,0,0,
       10.0f,-10.0f,1,0,
       10.0f,10.0f,1,1,
      -10.0f,10.0f,0,1
    };

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)vertices );
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(vertices + 2) );
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);

    // kick the draw
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


    draw_world(colour_shader_);

    // swap buffers so that the image is displayed.
    // gets a new buffer to draw a new scene.
    glutSwapBuffers();
  }

  // set up the world
  NewPongGame()
  {
    memset(keys, 0, sizeof(keys));
	memset(key_states,0,256);
	memset(key_special_states,0,246);
    
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
    float ball_hx = 0.02f;
    float ball_hy = 0.02f;
    ball.init(0, 0, ball_hx, ball_hy);

	//make the center obstacle
	float obstacle_hx = 0.05f;
	float obstacle_hy = 0.3f;
	obstacle.init(0.0f, 1.1f, obstacle_hx,obstacle_hy);

	//make bricks
	float brick_hx = 0.05f;
	float brick_hy = 0.15f;
	//Upper left
    brick[1].init(-0.15f, 0.8f, brick_hx, brick_hy);
	brick[0].init(-0.30f, 0.8f, brick_hx, brick_hy);
	brick[2].init(-0.15f, 0.45f, brick_hx, brick_hy);
	//Upper right
	brick[3].init(0.15f, 0.8f, brick_hx, brick_hy);
	brick[4].init(0.30f, 0.8f, brick_hx, brick_hy);
	brick[5].init(0.15f, 0.45f, brick_hx, brick_hy);
	//lower left
	brick[8].init(-0.15f, -0.8f, brick_hx, brick_hy);
	brick[7].init(-0.30f, -0.8f, brick_hx, brick_hy);
	brick[6].init(-0.15f, -0.45f, brick_hx, brick_hy);
	//lower right
	brick[10].init(0.15f, -0.8f, brick_hx, brick_hy);
	brick[11].init(0.30f, -0.8f, brick_hx, brick_hy);
	brick[9].init(0.15f, -0.45f, brick_hx, brick_hy);

    // set up a sim(ple shader to render the emissve color
    colour_shader_.init(
      // just copy the position attribute to gl_Position
      "attribute vec4 pos;"
      "void main() { gl_Position = pos; }",

      // just copy the color attribute to gl_FragColor
      "uniform vec4 emissive_color;"
      "void main() { gl_FragColor = emissive_color; }"

    );
	// set up a simple shader to render the emissve color
    texture_shader_.init(
      // just copy the position attribute to gl_Position
      "varying vec2 uv_;"
      "attribute vec3 pos;"
      "attribute vec2 uv;"
      "void main() { gl_Position = vec4(pos * 0.05, 1); uv_ = uv; }",

      // just copy the color attribute to gl_FragColor
      "varying vec2 uv_;"
      "uniform sampler2D texture;"
      "void main() { gl_FragColor = texture2D(texture, uv_); }"
      //"void main() { gl_FragColor = vec4(1, 0, 0, 1); }"
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


int main(int argc, char **argv) // boilerplate to run the sample
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
  glutInitWindowSize(500, 500);
  glutCreateWindow("James Gamlin's Pong");

  background = texture_manager::new_texture("texture.tga", 0, 0, 256, 256);

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
  glutSpecialFunc(keySpecial);
  glutSpecialUpFunc(keySpecialUp);
  glutMainLoop();

  return 0;
}