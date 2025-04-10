float get_pi() {
    return 3.14159;
}

int main() {
    float pi = get_pi();
    if (pi > 3.0f && pi < 4.0f) {
        return 1;
    }
    return 0;
}