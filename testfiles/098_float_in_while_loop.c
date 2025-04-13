int main() {
    float sum = 0.0;
    float i = 0.0;
    
    while (i < 5.0f) {
        sum += i;
        i += 0.5;
    }
    
    return sum;
}