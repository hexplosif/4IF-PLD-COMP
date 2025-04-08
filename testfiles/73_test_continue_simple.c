int main() {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        i = i + 1;
        if (i % 2 == 0) {
            continue;
        }
        sum = sum + i;
    }
    return sum;  // 应该返回25 (1+3+5+7+9)
} 