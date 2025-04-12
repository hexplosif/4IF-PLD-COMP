int main() {
    float x = 5.5;
    
    {
        float x = 10.5;
        x = x + 1.0;
    }
    
    return x;
}