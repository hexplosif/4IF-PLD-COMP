int main() {
    float temp = 25.5;
    
    if (temp > 30.0f) {
        return 3;
    } else if (temp > 20.0f) {
        return 2;
    } else if (temp > 10.0f) {
        return 1;
    } else {
        return 0;
    }
}