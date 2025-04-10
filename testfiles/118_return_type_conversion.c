float get_value() {
    int i = 42;
    return i;
}

int main() {
    float f = get_value();
    if (f == 42.0f) {
        return 1;
    }
    return 0;
}