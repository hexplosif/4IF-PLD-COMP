int test_condition(int x) {
    if (x > 10) {
        return 100;
    } else if (x > 5) {
        return 50;
    } else {
        return 10;
    }
}

int main() {
    int a = test_condition(15);
    int b = test_condition(7);
    int c = test_condition(3);
    return a - b + c;
}