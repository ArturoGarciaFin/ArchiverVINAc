#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lz.h"
#include "vinac.h"

void load_directory(FILE *archive, member **header, int *size)
{
    if (!archive)
    {
        fprintf(stderr, "Failed to open archiver in load_directory\n");
        *header = NULL;
        *size = 0;
        return;
    }

    //goes to end of archiver and reads size number
    fseek(archive, -sizeof(int), SEEK_END);
    fread(size, sizeof(int), 1, archive);
    
    if (*size == 0)
    {
        printf("Archiver empty\n");
        *header = NULL;
        return;
    }

    //points to start of directory
    fseek(archive, -(sizeof(int) + (*size)*sizeof(member)), SEEK_END);

    *header = malloc((*size)*sizeof(member));
    if (!*header)
    {
        fprintf(stderr, "Erro ao alocar memoria em load_directory\n");
        *size = 0;
        return;
    }

    //reads directory members
    fread(*header, sizeof(member), *size, archive);

    //printf("Directory loaded, %d members\n", *size);

    return;
}

void save_directory(FILE *archive, member **header, int size)
{
    if (!archive)
    {
        fprintf(stderr, "Failed to open archiver in save_directory\n");
        return;
    }

    if (size == 0 || *header == NULL)
    {
        fprintf(stderr, "Directory empty\n");
        return;
    }

    fseek(archive, 0, SEEK_END);

    //writes members and size in archiver
    fwrite(*header, sizeof(member), size, archive);
    fwrite(&size, sizeof(int), 1, archive);

    //printf("Directory saved, %d members\n", size);
}

void insert_normal(char *arc_name, char *path)
{
    FILE *archive = fopen(arc_name, "rb+");
    if (!archive)
    {
        archive = fopen(arc_name, "wb+");
        if (!archive)
        {
            fprintf(stderr, "Failed to create archiver in insert_normal");
            return;
        }
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, archive);
    }

    //loads directory
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    //opens entry file
    FILE *in = fopen(path, "rb");
    if (!in)
    {
        fprintf(stderr, "Failed to open %s\n file in insert_normal", path);
        fclose(archive);
        return;
    }

    //gets file metadata
    struct stat st;
    stat(path, &st);

    //fills struct member
    member entry;
    strncpy(entry.name, path, MAX_NAME);
    entry.uid = (unsigned int)getuid();
    entry.original_size = (unsigned int)st.st_size;
    entry.disk_size = entry.original_size;
    entry.mod_time = st.st_mtime;
    entry.is_compressed = 0;
    fseek(archive, 0, SEEK_END);
    entry.offset = ftell(archive);

    //creates buffer with standard Linux/Unix-like size
    char buffer[4096];
    size_t bytes;
    size_t bytes_written = 0;
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0)
    {
        fwrite(buffer, 1, bytes, archive);
        bytes_written += bytes;
    }

    fclose(in);
    printf("File inserted uncompressed\n");
    entry.disk_size = bytes_written;

    //updates directory
    header = realloc(header, (size + 1)*sizeof(member));
    if (!header)
    {
        fprintf(stderr, "Failed to reallocate memory in insert_normal\n");
        fclose(archive);
        return;
    }
    header[size] = entry;
    size++;

    //updates directory in archiver
    save_directory(archive, &header, size);

    free(header);
    fclose(archive);
}

