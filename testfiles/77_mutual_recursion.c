int is_odd(int n) {
    if (n == 0) return 0;
    return is_even(n - 1);
}

int is_even(int n) {
    if (n == 0) return 1;
    return is_odd(n - 1);
}

int main() {
    int result = is_even(6) + is_odd(9) * 10;
    return result;
}
