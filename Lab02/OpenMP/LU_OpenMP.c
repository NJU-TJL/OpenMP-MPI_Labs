#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<omp.h>

//线程数
int n_threads = 1;

//方阵
int **A;
int N = 0;
int **L, **U;

//从文件中读取矩阵A
void read_A(const char *input_file) {
	FILE* fp = fopen(input_file,"r");
	int m, n;
	fscanf(fp, "%d %d", &m, &n);
	if (m != n) {
		perror("矩阵A不是方阵，请检查输入数据！");
		exit(-1);
	}
	N = n;
	A = (int **)calloc(N, sizeof(int*));
	for (int i = 0; i < N; i++) {
		A[i] = (int *)calloc(N, sizeof(int));
		for (int j = 0; j < N; j++) {
			fscanf(fp, "%d", &A[i][j]);
		}
	}
	//生成和初始化 L、U矩阵
	L = (int **)calloc(N, sizeof(int*));
	for (int i = 0; i < N; i++) {
		L[i] = (int *)calloc(N, sizeof(int));
		for (int j = 0; j < N; j++) {
			L[i][j] = 0;
		}
	}
	U = (int **)calloc(N, sizeof(int*));
	for (int i = 0; i < N; i++) {
		U[i] = (int *)calloc(N, sizeof(int));
		for (int j = 0; j < N; j++) {
			U[i][j] = 0;
		}
	}
}

//按公式计算，L、U矩阵中i行j列的累加和（K个累加和）
int sum_i_j_K(int i, int j, int K) {
	int res = 0;
	for (int k = 0; k < K; k++) {
		res += L[i][k] * U[k][j];
	}
	return res;
}

//输出L、U矩阵到文件中
void printLU() {
	FILE* fpL = fopen("L.out", "w");
	FILE* fpU = fopen("U.out", "w");
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
	//设置线程数
    if (argc >= 2) n_threads = atoi(argv[1]);
    
	//初始化矩阵A
	if (argc >= 3) {
		//通过运行参数，指定输入文件
		read_A(argv[2]);
	}
	else {
		//默认使用"LU.in"作为输入文件
		read_A("LU.in");
	}

	//设置线程数
    omp_set_num_threads(n_threads);

    //计时开始
    double ts = omp_get_wtime();
	
	//计算L、U矩阵	
	for (int i = 0; i < N; i++) {
		U[i][i] = A[i][i] - sum_i_j_K(i, i, i);
		L[i][i] = 1;
		#pragma omp parallel for
		for (int j = i+1; j < N; j++) {
			//按照递推公式进行计算
			U[i][j] = A[i][j] - sum_i_j_K(i, j, i);
			L[j][i] = (A[j][i] - sum_i_j_K(j, i, i)) / U[i][i];
		}
	}

	//输出L、U矩阵到文件中
	printLU();

	//计时结束
    double te = omp_get_wtime();
    printf("Time:%f s\n", te - ts);
}