int square(int x) {
    return x * x;
}

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(square(3), square(4));
    return result;
}