class shader {
  GLuint program_;
  GLuint emissive_colorIndex_;
public:
  shader() {}
  
  void init(const char *vs, const char *fs) {
    bool debug = true;
    GLsizei length;
    GLchar buf[256];

    // create our vertex shader and compile it
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vs, NULL);
    glCompileShader(vertex_shader);
    glGetShaderInfoLog(vertex_shader, sizeof(buf), &length, buf);
    puts(buf);
    
    // create our fragment shader and compile it
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    glGetShaderInfoLog(fragment_shader, sizeof(buf), &length, buf);
    puts(buf);

    // assemble the program for use by glUseProgram
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    
    // pos and normal are always 0 and 1
    glBindAttribLocation(program, 0, "pos");
    glBindAttribLocation(program, 1, "normal");
    glLinkProgram(program);
    program_ = program;
    glGetProgramInfoLog(program, sizeof(buf), &length, buf);
    puts(buf);
    
    emissive_colorIndex_ = glGetUniformLocation(program, "emissive_color");
    if( debug ) printf("emissive_colorIndex_=%d\n", emissive_colorIndex_);
  }
  
  // set the uniforms
  void render(const vec4 &emissive_color) {
    glUseProgram(program_);

    glUniform4fv(emissive_colorIndex_, 1, emissive_color.get());
  }
};
