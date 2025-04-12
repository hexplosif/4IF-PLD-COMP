int main() {
    float values[5] = {1.1, 2.2, 3.3, 4.4, 5.5};
    float sum = 0.0;
    int i = 0;
    
    while (i < 5) {
        sum += values[i];
        i++;
    }
    
    return sum;
}