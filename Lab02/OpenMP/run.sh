#
# 脚本使用方法： ./run.sh
# 功能：编译源代码，运行10轮
#

#编译生成
gcc -fopenmp -o LU_OpenMP LU_OpenMP.c 
#运行10轮
for ((k=0; k<10; k++)); do
    echo Round${k}
    #每轮中，线程数取1 2 4 8 16 32 
    for i in 1 2 4 8 16 32 
    do
        ./LU_OpenMP ${i}
    done
done
