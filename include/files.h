#ifndef FILES_H
#define FILES_H

void list_files(char *path, int size_filter, long int bytes, int name_filter, char *string);
void list_files_recursive_tree(char *path, char *prefix);

#endif
