int main(){
    int arr[3], a, b, c;
    a = 10;
    b = 20;
    arr[0] = a;
    arr[1] = arr[0] + a + b;
    arr[2] = arr[1] + a + b;
    arr[0] = 10;
    arr[0] += 5;
    arr[1] = 20;
    arr[1] -= 2;
    arr[2] = 30;
    arr[2] *= 3;
    arr[2] /= 6;
    c = arr[0] + arr[1] + arr[2];
    return c; 
}