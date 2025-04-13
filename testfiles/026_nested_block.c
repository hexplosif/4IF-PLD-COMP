int main() {
    int a = 2;
    {
        int b = 3;
        a = a * b;
    }
    return a;
}
