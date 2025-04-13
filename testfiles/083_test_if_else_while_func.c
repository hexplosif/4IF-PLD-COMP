void testFunction(int x) {
    if (x > 0) {
        x = x - 1;
    } else {
        x = x + 1;
    }

    while (x < 10) {
        x = x + 2;
    }
}

int main() {
    int a = 5;
    testFunction(a);

    int b = -3;
    testFunction(b);

    return 0;
}
