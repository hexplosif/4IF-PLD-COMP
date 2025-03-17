int g = 100;
int main() {
    int g = 50;
    {
        int g = 25;
        {
            int g = 10;
            g = g + 5;
        }
        g = g * 2;
    }
    return g;
}