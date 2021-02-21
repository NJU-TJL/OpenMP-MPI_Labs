#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

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
    int myid = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &n_threads);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    //初始化
    if (argc >= 2) n = atoi(argv[1]);
    init();

    //计时开始
    double ts = MPI_Wtime();

    //各进程并行计算C：以行为单位进行划分计算任务
    for (int i = myid; i < n; i += n_threads) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    //将计算结果C的第i行，发送到主进程
    if(myid != 0){
        for (int i = myid; i < n; i += n_threads) {
            MPI_Send(C[i], n, MPI_INT, 0, i, MPI_COMM_WORLD);
        }
    }
    
    if (myid == 0) {
        //主进程接收其他进程的计算结果
        for (int id = 1; id < n_threads; id++) {
            for (int i = id; i < n; i += n_threads) {
                MPI_Recv(C[i], n, MPI_INT, id, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        //计算矩阵C元素之和
        printf("Sum of Matrix C:%llu\n", sum_C());

        //计算完成
        double te = MPI_Wtime();
        printf("Time:%f s\n", te - ts);
    }
    MPI_Finalize();
    return 0;
}