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
        fprintf(stderr, "Arquivo nao aberto\n");
        *header = NULL;
        *size = 0;
        return;
    }

    //vai fim do arquivo e le a quantidade de membros
    fseek(archive, -sizeof(int), SEEK_END);
    fread(size, sizeof(int), 1, archive);
    
    //retorna se o tamanho for 0
    if (*size == 0)
    {
        *header = NULL;
        return;
    }

    //coloca o ponteiro no diretorio
    fseek(archive, -(sizeof(int) + (*size)*sizeof(member)), SEEK_END);

    *header = malloc((*size)*sizeof(member));
    if (!*header)
    {
        fprintf(stderr, "Erro ao alocar memoria em load_directory\n");
        *size = 0;
        return;
    }

    //le os membros do diretorio
    fread(*header, sizeof(member), *size, archive);

    //printf("Diretorio carregado, %d membros\n", *size);

    return;
}

void save_directory(FILE *archive, member **header, int size)
{
    if (!archive)
    {
        fprintf(stderr, "Arquivo nao aberto\n");
        return;
    }

    if (size == 0 || *header == NULL)
    {
        fprintf(stderr, "Diretorio vazio\n");
        return;
    }

    fseek(archive, 0, SEEK_END);

    //grava os membros e a quantidade de membros no archive
    fwrite(*header, sizeof(member), size, archive);
    fwrite(&size, sizeof(int), 1, archive);

    //printf("Diretorio salvo, %d membros\n", size);
}

void insert_normal(char *arc_name, char *path)
{
    FILE *archive = fopen(arc_name, "rb+");
    if (!archive)
    {
        archive = fopen(arc_name, "wb+");
        if (!archive)
        {
            fprintf(stderr, "Erro ao criar arquivo em insert_normal");
            return;
        }
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, archive);
    }

    //carrega diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    //abre o arquivo de entrada
    FILE *in = fopen(path, "rb");
    if (!in)
    {
        fprintf(stderr, "Erro ao abrir arquivo %s\n em insert_normal", path);
        fclose(archive);
        return;
    }

    //obtem informacoes do arquivo original
    struct stat st;
    stat(path, &st);

    //preenche a struct membro
    member entry;
    strncpy(entry.name, path, MAX_NAME);
    entry.uid = (unsigned int)getuid();
    entry.original_size = (unsigned int)st.st_size;
    entry.disk_size = entry.original_size;
    entry.mod_time = st.st_mtime;
    entry.is_compressed = 0;
    fseek(archive, 0, SEEK_END);
    entry.offset = ftell(archive);
    //fprintf(stderr, "offset inicial: %ld\n", entry.offset);

    /*
    printf("Name: %s\n", entry.name);
    printf("UID: %d\nOriginal size: %d\nDisk size: %d\n", entry.uid, entry.original_size, entry.disk_size);
    printf("Is compressed: %d\nOffset:%ld\n", entry.is_compressed, entry.offset);
    */

    //cria um buffer de tamanho padrao da de sistemas Linux e UNIX-like
    char buffer[4096];
    size_t bytes;
    size_t bytes_written = 0;
    //printf("%ld\n", sizeof(buffer));
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0)
    {
        fwrite(buffer, 1, bytes, archive);
        //printf("%c %c %c\n", buffer[0], buffer[1], buffer[2]);
        bytes_written += bytes;
    }

    fclose(in);
    printf("Arquivo inserido sem compressao\n");
    entry.disk_size = bytes_written;

    //atualiza diretorio
    header = realloc(header, (size + 1)*sizeof(member));
    if (!header)
    {
        fprintf(stderr, "erro ao realocar memoria em insert_normal\n");
        fclose(archive);
        return;
    }
    header[size] = entry;
    size++;

    //atualiza diretorio no archive
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
            fprintf(stderr, "Erro ao criar arquivo em insert_normal");
            return;
        }
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, archive);
    }

    //carrega diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    //abre o arquivo de entrada
    FILE *in = fopen(path, "rb");
    if (!in)
    {
        fprintf(stderr, "Erro ao abrir arquivo %s\n em insert_normal", path);
        fclose(archive);
        return;
    }

    //obtem tamanho original do arquivo
    fseek(in, 0, SEEK_END);
    long original_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    //le o conteudo do arquivo
    unsigned char *data = malloc(original_size);
    if (!data)
    {
        fprintf(stderr, "Erro ao alocar memoria em insert_compressed\n");
        fclose(in);
        fclose(archive);
        return;
    }
    fread(data, 1, original_size, in);
    fclose(in);

    //aloca espaco para o conteudo comprimido
    unsigned char *c_data = malloc(original_size);
    if (!c_data)
    {
        fprintf(stderr, "Erro ao alocar memoria em insert_compressed\n");
        free(data);
        fclose(archive);
        return;
    }

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

    //obtem informacoes do arquivo original
    struct stat st;
    stat(path, &st);

    //preenche a struct membro
    member entry;
    strncpy(entry.name, path, MAX_NAME);
    entry.uid = (unsigned int)getuid();
    entry.original_size = (unsigned int)original_size;
    entry.disk_size = (unsigned int)compressed_size;
    entry.mod_time = st.st_mtime;
    entry.is_compressed = 1;
    fseek(archive, 0, SEEK_END);
    entry.offset = ftell(archive);

    /*
    printf("Name: %s\n", entry.name);
    printf("UID: %d\nOriginal size: %d\nDisk size: %d\n", entry.uid, entry.original_size, entry.disk_size);
    printf("Is compressed: %d\nOffset:%ld\n", entry.is_compressed, entry.offset);
    */

    //grava os dados no arquivo
    fwrite(c_data, 1, compressed_size, archive);
    free(c_data);

    //atualiza diretorio
    header = realloc(header, (size + 1)*sizeof(member));
    if (!header)
    {
        fprintf(stderr, "Erro ao realocar memoria em insert_compressed\n");
        fclose(archive);
        return;
    }
    header[size] = entry;
    size++;

    save_directory(archive, &header, size);
    free(header);
    fclose(archive);

    printf("Arquivo inserido comprimido.\n");
}

