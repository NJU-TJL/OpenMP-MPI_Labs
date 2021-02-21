extern long readAll(char *buffer, const char *filepath);
extern int make_dict_Hash(char *dict_buffer);
extern char *copyFrom(const char *src);
extern void make_profile(const char *filename, int dict_size, int *profile);
extern int get_names(const char *dir, char ***filenames);
extern void write_profiles(const char* filepath,int file_count,int dict_size, char** filenames, int** vectors);