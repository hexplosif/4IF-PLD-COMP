int main() {
    float outer = 5.5;
    
    {
        float inner = 2.5;
        outer = outer + inner;
        
        {
            float deepest = 1.5;
            outer = outer * deepest;
        }
    }
    
    return outer;
}