void extract_member(char *arc_name, char *name)
{
    FILE *archive = fopen(arc_name, "rb");
    if (!archive)
    {
        fprintf(stderr, "Erro ao abrir arquivo em extract_member\n");
        return;
    }

    //carrega diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Arquivo vazio\n");
        fclose(archive);
        return;
    }

    int index = find_member(header, size, name);
    if (index == -1)
    {
        printf("Membro nao esta no archive\n");
        free(header);
        fclose(archive);
        return;
    }

    member extracted = header[index];
    /*
    printf("Extraindo: %s\n", extracted.name);
    printf("Offset: %ld, disk size: %u, original size: %u\n", extracted.offset, extracted.disk_size, extracted.original_size);
    */

    unsigned char *c_data = malloc(extracted.disk_size);
    if (!c_data)
    {
        fprintf(stderr, "Erro ao alocar memoria em extract_member\n");
        free(header);
        fclose(archive);
        return;
    }

    //le o membro extraido
    fseek(archive, extracted.offset, SEEK_SET);
    fread(c_data, 1, extracted.disk_size, archive);

    //cria arquivo de saido com o nome do arquivo selecionado
    FILE *out = fopen(extracted.name, "wb");
    if (!out)
    {
        fprintf(stderr, "Erro ao criar arquivo de saida\n");
        free(c_data);
        free(header);
        fclose(archive);
        fclose(out);
        return;
    }

    //descomprime se estiver comprimido
    if (extracted.is_compressed)
    {
        unsigned char *dc_data = malloc(extracted.original_size);
        if (!dc_data)
        {
            fprintf(stderr, "Erro ao alocar memoria em extract_member\n");
            free(c_data);
            free(header);
            fclose(archive);
            fclose(out);
            return;
        }

        LZ_Uncompress(c_data, dc_data, extracted.disk_size);

        //escreve no arquivo de saída
        fwrite(dc_data, 1, extracted.original_size, out);
        free(dc_data);
    }
    //escreve o arquivo direto se nao estiver comprimido
    else
        fwrite(c_data, 1, extracted.disk_size, out);

    printf("Arquivo %s extraido com sucesso\n", extracted.name);

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
        fprintf(stderr, "Erro ao abrir arquivo em extract_member\n");
        return;
    }

    //carrega diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Arquivo vazio\n");
        fclose(archive);
        return;
    }

    for (int i = 0; i < size; i++)
        extract_member(arc_name, header[i].name);

    printf("Todos os membros foram extraidos com sucesso\n");
}

