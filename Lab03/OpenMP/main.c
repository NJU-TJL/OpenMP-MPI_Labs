#include <malloc.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "MyUtils.h"

// 线程数
int n_threads = 1;

// 命令行参数
#define THREAD_NUM_ARG 1
#define DIR_ARG 2
#define DICT_ARG 3
#define RES_ARG 4

// 字典文件内容读取缓存
#define DICT_FILE_BUFFER_SIZE 1000000
char dict_file_buffer[DICT_FILE_BUFFER_SIZE];

int main(int argc, char *argv[]) {
    int dict_size;     // 字典大小（即：字典中有多少个词）
    int file_count;    // 目标目录下总文件文件个数
    char **filenames;  // 目标目录下所有文件的文件名
    int **vectors;     // 存放所有文件的文档向量

    // 异常情况报错提示
    if (argc != 5) {
        printf("程序需要输入4个参数，用法如下：\n");
        printf("%s <n_threads> <dir> <dict> <results>\n", argv[0]);
        exit(-1);
    }
    // 设置线程数
    n_threads = atoi(argv[THREAD_NUM_ARG]);
    // 设置线程数
    omp_set_num_threads(n_threads);
    // 计时开始
    double ts = omp_get_wtime();

    /* ++++++++++ 主体工作部分 ++++++++++ */
    // 根据字典文件内容，构建Hash
    readAll(dict_file_buffer, argv[DICT_ARG]);
    dict_size = make_dict_Hash(dict_file_buffer);
    // 从指定目录下获取文件名列表
    file_count = get_names(argv[DIR_ARG], &filenames);
    // 分配存放所有文件的文档向量的空间
    vectors = (int **)calloc(file_count, sizeof(int *));
#pragma omp parallel for
    for (int i = 0; i < file_count; ++i) {
        vectors[i] = (int *)calloc(dict_size, sizeof(int));
        // 读取文件并生成文档向量
        make_profile(filenames[i], dict_size, vectors[i]);
    }
    // 写入结果文件
    write_profiles(argv[RES_ARG], file_count, dict_size, filenames, vectors);

    // 计时结束
    double te = omp_get_wtime();
    printf("Time:%f s\n", te - ts);
}