#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vinac.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
        return 1;
    }

    char *mode = argv[1];
    char *archive = argv[2];

    if (!strcmp(mode, "-ip"))
    {
        if (argc < 4)
        {
            fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
            return 1;
        }
        
        for (int i = 3; i < argc; i++)
            insert_normal(archive, argv[i]);

        return 0;
    }

    if (!strcmp(mode, "-ic"))
    {
        if (argc < 4)
        {
            fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
            return 1;
        }
        
        for (int i = 3; i < argc; i++)
            insert_compressed(archive, argv[i]);

        return 0;
    }

    if (!strcmp(mode, "-m"))
    {
        if (argc != 5)
        {
            fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
            return 1;
        }

        move_member(archive, argv[3], argv[4]);
        return 0;
    }

    if (!strcmp(mode, "-x"))
    {
        if (argc < 3)
        {
            fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
        }
        
        if (argc == 3)
        {
            extract_all(archive);
            return 0;
        }
        else
        {
            for (int i = 3; i < argc; i++)
                extract_member(archive, argv[i]);
        }

        return 0;
    }

    if (!strcmp(mode, "-r"))
    {
        if (argc < 4)
        {
            fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
            return 1;
        }
        
        for (int i = 3; i < argc; i++)
            remove_member(archive, argv[i]);

        return 0;
    }

    if (!strcmp(mode, "-c"))
    {
        if (argc != 3)
        {
            fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
        }
        list_archive(archive);
        return 0;
    }

    fprintf(stderr, "Uso incorreto dos parametros de entrada\n");
    return 1;
}