void list_archive(char *arc_name)
{
    FILE *archive = fopen(arc_name, "rb");
    if (!archive)
    {
        fprintf(stderr, "Erro ao abrir arquivo em list_archive\n");
        return;
    }

    //carrega diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);
    
    if (size == 0 || header == NULL)
    {
        printf("Arquivo vazio\n");
        return;
    }

    for (int i = 0; i < size; i++)
    {
        printf("Nome: %s\n", header[i].name);
        printf("UID: %u\n", header[i].uid);
        printf("Tamanho original: %u bytes\n", header[i].original_size);
        printf("Tamanho em disco: %u bytes\n", header[i].disk_size);
        printf("Tamanho offset: %ld\n", header[i].offset);
        printf("Comprimido: ");
        if (header[i].is_compressed)
            printf("sim\n");
        else
            printf("nao\n");
        printf("Ultima modificacao: tempo\n");
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
        
        fprintf(stderr, "Erro ao abrir arquivo em remove_member\n");
        return;
    }

    //carrega diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Arquivo vazio\n");
        fclose(archive);
        return;
    }

    //se o tamanho do diretorio for 1, apaga o arquivo
    if (size == 1)
    {
        free(header);
        fclose(archive);
        remove(arc_name);
        printf("Ultimo membro removido\n");
        return;
    }

    //busca o membro a ser removido
    int index = find_member(header, size, name);
    if (index != -1)
        printf("Indice: %d\n", index);
    else
        printf("Indice nao encontrado\n");

    if (index == -1)
    {
        printf("Membro nao encontrado no diretorio\n");
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

    //fseek(archive, 0, SEEK_END);
    save_directory(archive, &header, size);

    free(header);
    fclose(archive);
    printf("Membro removido\n");
}

void move_member(char *arc_name, char *name, char *target)
{
    FILE *archive = fopen(arc_name, "rb+");
    if (!archive)
    {
        fprintf(stderr, "Erro ao abrir arquivo em move_member\n");
        return;
    }

    //carrega o diretorio
    member *header = NULL;
    int size = 0;
    load_directory(archive, &header, &size);

    if (size == 0 || header == NULL)
    {
        printf("Arquivo vazio\n");
        fclose(archive);
        return;
    }

    //procura indice dos membros
    int idx_name = find_member(header, size, name);
    int idx_target = find_member(header, size, target);

    if (idx_name == -1)
    {
        printf("Membro '%s' não encontrado\n", name);
        free(header);
        fclose(archive);
        return;
    }

    if (idx_target == -1)
    {
        printf("Membro target '%s' não encontrado\n", target);
        free(header);
        fclose(archive);
        return;
    }

    if (idx_name == idx_target)
    {
        printf("Membro e target são o mesmo, nenhuma mudança\n");
        free(header);
        fclose(archive);
        return;
    }

    //extrai o membro a ser movido
    member moving = header[idx_name];

    if (idx_name < idx_target)
    {
        //move todos os elementos entre idx_name+1 e idx_target uma posição para trás
        for (int i = idx_name; i < idx_target; i++)
            header[i] = header[i + 1];

        //insere o membro na nova posição
        header[idx_target] = moving;
    }
    else
    {
        //move todos os elementos entre idx_target+1 e idx_name-1 uma posição para frente
        for (int i = idx_name; i > idx_target + 1; i--)
            header[i] = header[i - 1];

        //insere o membro logo depois do target
        header[idx_target + 1] = moving;
    }

    printf("Membro '%s' movido para depois de '%s'\n", name, target);

    //atualiza o diretório no archive
    fseek(archive, 0, SEEK_END);
    save_directory(archive, &header, size);

    free(header);
    fclose(archive);
}
