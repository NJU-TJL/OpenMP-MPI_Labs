#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

//线程数
int n_threads = 1;

//阶数
int n = 1000;

//方阵A、B、C
int **A, **B, **C;

//按照实验要求，按指定格式初始化A、B方阵、C方阵
void init() {
    A = (int **)calloc(n, sizeof(int *));
    for (int i = 0; i < n; i++) {
        A[i] = (int *)calloc(n, sizeof(int));
        for (int j = 0; j < n; j++) {
            A[i][j] = i + 1 + j;
        }
    }
    B = (int **)calloc(n, sizeof(int *));
    for (int i = 0; i < n; i++) {
        B[i] = (int *)calloc(n, sizeof(int));
        for (int j = 0; j < n; j++) {
            B[i][j] = 1;
        }
    }
    C = (int **)calloc(n, sizeof(int *));
    for (int i = 0; i < n; i++) {
        C[i] = (int *)calloc(n, sizeof(int));
        for (int j = 0; j < n; j++) {
            C[i][j] = 0;
        }
    }
}

//计算矩阵C元素之和
unsigned long long sum_C()
{
    unsigned long long sum = 0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            sum += C[i][j];
        }
    }
    return sum;
}

int main(int argc, char *argv[]) {
    //设置线程数
    if (argc >= 2) n_threads = atoi(argv[1]);
    //设置阶数
    if (argc >= 3) n = atoi(argv[2]);

    //初始化A、B方阵、C方阵
    init();

    //设置线程数
    omp_set_num_threads(n_threads);

    //计时开始
    double ts = omp_get_wtime();

    //计算C
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    //计算矩阵C元素之和
    printf("Sum of Matrix C:%llu\n", sum_C());

    //计时结束
    double te = omp_get_wtime();
    printf("Time:%f s\n", te - ts);
}