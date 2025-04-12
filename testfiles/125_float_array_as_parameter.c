float average(float arr[3]) {
    return (arr[0] + arr[1] + arr[2]) / 3.0;
}

int main() {
    float grades[3] = {85.5, 90.0, 78.5};
    float avg = average(grades);
    return avg;
}