void insert_compressed(char *arc_name, char *path)
{
    FILE *archive = fopen(arc_name, "rb+");
    if (!archive)
    {
        archive = fopen(arc_name, "wb+");
        if (!archive)
        {
            fprintf(stderr, "Failed to create archiver in insert_compressed");
            return;
        }
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, archive);
    }

    //loads directory
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    //opens entry file
    FILE *in = fopen(path, "rb");
    if (!in)
    {
        fprintf(stderr, "Failed to open %s\n file insert_normal", path);
        fclose(archive);
        return;
    }

    //gets file original size
    fseek(in, 0, SEEK_END);
    long original_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    //reads what is in the file
    unsigned char *data = malloc(original_size);
    if (!data)
    {
        fprintf(stderr, "Failed to allocate memory in insert_compressed\n");
        fclose(in);
        fclose(archive);
        return;
    }
    fread(data, 1, original_size, in);
    fclose(in);

    //creates space for compressed data
    unsigned char *c_data = malloc(original_size);
    if (!c_data)
    {
        fprintf(stderr, "Failed to allocate memory for compressed data in insert_compressed\n");
        free(data);
        fclose(archive);
        return;
    }

    //compresses data. if compressed size is bigger than original, inserts uncompressed
    int compressed_size = LZ_Compress(data, c_data, original_size);
    if (compressed_size >= original_size)
    {
        free(data);
        free(c_data);
        fclose(archive);
        insert_normal(arc_name, path);
        return;
    }

    free(data);

    //gets original file metadata
    struct stat st;
    stat(path, &st);

    //fills struct member
    member entry;
    strncpy(entry.name, path, MAX_NAME);
    entry.uid = (unsigned int)getuid();
    entry.original_size = (unsigned int)original_size;
    entry.disk_size = (unsigned int)compressed_size;
    entry.mod_time = st.st_mtime;
    entry.is_compressed = 1;
    fseek(archive, 0, SEEK_END);
    entry.offset = ftell(archive);

    //writes data in archiver
    fwrite(c_data, 1, compressed_size, archive);
    free(c_data);

    //updates directory
    header = realloc(header, (size + 1)*sizeof(member));
    if (!header)
    {
        fprintf(stderr, "Error reallocating memory in insert_compressed\n");
        fclose(archive);
        return;
    }
    header[size] = entry;
    size++;

    save_directory(archive, &header, size);
    free(header);
    fclose(archive);

    printf("File inserted compressed.\n");
}

void extract_member(char *arc_name, char *name)
{
    FILE *archive = fopen(arc_name, "rb");
    if (!archive)
    {
        fprintf(stderr, "Failed to open archiver in extract_member\n");
        return;
    }

    //carrega diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Archiver empty\n");
        fclose(archive);
        return;
    }

    int index = find_member(header, size, name);
    if (index == -1)
    {
        printf("Member not in archiver\n");
        free(header);
        fclose(archive);
        return;
    }

    member extracted = header[index];

    unsigned char *c_data = malloc(extracted.disk_size);
    if (!c_data)
    {
        fprintf(stderr, "Error allocating disk_size memory in extract_member\n");
        free(header);
        fclose(archive);
        return;
    }

    //reads extracted member
    fseek(archive, extracted.offset, SEEK_SET);
    fread(c_data, 1, extracted.disk_size, archive);

    //creates output file with extracted file name
    FILE *out = fopen(extracted.name, "wb");
    if (!out)
    {
        fprintf(stderr, "Error creating output file\n");
        free(c_data);
        free(header);
        fclose(archive);
        fclose(out);
        return;
    }

    //uncompresses file if compressed
    if (extracted.is_compressed)
    {
        unsigned char *dc_data = malloc(extracted.original_size);
        if (!dc_data)
        {
            fprintf(stderr, "Error allocating original_size memory extract_member\n");
            free(c_data);
            free(header);
            fclose(archive);
            fclose(out);
            return;
        }

        LZ_Uncompress(c_data, dc_data, extracted.disk_size);

        //writes data in output file
        fwrite(dc_data, 1, extracted.original_size, out);
        free(dc_data);
    }
    //writes data in outputfile without uncompressing
    else
        fwrite(c_data, 1, extracted.disk_size, out);

    printf("File %s extracted successfully\n", extracted.name);

    fclose(out);
    free(c_data);
    free(header);
    fclose(archive);
}

void extract_all(char *arc_name)
{
    FILE *archive = fopen(arc_name, "rb");
    if (!archive)
    {
        fprintf(stderr, "Failed to open archiver in extract_member\n");
        return;
    }

    //loads directory
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Archiver empty\n");
        fclose(archive);
        return;
    }

    for (int i = 0; i < size; i++)
        extract_member(arc_name, header[i].name);

    printf("All files extracted successfully\n");
}

