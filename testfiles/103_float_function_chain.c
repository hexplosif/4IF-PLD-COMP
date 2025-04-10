float multiply(float x, float y) {
    return x * y;
}

float add(float x, float y) {
    return x + y;
}

int main() {
    float result = add(multiply(2.5f, 3.0f), 1.5f);
    return result;
}