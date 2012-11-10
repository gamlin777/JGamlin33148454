class texture_manager {
  enum { max_textures = 64 };
  GLuint textures[max_textures];
  unsigned cur_texture;

  typedef unsigned char uint8_t;
  // this is the TGA header
  // http://en.wikipedia.org/wiki/Truevision_TGA
  //
  struct TgaHeader
  {
    uint8_t identsize;          // size of ID field that follows 18 uint8_t header (0 usually)
    uint8_t colourmaptype;      // type of colour map 0=none, 1=has palette
    uint8_t imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    uint8_t colourmapstart[2];     // first colour map entry in palette
    uint8_t colourmaplength[2];    // number of colours in palette
    uint8_t colourmapbits;      // number of bits per palette entry 15,16,24,32

    uint8_t xstart[2];             // image x origin
    uint8_t ystart[2];             // image y origin
    uint8_t width[2];              // image width in pixels
    uint8_t height[2];             // image height in pixels
    uint8_t bits;               // image bits per pixel 8,16,24,32
    uint8_t descriptor;         // image descriptor bits (vh flip bits)
  };

  // read a pair of bytes as a little-endian value
  int le2( uint8_t val[2] )
  {
    return val[0] + val[1] * 0x100;
  }

  // opengl will store the texture with this handle
  GLuint gl_texture_;
  
  texture_manager() {
    glGenTextures(max_textures, textures);
    cur_texture = 0;
  }
  
  void init(GLuint texture, const char *tga_file, int xoff, int yoff, int w, int h) {
    // convert the data
    TgaHeader *header = (TgaHeader*)tga_file;
    const uint8_t *data = (uint8_t *)tga_file + sizeof(TgaHeader);
    
    int width = le2(header->width);
    int height = le2(header->height);

    // make sure this is the GIMP flavour  
    assert(header->identsize == 0);
    assert(header->colourmaptype == 0);
    assert(header->imagetype == 2);
    //assert(header->descriptor == 0);
    assert(header->bits == 32 || header->bits == 24);
    
    int bytes = header->bits / 8;
    uint8_t *tmp = (uint8_t *)malloc(w*h*bytes);
    uint8_t *dest = tmp;

    // swap red and blue!
    for (int y = 0; y != h; ++y)
    {
      const uint8_t *src = data + (y+yoff) * width * bytes + xoff * bytes;
      if (bytes == 4) {
        for (int x = 0; x != w; ++x)
        {
          uint8_t alpha = src[ x*4 + 3 ];
          uint8_t red = src[ x*4 + 2 ];
          uint8_t green = src[ x*4 + 1 ];
          uint8_t blue = src[ x*4 + 0 ];
          dest[0] = red;
          dest[1] = green;
          dest[2] = blue;
          dest[3] = alpha;
          dest += 4;
        }
      } else {
        for (int x = 0; x != w; ++x)
        {
          uint8_t red = src[ x*3 + 2 ];
          uint8_t green = src[ x*3 + 1 ];
          uint8_t blue = src[ x*3 + 0 ];
          dest[0] = red;
          dest[1] = green;
          dest[2] = blue;
          dest += 3;
        }
      }
    }

    GLenum fmt = bytes == 4 ? GL_RGBA : GL_RGB;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, (void*)tmp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glGenerateMipmap(GL_TEXTURE_2D);
    free(tmp);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  
  GLuint get_texture(const char *filename, int x, int y, int w, int h) {
    char *data = (char*)file_manager::bytes(filename);
    if (!data || cur_texture == max_textures) {
      return 0;
    }
    GLuint result = textures[cur_texture++];
    init(result, data, x, y, w, h);
    return result;
  }

  static texture_manager &get() {
    static texture_manager the_texture_manager;
    return the_texture_manager;
  }
public:
  static GLuint new_texture(const char *filename, int x, int y, int w, int h) {
    return get().get_texture(filename, x, y, w, h);
  }
};

