int x = 5;
int y = 10;
int z;
int main() {
    int x = 2;
    {
        int y = 3;
        x = x + y;
        {
            int x = 7;
            x = x * 2;
            z = x + y;
        }
        z = z + x;
    }
    return x + z;
}