#include "MyUtils.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ++++++++++ 字典处理、哈希表相关 ++++++++++ */
//  哈希表表项
typedef struct HashTableItem {
    char *word;  // 单词
    int index;   // 单词在字典中的下标（即文档向量中，此单词的下标位置）
} HashTableItem;

// 哈希链表节点
typedef struct HashListNode {
    HashTableItem *val;
    struct HashListNode *next;
} HashListNode;

// 哈希表头结点数组
#define HASHTABLE_SIZE (0x3fff + 1)
HashListNode *hashTable[HASHTABLE_SIZE];

// 初始化Hash表
void init_hashTable() {
    for (int i = 0; i < HASHTABLE_SIZE; i++) hashTable[i] = NULL;
}

// 一个对字符串Hash效果较好的函数。
// P.J.Weinberger提出（http://en.wikipedia.org/wiki/Peter_J._Weinberger）
unsigned hash_pjw(const char *name) {
    unsigned val = 0, i;
    for (int k = 0; name[k] != '\0'; k++) {
        val = (val << 2) + name[k];
        if (i = val & ~0x3fff) val = (val ^ (i >> 12)) & 0x3fff;
    }
    return val;
}

//  Hash表中插入项目
void hash_insert(const char *word, int word_index) {
    // 构建Hash表项
    HashTableItem *val = malloc(sizeof(HashTableItem));
    val->word = copyFrom(word);
    val->index = word_index;

    // 插入Hash表
    unsigned index = hash_pjw(word);
    // 头插法 构建链地址法hash链表
    HashListNode *p = malloc(sizeof(HashListNode));
    p->val = val;
    p->next = hashTable[index];
    hashTable[index] = p;
}
//  查找Hash表
int getWordIndex(const char *name) {
    unsigned index = hash_pjw(name);
    HashListNode *p = hashTable[index];
    while (p != NULL) {
        if (strcmp(name, p->val->word) == 0) {
            return p->val->index;  // 返回单词name的下标
        }
        p = p->next;
    }
    return -1;
}

/* ++++++++++ Worker部分相关函数 ++++++++++ */
// 读取文本文件中全部字节内容到buffer中，返回文件字节大小
long readAll(char *buffer, const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    fread(buffer, sizeof(char), fileSize, fp);
    buffer[fileSize] = '\0';
    fclose(fp);
    return fileSize;
}

// 从字典文本中，生成Hash表
int make_dict_Hash(char *dict_buffer) {
    init_hashTable();
    int i = 0;
    char *token = strtok(dict_buffer, " \r\n\t");
    while (token != NULL) {
        hash_insert(token, i);
        ++i;
        token = strtok(NULL, " \r\n\t");
    }
    return i;
}

// 从src产生一个字符串副本，动态分配空间
#define TEMP_STR_MAX_LENGTH 1000
char temp_str[TEMP_STR_MAX_LENGTH];  // Temp string for construction
char *copyFrom(const char *src) {
    char *dst = NULL;
    dst = malloc(strlen(src) + 1);
    strcpy(dst, src);
    return dst;
}

#define SINGLE_WORD_MAX_LENGTH 50
void make_profile(const char *filename, int dict_size, int *profile) {
    char word[SINGLE_WORD_MAX_LENGTH];
    FILE *fp = fopen(filename, "r");
    // 清空文档向量
    for (int i = 0; i < dict_size; ++i) {
        profile[i] = 0;
    }
    // 初始化word
    for (int i = 0; i < SINGLE_WORD_MAX_LENGTH; ++i)
        word[i] = '\0';
    while (fscanf(fp, "%s", word) != EOF) {
        int index = getWordIndex(word);
        if (index != -1) {
            ++profile[index];
        }
        // 初始化word
        for (int i = 0; i < SINGLE_WORD_MAX_LENGTH; ++i)
            word[i] = '\0';
    }
    fclose(fp);
}

/* ++++++++++ Manager部分相关函数 ++++++++++ */
// 对文件名进行排序
int filename_cmp(const void *a, const void *b) {
    if (strlen(*(char **)a) != strlen(*(char **)b))
        // 先按文件名长度比较
        return strlen(*(char **)a) - strlen(*(char **)b);
    else
        // 长度相同，返回字符比较结果
        return strcmp(*(char **)a, *(char **)b);
}
// 读取目录，生成文件名数组
int get_names(const char *dir, char ***filenames) {
    DIR *dp;
    struct dirent *dirp;
    int count = 0;
    int i = 0;

    // 打开目录
    dp = opendir(dir);
    // 先遍历一遍，获得文件总数
    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_type == 8) {  // 查dirent.h可知，d_type值为8的，代表是普通文件
            ++count;
        }
    }
    // 根据count分配文件名数组大小
    *filenames = (char **)calloc(count, sizeof(char *));
    // 重置目录指针
    rewinddir(dp);
    // 再次遍历，记录文件名
    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_type == 8) {  // 查dirent.h可知，d_type值为8的，代表是普通文件
            size_t filename_len = strlen(dir) + strlen(dirp->d_name) + 1;
            (*filenames)[i] = (char *)malloc(filename_len);
            sprintf((*filenames)[i], "%s%s", dir, dirp->d_name);
            (*filenames)[i][filename_len - 1] = '\0';
            ++i;
        }
    }
    //按照文件名字进行排序
    qsort((*filenames), count, sizeof(char *), filename_cmp);
    closedir(dp);
    return count;
}
// 将文档向量汇总结果写入结果文件
void write_profiles(const char *filepath, int file_count, int dict_size, char **filenames, int **vectors) {
    FILE *fp = fopen(filepath, "w");
    for (int i = 0; i < file_count; ++i) {
        fprintf(fp, "%s\n", filenames[i]);
        for (int j = 0; j < dict_size; ++j) {
            fprintf(fp, "%d ", vectors[i][j]);
        }
        fputc('\n', fp);
    }
    fclose(fp);
}