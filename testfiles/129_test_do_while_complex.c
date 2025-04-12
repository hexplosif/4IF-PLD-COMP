int main() {
    int i = 0;
    int sum = 0;
    int count = 0;
    
    // 测试do...while与if-else组合
    do {
        if (i % 2 == 0) {
            sum = sum + i;
        } else {
            count = count + 1;
        }
        i = i + 1;
    } while (i < 10);
    
    // 测试do...while与while组合
    int j = 0;
    int product = 1;
    
    do {
        int k = 0;
        while (k < 3) {
            product = product * 2;
            k = k + 1;
        }
        j = j + 1;
    } while (j < 3);
    
    // 测试do...while与函数调用
    int x = 0;
    int y = 0;
    
    do {
        y = y + square(x);
        x = x + 1;
    } while (x < 5);
    
    // 返回计算结果
    return sum + count + product + y;
}

// 辅助函数
int square(int n) {
    return n * n;
} 