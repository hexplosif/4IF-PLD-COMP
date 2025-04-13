int main() {
    int x = 3;
    {
        int y = 7;
        x = x + y;
    }
    return x;
}