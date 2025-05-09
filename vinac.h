#ifndef _VINAC_
#define _VINAC_

#include <stdio.h>
#include <time.h>

#define MAX_NAME 1025

struct Member
{
    char name[MAX_NAME];
    unsigned int uid;
    unsigned int original_size;
    unsigned int disk_size;
    time_t mod_time;
    unsigned int order;
    long offset;
    char is_compressed;
};

typedef struct Member member;

void load_directory(FILE *archive, member **header, int *size);
void save_directory(FILE *archive, member **header, int size);
void insert_normal(char *arc_name, char *path);
void insert_compressed(char *arc_name, char *path);
char find_member(member *header, int size, char *name);
void extract_member(char *arc_name, char *name);
void extract_all(char *arc_name);
void list_archive(char *arc_name);
void move_member(char *arc_name, char *name, char *target);
void remove_member(char *arc_name, char *name);

#endif