#
# 脚本使用方法： ./run.sh
# 功能：编译源代码，运行10轮
#

#编译生成
mpicc -o main main.c MyUtils.c
#运行10轮
for ((k=0; k<10; k++)); do
    echo Round${k}
    #每轮中，进程数（工人进程）取1 2 3 4 8 16 32 
	for i in 2 3 4 5 9 17 33 
    do
        mpirun -np ${i} ./main ./test/ dict.txt result.txt
    done
done