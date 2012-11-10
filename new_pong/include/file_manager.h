// a simple file manager class
class file_manager {
  enum { max_files = 16 };

  struct file {
    const char *filename;
    const void *bytes;
    size_t size;
  };
  
  file files[max_files];
  unsigned cur_file;
  
  file_manager() {
    memset(files, 0, sizeof(files));
    cur_file = 0;
  }
  
  file &get_file(const char *name) {
    for (unsigned i = 0; i != max_files; ++i) {
      if (files[i].filename && !strcmp(files[i].filename, name)) {
        return files[i];
      }
    }
    file &file = files[cur_file++ % max_files];
    
    // if we have wrapped around to the first file...
    if (file.filename) {
      free((void*)file.filename);
      free((void*)file.bytes);
    }
    
    FILE *in = fopen(name, "rb");
    file.size = 0;
    file.bytes = 0;
    if (in != 0) {
      fseek(in, 0, SEEK_END);
      file.size = ftell(in);
      fseek(in, 0, SEEK_SET);
      file.bytes = malloc(file.size);
      fread((void*)file.bytes, 1, file.size, in);
      fclose(in);
    }
    
    file.filename = (char*)malloc(strlen(name) + 1);
    strcpy((char*)file.filename, name);
    return file;
  }

  static file_manager &get() {
    static file_manager the_file_manager;
    return the_file_manager;
  }
public:
  // get a temporary pointer to the bytes of a file
  static const void *bytes(const char *name) {
    return get().get_file(name).bytes;
  }
  
  // get a temporary size of a file
  static size_t size(const char *name) {
    return get().get_file(name).size;
  }
};

