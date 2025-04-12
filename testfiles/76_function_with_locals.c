int complex_calculation(int a, int b) {
    int x = a * 2;
    int y = b - 5;
    int z = x + y;
    return z * z;
}

int main() {
    int result = complex_calculation(3, 8);
    return result;
}