int main() {
    int i = 0;
    int sum = 0;
    
    // 测试do...while中的break
    do {
        if (i == 5) {
            break;
        }
        sum = sum + i;
        i = i + 1;
    } while (i < 10);
    
    // 测试do...while中的continue
    int j = 0;
    int count = 0;
    
    do {
        j = j + 1;
        if (j % 2 == 0) {
            continue;
        }
        count = count + 1;
    } while (j < 10);
    
    // 测试嵌套do...while中的break
    int x = 0;
    int y = 0;
    int result = 0;
    
    do {
        y = 0;
        do {
            if (x == 1 && y == 1) {
                break;
            }
            result = result + x + y;
            y = y + 1;
        } while (y < 3);
        x = x + 1;
    } while (x < 3);
    
    // 返回计算结果
    return sum + count + result;
} 