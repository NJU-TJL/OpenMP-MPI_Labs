#include <malloc.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MyUtils.h"

// 命令行参数
#define DIR_ARG 1
#define DICT_ARG 2
#define RES_ARG 3

// 管理者、工人通信TAG类型标识
#define DICT_SIZE_MSG_TAG 0
#define FILENAME_MSG_TAG 1
#define VECTOR_MSG_TAG 2
#define EMPTY_MSG_TAG 3

// 字典文件内容读取缓存
#define DICT_FILE_BUFFER_SIZE 1000000
char dict_file_buffer[DICT_FILE_BUFFER_SIZE];

void manager(char *argv[], int p) {
    int dict_size;     // 字典大小（即：字典中有多少个词）
    int file_count;    // 目标目录下总文件文件个数
    char **filenames;  // 目标目录下所有文件的文件名
    int *buffer;       // 接收文档向量时的缓存
    int **vectors;     // 存放所有文件的文档向量
    int terminated;    // 记录已结束的工人进程个数
    int assign_count;  // 已分配出去的文档个数
    int *assigned;     // 记录i号工人进程分配到的是哪个文档
    int src;           // 消息来自哪个工人
    int tag;           // 消息Tag
    MPI_Status status;
    MPI_Request pending;

    // 从工人进程获取字典大小
    MPI_Irecv(&dict_size, 1, MPI_INT, MPI_ANY_SOURCE, DICT_SIZE_MSG_TAG, MPI_COMM_WORLD, &pending);
    // 从指定目录下获取文件名列表
    file_count = get_names(argv[DIR_ARG], &filenames);
    // 等待获取字典大小完成
    MPI_Wait(&pending, &status);
    // 分配存放所有文件的文档向量的空间
    buffer = (int *)calloc(dict_size, sizeof(int));  // 缓存
    vectors = (int **)calloc(file_count, sizeof(int *));
    for (int i = 0; i < file_count; ++i) {
        vectors[i] = (int *)calloc(dict_size, sizeof(int));
    }
    // 回应工人进程请求
    terminated = 0;
    assign_count = 0;
    assigned = (int *)calloc(p, sizeof(int));
    do {
        // 接收工人进程给出的文档向量
        MPI_Recv(buffer, dict_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        src = status.MPI_SOURCE;
        tag = status.MPI_TAG;
        if (tag == VECTOR_MSG_TAG) {
            // 如果不是空消息，就存储下文档向量
            for (int i = 0; i < dict_size; ++i)
                vectors[assigned[src]][i] = buffer[i];
        }
        // 给这个工人进程，分配下一个文档处理任务，或者告诉它终止
        if (assign_count < file_count) {
            // 分配下一个文档处理任务
            MPI_Send(filenames[assign_count], strlen(filenames[assign_count]) + 1, MPI_CHAR, src, FILENAME_MSG_TAG, MPI_COMM_WORLD);
            assigned[src] = assign_count;
            ++assign_count;
        } else {
            // 告诉它终止
            MPI_Send(NULL, 0, MPI_CHAR, src, FILENAME_MSG_TAG, MPI_COMM_WORLD);
            ++terminated;
        }

    } while (terminated < (p - 1));
    // 写入结果文件
    write_profiles(argv[RES_ARG], file_count, dict_size, filenames, vectors);
}

void worker(char *argv[], MPI_Comm worker_comm) {
    int all_id;        // MPI_COMM_WORLD中的ID
    int worker_id;     // worker_comm中的ID
    long file_len;     // 字典文件字节长度
    int dict_size;     // 字典大小（即：字典中有多少个词）
    int filename_len;  // 暂存文件名长度
    char *filename;    // 文件路径
    int *profile;      // 文档向量（即：文档中，每次的出现个数）
    MPI_Status status;
    MPI_Request pending;

    // 工人进程获得工人通信域的ID
    MPI_Comm_rank(worker_comm, &worker_id);
    // 通知管理进程，此工人进程可以开始工作了
    MPI_Isend(NULL, 0, MPI_UNSIGNED_CHAR, 0, EMPTY_MSG_TAG, MPI_COMM_WORLD, &pending);
    // 0号工人读文件
    if (!worker_id) {
        file_len = readAll(dict_file_buffer, argv[DICT_ARG]);
    }
    // 广播文件大小
    MPI_Bcast(&file_len, 1, MPI_LONG, 0, worker_comm);
    // 广播文件内容
    MPI_Bcast(dict_file_buffer, file_len, MPI_CHAR, 0, worker_comm);
    // 根据字典文件内容，构建Hash
    dict_size = make_dict_Hash(dict_file_buffer);
    // 分配存储文档向量的数组空间
    profile = (int *)calloc(dict_size, sizeof(int));
    // 0号工人，告诉管理者字典大小
    if (!worker_id) {
        MPI_Send(&dict_size, 1, MPI_INT, 0, DICT_SIZE_MSG_TAG, MPI_COMM_WORLD);
    }
    while (1) {
        // 从消息中，获知文件路径长度
        MPI_Probe(0, FILENAME_MSG_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_CHAR, &filename_len);
        // 为0，说明没有更多的工作了
        if (0 == filename_len) break;
        filename = (char *)malloc(filename_len);
        MPI_Recv(filename, filename_len, MPI_CHAR, 0, FILENAME_MSG_TAG, MPI_COMM_WORLD, &status);
        // 读取文件并生成文档向量
        make_profile(filename, dict_size, profile);
        free(filename);
        // 发送文档向量
        MPI_Send(profile, dict_size, MPI_INT, 0, VECTOR_MSG_TAG, MPI_COMM_WORLD);
    }
    free(profile);
}

int main(int argc, char *argv[]) {
    int id;                // 当前进程ID
    int p;                 // 总进程数
    MPI_Comm worker_comm;  // 仅包含工人进程的通信域
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // 异常情况报错提示
    if (argc != 4) {
        if (!id) {
            printf("程序需要输入3个参数，用法如下：\n");
            printf("%s <dir> <dict> <results>\n", argv[0]);
            exit(-1);
        }
    } else if (p < 2) {
        printf("错误：进程数需要大于1!\n");
        exit(-2);
    }

    //计时开始
    double ts = MPI_Wtime();

    // 正常工作
    if (!id) {
        MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, id, &worker_comm);
        manager(argv, p);

        //计时结束
        double te = MPI_Wtime();
        printf("Time:%f s\n", te - ts);
    } else {
        MPI_Comm_split(MPI_COMM_WORLD, 0, id, &worker_comm);
        worker(argv, worker_comm);
    }

    MPI_Finalize();  // 不能缺少的结尾
}