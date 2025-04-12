int main() {
    int a = 10;
    {
        int b = 5;
        {
            int c = -3;
            c = (c < 0 || b > 10) && (a == 10);
        }
        b = (b != 5 && a > 0) || (a < 20);
    }
    return (a > 5 && 1) || (0 || -1);
}