int main() {
    int i = 0;
    while (i < 10) {
        if (i == 5) {
            break;
        }
        i = i + 1;
    }
    return i;  // 应该返回5
} 