#include <malloc.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

//进程数
int n_threads = 1;

//方阵
int **A;
int N = 0;
int **L, **U;

//初始化
void init(const char *input_file) {
    //从文件中读取矩阵A
    FILE *fp = fopen(input_file, "r");
    int m, n;
    fscanf(fp, "%d %d", &m, &n);
    if (m != n) {
        perror("矩阵A不是方阵，请检查输入数据！");
        exit(-1);
    }
    N = n;
    A = (int **)calloc(N, sizeof(int *));
    for (int i = 0; i < N; i++) {
        A[i] = (int *)calloc(N, sizeof(int));
        for (int j = 0; j < N; j++) {
            fscanf(fp, "%d", &A[i][j]);
        }
    }
    fclose(fp);
}

//从计算好的A中分离、整理，得到最终L、U结果
void getLU() {
    //生成和初始化 L、U矩阵
    L = (int **)calloc(N, sizeof(int *));
    for (int i = 0; i < N; i++) {
        L[i] = (int *)calloc(N, sizeof(int));
    }
    U = (int **)calloc(N, sizeof(int *));
    for (int i = 0; i < N; i++) {
        U[i] = (int *)calloc(N, sizeof(int));
    }
    //从计算好的A中，分离得到L、U
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i < j) {
                U[i][j] = A[i][j];
                L[i][j] = 0;
            } else if (i == j) {
                U[i][j] = A[i][j];
                L[i][j] = 1;
            } else {
                U[i][j] = 0;
                L[i][j] = A[i][j];
            }
        }
    }
    //输出L、U矩阵到文件中
    FILE *fpL = fopen("L.out", "w");
    FILE *fpU = fopen("U.out", "w");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(fpL, "%d ", L[i][j]);
            fprintf(fpU, "%d ", U[i][j]);
        }
        fprintf(fpL, "\n");
        fprintf(fpU, "\n");
    }
}

int main(int argc, char *argv[]) {
    int myid = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &n_threads);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    //初始化矩阵A
    if (argc >= 2) {
        //通过运行参数，指定输入文件
        init(argv[1]);
    } else {
        //或者：默认使用"LU.in"作为输入文件        
        init("LU.in");
    }

    //计时开始
    double ts = MPI_Wtime();

    //逐行作为主行元素，进行初等行变换
    for (int i = 0; i < N; i++) {
        //同步当前的主行元素到所有进程，
        MPI_Bcast(A[i], N, MPI_INT, i % n_threads, MPI_COMM_WORLD);

        //各进程将自己所负责的行完成行初等变换
        int j = myid;
        while (j < i + 1) j += n_threads;
        for (; j < N; j += n_threads) {
            A[j][i] = A[j][i] / A[i][i];  //初等行变换的第一步，第一个元素作除法（也相当于计算出了L[j][i]）
            for (int k = i + 1; k < N; k++) {
                A[j][k] = A[j][k] - A[i][k] * A[j][i];  //初等行变化第二步，该行其他元素进行初等行变化
            }
        }
    }
    //将计算结果汇总到0号进程
    if (myid != 0) {
        //各进程发送自己负责计算的那些行
        for (int i = myid; i < N; i += n_threads) {
            MPI_Send(A[i], N, MPI_INT, 0, i, MPI_COMM_WORLD);
        }
    } else {
        // 0号进程依次接收
        for (int i = 0; i < N; i++) {
            if (i % n_threads != 0) {
                MPI_Recv(A[i], N, MPI_INT, i % n_threads, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        //得到分解的L、U矩阵，并且将结果写入文件
        getLU();

        //计时结束
        double te = MPI_Wtime();
        printf("Time:%f s\n", te - ts);
    }
    MPI_Finalize();
    return 0;
}
