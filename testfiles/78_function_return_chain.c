int step1(int x) {
    return x + 5;
}

int step2(int x) {
    return x * 2;
}

int step3(int x) {
    return x - 3;
}

int main() {
    return step3(step2(step1(4)));
}