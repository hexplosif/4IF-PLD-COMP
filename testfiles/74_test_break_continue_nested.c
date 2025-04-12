int main() {
    int i = 0;
    int j = 0;
    int sum = 0;
    
    while (i < 3) {
        i = i + 1;
        j = 0;
        while (j < 5) {
            j = j + 1;
            if (j == 3) {
                continue;
            }
            if (i == 2 && j == 4) {
                break;
            }
            sum = sum + 1;
        }
    }
    return sum;  // 应该返回11
    // 解释：
    // i=1: j=1,2,4,5 (4次)
    // i=2: j=1,2,4 (3次)
    // i=3: j=1,2,4,5 (4次)
    // 总共11次
} 