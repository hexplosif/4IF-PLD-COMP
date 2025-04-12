int main() {
    int i = 0;
    int sum = 0;
    
    // 基本do...while循环测试
    do {
        sum = sum + i;
        i = i + 1;
    } while (i < 10);
    
    // 测试do...while循环至少执行一次
    int j = 20;
    do {
        j = j + 1;
    } while (j < 10);
    
    // 测试嵌套do...while循环
    int x = 0;
    int y = 0;
    int result = 0;
    
    do {
        y = 0;
        do {
            result = result + x + y;
            y = y + 1;
        } while (y < 3);
        x = x + 1;
    } while (x < 3);
    
    // 返回sum + j + result作为测试结果
    return sum + j + result;
} 