int main()
{
    int x = 0;
    if (1 == 1) {
        x = 10;
        if (x == 10) {
            x = 20;
        } else {
            x = 30;
        }
    }
    return x;
}