void list_archive(char *arc_name)
{
    FILE *archive = fopen(arc_name, "rb");
    if (!archive)
    {
        fprintf(stderr, "Failed to open archiver in list_archive\n");
        return;
    }

    //loads directory
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);
    
    if (size == 0 || header == NULL)
    {
        printf("Archiver empty\n");
        return;
    }

    for (int i = 0; i < size; i++)
    {
        printf("Name: %s\n", header[i].name);
        printf("UID: %u\n", header[i].uid);
        printf("Original size: %u bytes\n", header[i].original_size);
        printf("Disk size: %u bytes\n", header[i].disk_size);
        printf("Offset size: %ld\n", header[i].offset);
        printf("Compressed: ");
        if (header[i].is_compressed)
            printf("yes\n");
        else
            printf("no\n");
        //converts stored time to year-month-day hours:minutes:seconds
        char timebuf[64];
        struct tm *time = localtime(&header[i].mod_time);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", time);
        printf("Last modification: %s\n", timebuf);
        printf("==================\n");
    }

    free(header);
    fclose(archive);
}

char find_member(member *header, int size, char *name)
{
    for (int i = 0; i < size; i++)
    {
        if (!strcmp(header[i].name, name))
            return i;
    }

    return -1;
}

void remove_member(char *arc_name, char *name)
{
    FILE *archive = fopen(arc_name, "rb+");
    if (!archive)
    {
        
        fprintf(stderr, "Failed to open archiver in remove_member\n");
        return;
    }

    //loads directory
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Archiver empty\n");
        fclose(archive);
        return;
    }

    //if directory size is 1, deletes archiver
    if (size == 1)
    {
        free(header);
        fclose(archive);
        remove(arc_name);
        printf("Last member removed successfully\n");
        return;
    }

    //searches for member to be removed
    int index = find_member(header, size, name);
    if (index == -1)
    {
        printf("Member not in directory\n");
        free(header);
        fclose(archive);
        return;
    }

    for (int i = index; i < size - 1; i++)
        header[i] = header[i + 1];
    size--;    

    member *new_header = realloc(header, size*sizeof(member));
    if (new_header)
        header = new_header;
    
    save_directory(archive, &header, size);

    free(header);
    fclose(archive);
    printf("Membro %s removed successfully\n", name);
}

void move_member(char *arc_name, char *name, char *target)
{
    FILE *archive = fopen(arc_name, "rb+");
    if (!archive)
    {
        fprintf(stderr, "Failed to open archiver in move_member\n");
        return;
    }

    //loads directory
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Archiver empty\n");
        fclose(archive);
        return;
    }

    //searches for member indexes
    int idx_name = find_member(header, size, name);
    int idx_target = find_member(header, size, target);

    if (idx_name == -1)
    {
        printf("Member '%s' not in directory\n", name);
        free(header);
        fclose(archive);
        return;
    }

    if (idx_target == -1)
    {
        printf("Target member '%s' não encontrado\n", target);
        free(header);
        fclose(archive);
        return;
    }

    if (idx_name == idx_target)
    {
        printf("Member and target member are the same\n");
        free(header);
        fclose(archive);
        return;
    }

    //extracts member that is being moved
    member moving = header[idx_name];

    if (idx_name < idx_target)
    {
        //moves all elements between idx_name + 1 and idx_target + 1 back 1 position
        for (int i = idx_name; i < idx_target; i++)
            header[i] = header[i + 1];

        //inserts member in new position
        header[idx_target] = moving;
    }
    else
    {
        //moves all elements between idx_target + 1 and idx_name - 1 forward 1 position
        for (int i = idx_name; i > idx_target + 1; i--)
            header[i] = header[i - 1];

        //inserts member right after target
        header[idx_target + 1] = moving;
    }

    printf("Member '%s' is now after member '%s'\n", name, target);

    //updates directory and archiver
    fseek(archive, 0, SEEK_END);
    save_directory(archive, &header, size);

    free(header);
    fclose(archive);
}
