int main() {
    int arr[10], dsmd[3], vari, result;
    arr[0] = 10; 
    arr[1] = 20;
    arr[2 + 3 * 2] = 30;
    vari = 2;
    arr[3] = vari * 5; 
    dsmd[0] = 50;
    dsmd[1] = dsmd[0] / 2;
    dsmd[2] = dsmd[1] + arr[3]; 
    arr[4] = arr[0] + arr[1] + dsmd[2]; 
    arr[5] = arr[4] - dsmd[1]; 
    return arr[5] * 2; 
}