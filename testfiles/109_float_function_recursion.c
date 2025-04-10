float factorial(float n) {
    if (n <= 1.0) {
        return 1.0;
    }
    return n * factorial(n - 1.0);
}

int main() {
    float result = factorial(4.0f);
    return result;
}