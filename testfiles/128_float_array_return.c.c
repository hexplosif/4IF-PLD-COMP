float get_max(float arr[4]) {
    float max = arr[0];
    int i = 1;
    
    while (i < 4) {
        if (arr[i] > max) {
            max = arr[i];
        }
        i++;
    }
    
    return max;
}

int main() {
    float values[4] = {3.3, 7.7, 2.2, 5.5};
    float max_value = get_max(values);
    return max_value;
}